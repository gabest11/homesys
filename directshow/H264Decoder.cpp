#include "stdafx.h"
#include <vmr9.h>
#include <evr9.h>
#include <math.h>
#include "H264Decoder.h"
#include "DeCSSInputPin.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include "BitBlt.h"
#include <initguid.h>
#include "moreuuids.h"
#include "libavcodec.h"

// CH264DecoderFilter

CH264DecoderFilter::CH264DecoderFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseVideoFilter(NAME("CH264DecoderFilter"), lpunk, phr, __uuidof(this))
	, m_waitkf(30)
{
	delete m_pInput;

	if(FAILED(*phr)) return;

	m_pInput = new CDeCSSInputPin(NAME("CDeCSSInputPin"), this, phr, L"Video");

	if(FAILED(*phr)) return;

	m_libavcodec = new libavcodec();
}

CH264DecoderFilter::~CH264DecoderFilter()
{
	delete m_libavcodec;
}

void CH264DecoderFilter::OnDiscontinuity(bool transform)
{
	if(!transform)
	{
		m_libavcodec->Flush();
			
		m_waitkf = 30;
	}
}

STDMETHODIMP CH264DecoderFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CH264DecoderFilter::EndFlush()
{
	if(m_libavcodec != NULL)
	{
		m_libavcodec->Flush();
	}

	return __super::EndFlush();
}

HRESULT CH264DecoderFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	OnDiscontinuity(false);

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CH264DecoderFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	HRESULT hr;
	
	hr = __super::SetMediaType(dir, pmt);

	if(dir == PINDIR_INPUT)
	{
		const CMediaType& mt = m_pInput->CurrentMediaType();

		m_atpf = mt.GetAvgTimePerFrame(400000);

		if(mt.formattype == FORMAT_VideoInfo)
		{
			// avi?

			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;

			int nal_length_size = 0;
			const void* extradata = NULL;
			int extradata_size = 0;
			
			if(mt.cbFormat > sizeof(VIDEOINFOHEADER))
			{
				nal_length_size = 4;
				extradata = vih + 1;
				extradata_size = mt.cbFormat - sizeof(VIDEOINFOHEADER);
			}

			if(vih == NULL || !m_libavcodec->InitH264Decoder(vih->bmiHeader.biWidth, vih->bmiHeader.biHeight, nal_length_size, extradata, extradata_size)) 
			{
				return E_FAIL;
			}
		}
		else
		{
			VIDEOINFOHEADER2* vih = NULL;

			int nal_length_size = 0;
			const void* extradata = NULL;
			int extradata_size = 0;

			if(mt.formattype == FORMAT_MPEG2_VIDEO)
			{
				vih = &((MPEG2VIDEOINFO*)mt.pbFormat)->hdr;

				nal_length_size = ((MPEG2VIDEOINFO*)mt.pbFormat)->dwFlags;
				extradata = &((MPEG2VIDEOINFO*)mt.pbFormat)->dwSequenceHeader[0];
				extradata_size = ((MPEG2VIDEOINFO*)mt.pbFormat)->cbSequenceHeader;
			}
			else if(mt.formattype == FORMAT_VideoInfo2)
			{
				vih = (VIDEOINFOHEADER2*)mt.pbFormat;
			}

			if(vih == NULL || !m_libavcodec->InitH264Decoder(vih->bmiHeader.biWidth, vih->bmiHeader.biHeight, nal_length_size, extradata, extradata_size)) 
			{
				return E_FAIL;
			}
		}
	}

	return hr;
}

HRESULT CH264DecoderFilter::Transform(IMediaSample* pIn)
{
	HRESULT hr;

	BYTE* data = NULL;

	hr = pIn->GetPointer(&data);

	if(FAILED(hr))
	{
		return hr;
	}

	long len = pIn->GetActualDataLength();

	if(pIn->IsDiscontinuity() == S_OK)
	{
		OnDiscontinuity(true);
	}

	REFERENCE_TIME start;
	REFERENCE_TIME stop;

	if(FAILED(pIn->GetTime(&start, &stop)))
	{
		start = _I64_MIN;
		stop = _I64_MIN;
	}

// printf("vi: %I64d\n", start / 10000);

	const CMediaType& mt = m_pInput->CurrentMediaType();

	if(mt.subtype == MEDIASUBTYPE_H264 || mt.subtype == MEDIASUBTYPE_H264_TRANSPORT)
	{
		if(m_h264.size > (1 << 20) || m_h264.size > 0 && (start != _I64_MIN || pIn->IsDiscontinuity() == S_OK))
		{
			hr = DecodeH264(m_h264.buff, m_h264.size, m_h264.start, m_h264.stop);

			m_h264.size = 0;
			m_h264.start = _I64_MIN;

			if(FAILED(hr))
			{
				return hr;
			}
		}

		if(m_h264.size + len > m_h264.maxsize)
		{
			m_h264.maxsize = (m_h264.size + len) * 3 / 2;

			BYTE* tmp = new BYTE[m_h264.maxsize];

			memcpy(tmp, m_h264.buff, m_h264.size);

			delete [] m_h264.buff;

			m_h264.buff = tmp;
		}

		if(m_h264.size > 0 || start != _I64_MIN)
		{
			memcpy(m_h264.buff + m_h264.size, data, len);

			m_h264.size += len;
		}

		if(m_h264.start == _I64_MIN)
		{
			m_h264.start = start;
			m_h264.stop = stop;
		}

		return S_OK;
	}
	else if(mt.subtype == MEDIASUBTYPE_AVC1)
	{
		return DecodeH264(data, len, start, stop);
	}

	return E_UNEXPECTED;
}

#include "H264Nalu.h"

HRESULT CH264DecoderFilter::DecodeH264(BYTE* data, int len, REFERENCE_TIME start, REFERENCE_TIME stop)
{
	HRESULT hr = S_OK;

	if(start != _I64_MIN)
	{
		m_libavcodec->m_ctx->reordered_opaque = start;
		m_libavcodec->m_ctx->reordered_opaque2 = stop;
	}

	int i = 0;

	while(i < len)
	{
		int size = len - i;

		int	got_picture = 0;

		int	used_bytes = m_libavcodec->DecodeVideo(&got_picture, &data[i], size, start < 0 ? 1000 : m_late);

		if(used_bytes <= 0)
		{
			break;
		}

		if(used_bytes < size)
		{
			ASSERT(0);
		}

		if(got_picture && m_libavcodec->m_frame->data[0])
		{
			if(m_waitkf > 0)
			{
				m_waitkf--;

				if(m_libavcodec->m_frame->pict_type == FF_I_TYPE || m_libavcodec->m_frame->pict_type == FF_SI_TYPE)
				{
					m_waitkf = 0;
				}
			}

			start = m_libavcodec->m_frame->reordered_opaque;
			stop = m_libavcodec->m_frame->reordered_opaque2;

			stop = start + m_pInput->CurrentMediaType().GetAvgTimePerFrame(400000);

			if(m_waitkf == 0 && start >= 0)
			{
				int w = m_libavcodec->m_ctx->width;
				int h = m_libavcodec->m_ctx->height;
				int srcpitch = m_libavcodec->m_frame->linesize[0];
				BYTE** src = m_libavcodec->m_frame->data;

				//m_realsize.x = w;
				//m_realsize.y = h;

				if((m_libavcodec->m_ctx->sample_aspect_ratio.num > 0 && m_libavcodec->m_ctx->sample_aspect_ratio.den > 0)
				&& (m_libavcodec->m_ctx->sample_aspect_ratio.num > 1 || m_libavcodec->m_ctx->sample_aspect_ratio.den > 1))
				{
					Vector2i ar;
					
					ar.x = m_libavcodec->m_ctx->width * m_libavcodec->m_ctx->sample_aspect_ratio.num / m_libavcodec->m_ctx->sample_aspect_ratio.den;
					ar.y = m_libavcodec->m_ctx->height;

					SetAspectRatio(ar);
				}

				CComPtr<IMediaSample> pOut;
				BYTE* pDataOut = NULL;

				if(FAILED(hr = GetDeliveryBuffer(w, h, &pOut)) 
				|| FAILED(hr = pOut->GetPointer(&pDataOut)))
				{
					break;
				}

				pOut->SetTime(&start, &stop);
				// pOut->SetTime(NULL, NULL);
				
				// pOut->SetSyncPoint(TRUE);

				const CMediaType& mt = m_pOutput->CurrentMediaType();
				
				BITMAPINFOHEADER bihOut;

				CMediaTypeEx(mt).ExtractBIH(&bihOut);

				/*
				       weave   blend        bob
				 evr+i   p       p           i (weave)
				 evr+p   p       p (weave)   p (weave)
				!evr+p   p       p (weave)   p (weave)
				!evr+i   p       p           p (blend, TODO: sw bob)
				
				*/

				bool interlaced = m_libavcodec->m_frame->interlaced_frame != 0;
				bool blend = interlaced && !m_evr;
				bool weave = !(interlaced && m_evr);
				bool tff = m_libavcodec->m_frame->top_field_first != 0;
				bool rff = false; // ?

				//

				if(mt.subtype == MEDIASUBTYPE_I420 || mt.subtype == MEDIASUBTYPE_IYUV || mt.subtype == MEDIASUBTYPE_YV12)
				{
					int dstpitch = bihOut.biWidth;

					BYTE* y = pDataOut;
					BYTE* u = y + dstpitch * h;
					BYTE* v = y + dstpitch * h * 5 / 4;

					if(bihOut.biCompression == '21VY') {BYTE* tmp = u; u = v; v = tmp;}

					if(blend)
					{
						Image::DeinterlaceBlend(y, src[0], w, h, dstpitch, srcpitch);
						Image::DeinterlaceBlend(u, src[1], w / 2, h / 2, dstpitch / 2, srcpitch / 2);
						Image::DeinterlaceBlend(v, src[2], w / 2, h / 2, dstpitch / 2, srcpitch / 2);
					}
					else
					{
						Image::I420ToI420(w, h, y, u, v, dstpitch, src[0], src[1], src[2], srcpitch);
					}
				}
				else
				{
					if(m_fb.w != w || m_fb.h != h || m_fb.pitch != srcpitch)
					{
						m_fb.Init(w, h, srcpitch);
					}

					int dstpitch = m_fb.pitch;

					if(blend)
					{
						Image::DeinterlaceBlend(m_fb.buff[0], src[0], w, h, dstpitch, srcpitch);
						Image::DeinterlaceBlend(m_fb.buff[1], src[1], w / 2, h / 2, dstpitch / 2, srcpitch / 2);
						Image::DeinterlaceBlend(m_fb.buff[2], src[2], w / 2, h / 2, dstpitch / 2, srcpitch / 2);
					}
					else
					{
						Image::I420ToI420(w, h, m_fb.buff[0], m_fb.buff[1], m_fb.buff[2], dstpitch, src[0], src[1], src[2], srcpitch);
					}

					CopyBuffer(pDataOut, m_fb.buff, w, h, m_fb.pitch, MEDIASUBTYPE_I420, false);
				}

				SetInterlaceFlags(pOut, weave, tff, rff);

// printf("vo %I64d\n", start / 10000);

				hr = m_pOutput->Deliver(pOut);

				if(hr != S_OK)
				{
					break;
				}
			}
		}

		i += used_bytes;
	}

	return hr;
}

HRESULT CH264DecoderFilter::CheckInputType(const CMediaType* mtIn)
{
	if(mtIn->majortype == MEDIATYPE_Video && (mtIn->subtype == MEDIASUBTYPE_H264 || mtIn->subtype == MEDIASUBTYPE_H264_TRANSPORT))
	{
		return S_OK;
	}
	else if(mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_AVC1)
	{
		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CH264DecoderFilter::StartStreaming()
{
	HRESULT hr;
	
	hr = __super::StartStreaming();

	if(FAILED(hr)) 
	{
		return hr;
	}

	m_h264.size = 0;
	m_h264.start = _I64_MIN;

	OnDiscontinuity(false);

	return S_OK;
}

HRESULT CH264DecoderFilter::StopStreaming()
{
	return __super::StopStreaming();
}
