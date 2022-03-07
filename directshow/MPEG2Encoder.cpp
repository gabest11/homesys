#include "StdAfx.h"
#include "Mpeg2Encoder.h"
#include "MediaTypeEx.h"
#include <initguid.h>
#include "moreuuids.h"
#include "libavcodec.h"

#define MAX_PACKET_SIZE 81920

CMpeg2EncoderFilter::CMpeg2EncoderFilter()
	: CTransformFilter(L"CMpeg2EncoderFilter", NULL, __uuidof(*this))
	, m_size(0)
{
	m_libavcodec = new libavcodec();
}

CMpeg2EncoderFilter::~CMpeg2EncoderFilter()
{
	try
	{
		delete m_libavcodec;
	}
	catch(...)
	{
		// printf("***\n"); // hm
	}
}

HRESULT CMpeg2EncoderFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	if(m_pInput->SampleProps()->dwStreamId != AM_STREAM_MEDIA)
	{
		return m_pOutput->Deliver(pIn);
	}

	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt != NULL)
	{
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	return Transform(pIn);
}

HRESULT CMpeg2EncoderFilter::Transform(IMediaSample* pIn)
{
	HRESULT hr;

	const CMediaType& mt = m_pInput->CurrentMediaType();
	const CMediaType& mtOut = m_pOutput->CurrentMediaType();

	MPEG2VIDEOINFO* vihIn = (MPEG2VIDEOINFO*)mt.pbFormat;
	MPEG2VIDEOINFO* vihOut = (MPEG2VIDEOINFO*)mtOut.pbFormat;

	BITMAPINFOHEADER bih;

	CMediaTypeEx(mt).ExtractBIH(&bih);

	BYTE* src = NULL;
	
	pIn->GetPointer(&src);

	int len = pIn->GetActualDataLength();

	REFERENCE_TIME start = 0, stop = 0;
	
	hr = pIn->GetTime(&start, &stop);

	if(FAILED(hr))
	{
		start = stop = 0;
	}

	if(mt.subtype == MEDIASUBTYPE_IYUV)
	{
		if(SUCCEEDED(hr))
		{
			m_start.push_back(start);
		}

		int wh = bih.biWidth * bih.biHeight;

		BYTE* y = src;
		BYTE* u = y + wh;
		BYTE* v = u + (wh >> 2);

		bool keyframe;
		int pict_type;

		BYTE* buff = &m_buff[0];

		len = m_libavcodec->EncodeVideo(y, u, v, bih.biWidth, buff, m_buff.size(), &keyframe, &pict_type);

		for(int i = 0, size = 0; i < len; i += size)
		{
			size = min(MAX_PACKET_SIZE, len - i);

			CComPtr<IMediaSample> pOut;

			if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0)))
			{
				return hr;
			}

			pOut->SetDiscontinuity(FALSE);
			
			if(i == 0)
			{
				if(keyframe)
				{
					pOut->SetSyncPoint(TRUE);
				}

				if(!m_start.empty())
				{
					start = m_start.front();
					stop = start + 1;

					m_start.pop_front();

					pOut->SetTime(&start, &stop);
				}
			}

			BYTE* dst = NULL;

			pOut->GetPointer(&dst);

			ASSERT(size <= pOut->GetSize());

			memcpy(dst, &buff[i], size);

			pOut->SetActualDataLength(size);

			m_pOutput->Deliver(pOut);
		}
	}
	else if(mt.subtype == MEDIASUBTYPE_PCM)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;

		{
			int i = min(m_buff.size() - m_size, len);
			memcpy(&m_buff[m_size], src, i);
			m_size += i;
		}

		BYTE* buff = new BYTE[3840];

		int i = 0;

		while(i < m_size)
		{
			int frame_size = 0;

			len = m_libavcodec->EncodeAudio(&m_buff[i], m_size - i, buff, 3840, &frame_size);

			if(len <= 0)
			{
				break;
			}

			i += frame_size;

			REFERENCE_TIME dur = 10000000i64 * frame_size / (wfe->nSamplesPerSec * wfe->nChannels * wfe->wBitsPerSample >> 3);

			CComPtr<IMediaSample> pOut;

			if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0)))
			{
				return hr;
			}

			stop = start + dur;

			pOut->SetTime(&start, &stop);

			start = stop;

			pOut->SetDiscontinuity(FALSE);
			pOut->SetSyncPoint(TRUE);

			BYTE* dst = NULL;

			pOut->GetPointer(&dst);

			ASSERT(len <= pOut->GetSize());

			memcpy(dst, buff, len);

			pOut->SetActualDataLength(len);

			m_pOutput->Deliver(pOut);
		}

		memmove(&m_buff[0], &m_buff[i], m_size - i);

		m_size -= i;

		delete [] buff;
	}

	return S_OK;
}

HRESULT CMpeg2EncoderFilter::StartStreaming()
{
	m_start.clear();
	m_frame = 0;

	if(m_pInput->IsConnected())
	{
		const CMediaType& mt = m_pInput->CurrentMediaType();

		if(mt.subtype == MEDIASUBTYPE_IYUV)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;

			if(!m_libavcodec->InitMpeg2Encoder(vih->bmiHeader.biWidth, abs(vih->bmiHeader.biHeight), true, false))
			{
				return E_FAIL;
			}
		}
		else if(mt.subtype == MEDIASUBTYPE_PCM)
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;

			if(!m_libavcodec->InitAC3Encoder(wfe->nSamplesPerSec, wfe->nChannels))
			{
				return E_FAIL;
			}
		}

		m_buff.resize(2048 * 2048 * 4);
		m_size = 0;
	}

	return __super::StartStreaming();
}

HRESULT CMpeg2EncoderFilter::StopStreaming()
{
	m_start.clear();
	m_buff.clear();

	return __super::StopStreaming();
}

HRESULT CMpeg2EncoderFilter::CheckInputType(const CMediaType* mtIn)
{
	BITMAPINFOHEADER bih;

	CMediaTypeEx(*mtIn).ExtractBIH(&bih);

	return mtIn->majortype == MEDIATYPE_Video 
		&& (mtIn->subtype == MEDIASUBTYPE_IYUV && mtIn->formattype == FORMAT_VideoInfo && bih.biHeight > 0 
		|| mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
		|| mtIn->majortype == MEDIATYPE_Audio 
		&& (mtIn->subtype == MEDIASUBTYPE_PCM && mtIn->formattype == FORMAT_WaveFormatEx 
		|| mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
		|| mtIn->subtype == MEDIASUBTYPE_DOLBY_AC3)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2EncoderFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(FAILED(CheckInputType(mtIn)) 
	|| !(mtOut->majortype == MEDIATYPE_Video && mtOut->subtype == MEDIASUBTYPE_MPEG2_VIDEO 
		|| mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_DOLBY_AC3 
		|| mtOut->majortype == MEDIATYPE_Stream && mtOut->subtype == MEDIASUBTYPE_MPEG2_PROGRAM))
		return VFW_E_TYPE_NOT_ACCEPTED;

	return S_OK;
}

HRESULT CMpeg2EncoderFilter::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	pProperties->cBuffers = 100;
	pProperties->cbBuffer = MAX_PACKET_SIZE;
    
	return S_OK;
}

HRESULT CMpeg2EncoderFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CMediaType& mt = m_pInput->CurrentMediaType();

	if(mt.subtype == MEDIASUBTYPE_IYUV)
	{
		pmt->majortype = MEDIATYPE_Video;
		pmt->subtype = MEDIASUBTYPE_MPEG2_VIDEO;
		pmt->formattype = FORMAT_MPEG2_VIDEO;

		BITMAPINFOHEADER bih;
		
		CMediaTypeEx(mt).ExtractBIH(&bih);

		MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)pmt->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO));

		memset(vih, 0, sizeof(MPEG2VIDEOINFO));

		vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
		vih->hdr.bmiHeader.biWidth = bih.biWidth;
		vih->hdr.bmiHeader.biHeight = bih.biHeight;
		vih->hdr.AvgTimePerFrame = ((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame;
		vih->hdr.dwPictAspectRatioX = 4;
		vih->hdr.dwPictAspectRatioY = 3;

		vih->dwLevel = 2;
		vih->dwProfile = 2;
	}
	else if(mt.subtype == MEDIASUBTYPE_PCM)
	{
		pmt->majortype = MEDIATYPE_Audio;
		pmt->subtype = MEDIASUBTYPE_DOLBY_AC3;
		pmt->formattype = FORMAT_WaveFormatEx;

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));

		memset(wfe, 0, sizeof(WAVEFORMATEX));

		wfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3;
		wfe->nChannels = ((WAVEFORMATEX*)mt.pbFormat)->nChannels;
		wfe->nSamplesPerSec = ((WAVEFORMATEX*)mt.pbFormat)->nSamplesPerSec;
		wfe->nBlockAlign = 1;
		wfe->cbSize = 0;
	}

	return S_OK;
}

