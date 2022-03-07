#include "stdafx.h"
#include <vmr9.h>
#include <evr9.h>
#include <math.h>
#include "MPEG2Decoder.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include "BitBlt.h"
#include <initguid.h>
#include "moreuuids.h"

// CMPEG2DecoderFilter

CMPEG2DecoderFilter::CMPEG2DecoderFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseVideoFilter(NAME("CMPEG2DecoderFilter"), lpunk, phr, __uuidof(this))
	, m_waitkf(30)
{
	delete m_pInput;

	if(FAILED(*phr)) return;

	m_pInput = new CMPEG2DecoderInputPin(this, phr, L"Video");

	if(FAILED(*phr)) return;

	m_pSubpicInput = new CSubpicInputPin(this, phr);

	if(FAILED(*phr)) return;

	m_pClosedCaptionOutput = new CClosedCaptionOutputPin(this, m_pLock, phr);

	if(FAILED(*phr)) return;

	SetDeinterlaceMethod(DIBob);
	EnableForcedSubtitles(true);
	EnablePlanarYUV(true);

	m_rate.Rate = 10000;
	m_rate.StartTime = 0;
}

CMPEG2DecoderFilter::~CMPEG2DecoderFilter()
{
	delete m_pSubpicInput;
	delete m_pClosedCaptionOutput;
}

void CMPEG2DecoderFilter::OnDiscontinuity(bool transform)
{
	const CMediaType& mt = m_pInput->CurrentMediaType();

	BYTE* pSequenceHeader = NULL;
	DWORD cbSequenceHeader = 0;

	if(mt.formattype == FORMAT_MPEGVideo)
	{
		pSequenceHeader = ((MPEG1VIDEOINFO*)mt.Format())->bSequenceHeader;
		cbSequenceHeader = ((MPEG1VIDEOINFO*)mt.Format())->cbSequenceHeader;
	}
	else if(mt.formattype == FORMAT_MPEG2_VIDEO)
	{
		pSequenceHeader = (BYTE*)((MPEG2VIDEOINFO*)mt.Format())->dwSequenceHeader;
		cbSequenceHeader = ((MPEG2VIDEOINFO*)mt.Format())->cbSequenceHeader;
	}

	for(int i = 0; i < countof(m_libmpeg2->m_pictures); i++)
	{
		m_libmpeg2->m_pictures[i].start = m_libmpeg2->m_pictures[i].stop = _I64_MIN + 1;
		m_libmpeg2->m_pictures[i].delivered = false;
		m_libmpeg2->m_pictures[i].flags &= ~PIC_MASK_CODING_TYPE;
	}

	m_libmpeg2->mpeg2_close();
	m_libmpeg2->mpeg2_init();
	m_libmpeg2->mpeg2_buffer(pSequenceHeader, pSequenceHeader + cbSequenceHeader);

	m_fb.flags = 0;

	m_waitkf = 1;

	m_film = false;
}

STDMETHODIMP CMPEG2DecoderFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMPEG2DecoderFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

int CMPEG2DecoderFilter::GetPinCount()
{
	return 4;
}

CBasePin* CMPEG2DecoderFilter::GetPin(int n)
{
	switch(n)
	{
	case 0: return m_pInput;
	case 1: return m_pOutput;
	case 2: return m_pSubpicInput;
	case 3: return m_pClosedCaptionOutput;
	}
	return NULL;
}

HRESULT CMPEG2DecoderFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	m_pClosedCaptionOutput->EndOfStream();

	return __super::EndOfStream();
}

HRESULT CMPEG2DecoderFilter::BeginFlush()
{
	m_pClosedCaptionOutput->DeliverBeginFlush();

	return __super::BeginFlush();
}

HRESULT CMPEG2DecoderFilter::EndFlush()
{
	m_pClosedCaptionOutput->DeliverEndFlush();

	return __super::EndFlush();
}

HRESULT CMPEG2DecoderFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	
	m_pClosedCaptionOutput->DeliverNewSegment(tStart, tStop, dRate);

	OnDiscontinuity(false);

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMPEG2DecoderFilter::Transform(IMediaSample* pIn)
{
	HRESULT hr;

	BYTE* data = NULL;

	hr = pIn->GetPointer(&data);

	if(FAILED(hr))
	{
		return hr;
	}

	long len = pIn->GetActualDataLength();

	((CDeCSSInputPin*)m_pInput)->StripPacket(data, len);

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

	const CMediaType& mt = m_pInput->CurrentMediaType();

	while(len >= 0)
	{
		mpeg2_state_t state = m_libmpeg2->mpeg2_parse();

		__asm emms; // this one is missing somewhere in the precompiled mmx obj files

		Vector2i ar;

		switch(state)
		{
		case STATE_BUFFER:
			if(len > 0) {m_libmpeg2->mpeg2_buffer(data, data + len); len = 0;}
			else len = -1;
			break;
		case STATE_INVALID:
			break;
		case STATE_GOP:
			m_pClosedCaptionOutput->Deliver(m_libmpeg2->m_info.m_user_data, m_libmpeg2->m_info.m_user_data_len);
			break;
		case STATE_SEQUENCE:
			m_atpf = m_libmpeg2->m_info.m_sequence->frame_period 
				? 10i64 * m_libmpeg2->m_info.m_sequence->frame_period / 27
				: ((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame;
			ar.x = m_libmpeg2->m_info.m_sequence->picture_width * m_libmpeg2->m_info.m_sequence->pixel_width / m_libmpeg2->m_info.m_sequence->pixel_height;
			ar.y = m_libmpeg2->m_info.m_sequence->picture_height;
			SetAspectRatio(ar);
			break;
		case STATE_PICTURE:
			m_libmpeg2->m_picture->start = start; start = _I64_MIN;
			m_libmpeg2->m_picture->delivered = false;
			m_libmpeg2->mpeg2_skip(m_late >= 100 && (m_libmpeg2->m_picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_B);
			break;
		case STATE_SLICE:
		case STATE_END:
			if((hr = DeliverPicture()) != S_OK) return hr;
			break;
		default:
			break;
		}
	}

	return S_OK;
}

HRESULT CMPEG2DecoderFilter::DeliverPicture()
{
	mpeg2_picture_t* picture = m_libmpeg2->m_info.m_display_picture;
	mpeg2_fbuf_t* fbuf = m_libmpeg2->m_info.m_display_fbuf;

	if(picture == NULL || (picture->flags & PIC_FLAG_SKIP) != 0 || fbuf == NULL)
	{
		return S_OK;
	}

	ASSERT(!picture->delivered);

	picture->delivered = true;

	// frame buffer

	int w = m_libmpeg2->m_info.m_sequence->picture_width;
	int h = m_libmpeg2->m_info.m_sequence->picture_height;
	int pitch = (m_libmpeg2->m_info.m_sequence->width + 31) & ~31;

	if(m_fb.w != w || m_fb.h != h || m_fb.pitch != pitch)
	{
		m_fb.Init(w, h, pitch);
	}

	// start - end

	m_fb.start = picture->start;
					
	if(m_fb.start == _I64_MIN) 
	{
		m_fb.start = m_fb.stop;
	}

	m_fb.stop = m_fb.start + m_atpf * picture->nb_fields / (m_libmpeg2->m_info.m_display_picture_2nd ? 1 : 2);

	//

	DWORD seqflags = m_libmpeg2->m_info.m_sequence->flags;
	DWORD oldflags = m_fb.flags;
	DWORD newflags = m_libmpeg2->m_info.m_display_picture->flags;

	m_fb.flags = newflags;

	if((seqflags & SEQ_FLAG_PROGRESSIVE_SEQUENCE) == 0
	&& (oldflags & PIC_FLAG_REPEAT_FIRST_FIELD) == 0
	&& (newflags & PIC_FLAG_PROGRESSIVE_FRAME) != 0)
	{
		if(!m_film && (newflags & PIC_FLAG_REPEAT_FIRST_FIELD) != 0)
		{
			m_film = true;
		}
		else if(m_film && (newflags & PIC_FLAG_REPEAT_FIRST_FIELD) == 0)
		{
			m_film = false;
		}
	}

	/*
			weave   blend        bob
	evr+i     p       p           i (weave)
	evr+p     p       p (weave)   p (weave)
	!evr+p    p       p (weave)   p (weave)
	!evr+i    p       p           p (sw bob)
				
	*/

	m_is.di = m_di;
	m_is.interlaced = !((seqflags & SEQ_FLAG_PROGRESSIVE_SEQUENCE) != 0 || m_film); // PIC_FLAG_PROGRESSIVE_FRAME isn't reliable
	m_is.weave = !(m_is.interlaced && m_is.di == DIBob && m_evr);
	m_is.tff = (newflags & PIC_FLAG_TOP_FIELD_FIRST) != 0;
	m_is.rff = (newflags & PIC_FLAG_REPEAT_FIRST_FIELD) != 0;

	if(!m_is.interlaced || m_evr && m_is.di == DIBob) 
	{
		m_is.di = DIWeave;
	}

	// printf("di=%d sidi=%d i=%d weave=%d evr=%d tff=%d rff=%d film=%d\n", m_di, m_is.di, m_is.interlaced, m_is.weave, m_evr, m_is.tff, m_is.rff, m_film);

	HRESULT hr;

	hr = DeliverFast();

	if(S_OK != hr)
	{
		hr = DeliverNormal();
	}

	return hr;
}

HRESULT CMPEG2DecoderFilter::DeliverFast()
{
	HRESULT hr;

	CAutoLock cAutoLock(&m_csReceive);

	mpeg2_fbuf_t* fbuf = m_libmpeg2->m_info.m_display_fbuf;

	if(fbuf == NULL) 
	{
		return S_FALSE;
	}

	{
		CAutoLock cAutoLock2(&m_csProps);

		if(m_dvd || m_pSubpicInput->HasAnythingToRender(m_fb.start))
		{
			return S_FALSE;
		}
	}

	if((m_fb.flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I)
	{
		m_waitkf = 0;
	}

	if(m_fb.start < 0 || m_waitkf > 0)
	{
		return S_OK;
	}

	const CMediaType& mt = m_pOutput->CurrentMediaType();
	
	if(mt.subtype != MEDIASUBTYPE_I420 && mt.subtype != MEDIASUBTYPE_IYUV && mt.subtype != MEDIASUBTYPE_YV12)
	{
		return S_FALSE;
	}

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;

	if(FAILED(hr = GetDeliveryBuffer(m_fb.w, m_fb.h, &pOut)) || FAILED(hr = pOut->GetPointer(&pDataOut)))
	{
		return hr;
	}

	if(mt.subtype != MEDIASUBTYPE_I420 && mt.subtype != MEDIASUBTYPE_IYUV && mt.subtype != MEDIASUBTYPE_YV12)
	{
		return S_FALSE;
	}

	BITMAPINFOHEADER bihOut;

	CMediaTypeEx(mt).ExtractBIH(&bihOut);

	int w = m_fb.w;
	int h = m_fb.h;
	int srcpitch = m_libmpeg2->m_info.m_sequence->width; // TODO (..+7)&~7; ?
	int dstpitch = bihOut.biWidth;

	BYTE* y = pDataOut;
	BYTE* u = y + dstpitch * h;
	BYTE* v = y + dstpitch * h * 5 / 4;

	if(bihOut.biCompression == '21VY') {BYTE* tmp = u; u = v; v = tmp;}

	if(m_is.di == DIWeave)
	{
		Image::I420ToI420(w, h, y, u, v, dstpitch, fbuf->buf[0], fbuf->buf[1], fbuf->buf[2], srcpitch);
	}
	else if(m_is.di == DIBlend)
	{
		Image::DeinterlaceBlend(y, fbuf->buf[0], w, h, dstpitch, srcpitch);
		Image::DeinterlaceBlend(u, fbuf->buf[1], w / 2, h / 2, dstpitch / 2, srcpitch / 2);
		Image::DeinterlaceBlend(v, fbuf->buf[2], w / 2, h / 2, dstpitch / 2, srcpitch / 2);
	}
	else // TODO: fast bob
	{
		return S_FALSE;
	}

	if(h == 1088)
	{
		memset(y + dstpitch * (h - 8), 0xff, dstpitch * 8);
		memset(u + dstpitch * (h - 8) / 4, 0x80, dstpitch * 8 / 4);
		memset(v + dstpitch * (h - 8) / 4, 0x80, dstpitch * 8 / 4);
	}

	if(CMPEG2DecoderInputPin* pPin = dynamic_cast<CMPEG2DecoderInputPin*>(m_pInput))
	{
		CAutoLock cAutoLock(&pPin->m_csRateLock);

		if(m_rate.Rate != pPin->m_ratechange.Rate)
		{
			m_rate.Rate = pPin->m_ratechange.Rate;
			m_rate.StartTime = m_fb.start;
		}
	}

	REFERENCE_TIME start = m_fb.start;
	REFERENCE_TIME stop = m_fb.stop;

	start = m_rate.StartTime + (start - m_rate.StartTime) * m_rate.Rate / 10000;
	stop = m_rate.StartTime + (stop - m_rate.StartTime) * m_rate.Rate / 10000;

	pOut->SetTime(&start, &stop);
	pOut->SetMediaTime(NULL, NULL);

	SetInterlaceFlags(pOut, m_is.weave, m_is.tff, m_is.rff);

	return m_pOutput->Deliver(pOut);
}

HRESULT CMPEG2DecoderFilter::DeliverNormal()
{
	HRESULT hr;

	CAutoLock cAutoLock(&m_csReceive);

	mpeg2_fbuf_t* fbuf = m_libmpeg2->m_info.m_display_fbuf;

	if(fbuf == NULL) 
	{
		return S_FALSE;
	}

	int w = m_fb.w;
	int h = m_fb.h;
	int spitch = m_libmpeg2->m_info.m_sequence->width; // TODO (..+7)&~7; ?
	int dpitch = m_fb.pitch;

	REFERENCE_TIME start = m_fb.start;
	REFERENCE_TIME stop = m_fb.stop;

	bool tff = (m_fb.flags & PIC_FLAG_TOP_FIELD_FIRST) != 0;

	// deinterlace

	if(m_is.di == DIWeave)
	{
		Image::I420ToI420(w, h, m_fb.buff[0], m_fb.buff[1], m_fb.buff[2], dpitch, fbuf->buf[0], fbuf->buf[1], fbuf->buf[2], spitch);
	}
	else if(m_is.di == DIBlend)
	{
		Image::DeinterlaceBlend(m_fb.buff[0], fbuf->buf[0], w, h, dpitch, spitch);
		Image::DeinterlaceBlend(m_fb.buff[1], fbuf->buf[1], w / 2, h / 2, dpitch / 2, spitch / 2);
		Image::DeinterlaceBlend(m_fb.buff[2], fbuf->buf[2], w / 2, h / 2, dpitch / 2, spitch / 2);
	}
	else if(m_is.di == DIBob)
	{
		Image::DeinterlaceBob(m_fb.buff[0], fbuf->buf[0], w, h, dpitch, spitch, tff);
		Image::DeinterlaceBob(m_fb.buff[1], fbuf->buf[1], w / 2, h / 2, dpitch / 2, spitch / 2, tff);
		Image::DeinterlaceBob(m_fb.buff[2], fbuf->buf[2], w / 2, h / 2, dpitch / 2, spitch / 2, tff);

		m_fb.start = start;
		m_fb.stop = (start + stop) / 2;

		/*
		if(CComQIPtr<IVMRDeinterlaceControl9> pDC = GetFilterFromPin(m_pOutput->GetConnected()))
		{
			const CMediaType& mt = m_pOutput->CurrentMediaType();

			BITMAPINFOHEADER bih;
			ExtractBIH(&mt, &bih);

			VMR9VideoDesc desc;
			memset(&desc, 0, sizeof(desc));
			desc.dwSize = sizeof(desc);
			desc.dwSampleWidth = bih.biWidth;
			desc.dwSampleHeight = abs(bih.biHeight);
			desc.dwFourCC = bih.biCompression;
			desc.InputSampleFreq.dwNumerator = 10000000;
			desc.InputSampleFreq.dwDenominator = ((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame;
			desc.OutputFrameFreq.dwNumerator = 20000000;
			desc.OutputFrameFreq.dwDenominator = ((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame;

			HRESULT hr;

			DWORD prefs = 0;
			hr = pDC->GetDeinterlacePrefs(&prefs);

			DWORD nModes = 0;
			hr = pDC->GetNumberOfDeinterlaceModes(&desc, &nModes, NULL);

			CAtlArray<GUID> guids;
			guids.SetCount(nModes);

			hr = pDC->GetNumberOfDeinterlaceModes(&desc, &nModes, &guids[0]);

			for(DWORD i = 0; i < nModes; i++)
			{
				VMR9DeinterlaceCaps caps;
				memset(&caps, 0, sizeof(caps));
				caps.dwSize = sizeof(caps);

				hr = pDC->GetDeinterlaceModeCaps(&guids[i], &desc, &caps);

				hr = hr;
			}
		}
		*/
	}

	// deliver

	hr = Deliver();

	if(FAILED(hr))
	{
		return hr;
	}

	if(m_is.di == DIBob)
	{
		Image::DeinterlaceBob(m_fb.buff[0], fbuf->buf[0], w, h, dpitch, spitch, !tff);
		Image::DeinterlaceBob(m_fb.buff[1], fbuf->buf[1], w / 2, h / 2, dpitch / 2, spitch / 2, !tff);
		Image::DeinterlaceBob(m_fb.buff[2], fbuf->buf[2], w / 2, h / 2, dpitch / 2, spitch / 2, !tff);

		m_fb.start = (start + stop) / 2;
		m_fb.stop = stop;

		// deliver

		hr = Deliver();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	return S_OK;
}

HRESULT CMPEG2DecoderFilter::Deliver()
{
	CAutoLock cAutoLock(&m_csReceive);

	if((m_fb.flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I)
	{
		m_waitkf = 0;
	}

	if(m_fb.start < 0 || m_waitkf > 0)
	{
		return S_OK;
	}

	Vector2i size(m_fb.w, m_fb.h);

	CorrectInputSize(size);

	m_fb.w = size.x;
	m_fb.h = size.y;

	HRESULT hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;

	if(FAILED(hr = GetDeliveryBuffer(m_fb.w, m_fb.h, &pOut)) || FAILED(hr = pOut->GetPointer(&pDataOut)))
	{
		return hr;
	}

	if(m_fb.h == 1088)
	{
		memset(m_fb.buff[0] + m_fb.w * (m_fb.h - 8), 0xff, m_fb.pitch * 8);
		memset(m_fb.buff[1] + m_fb.w * (m_fb.h - 8) / 4, 0x80, m_fb.pitch * 8 / 4);
		memset(m_fb.buff[2] + m_fb.w * (m_fb.h - 8) / 4, 0x80, m_fb.pitch * 8 / 4);
	}

	BYTE** buf = &m_fb.buff[0];

	if(m_pSubpicInput->HasAnythingToRender(m_fb.start))
	{
		Image::I420ToI420(m_fb.w, m_fb.h, 
			m_fb.buff[3], m_fb.buff[4], m_fb.buff[5], m_fb.pitch, 
			m_fb.buff[0], m_fb.buff[1], m_fb.buff[2], m_fb.pitch);

		buf = &m_fb.buff[3];

		m_pSubpicInput->RenderSubpics(m_fb.start, buf, m_fb.pitch, m_fb.h);
	}

	CopyBuffer(pDataOut, buf, (m_fb.w + 7) & ~7, m_fb.h, m_fb.pitch, MEDIASUBTYPE_I420, (m_fb.flags & PIC_FLAG_PROGRESSIVE_FRAME) == 0);

	//

	if(CMPEG2DecoderInputPin* pPin = dynamic_cast<CMPEG2DecoderInputPin*>(m_pInput))
	{
		CAutoLock cAutoLock(&pPin->m_csRateLock);

		if(m_rate.Rate != pPin->m_ratechange.Rate)
		{
			m_rate.Rate = pPin->m_ratechange.Rate;
			m_rate.StartTime = m_fb.start;
		}
	}

	REFERENCE_TIME start = m_fb.start;
	REFERENCE_TIME stop = m_fb.stop;

	start = m_rate.StartTime + (start - m_rate.StartTime) * m_rate.Rate / 10000;
	stop = m_rate.StartTime + (stop - m_rate.StartTime) * m_rate.Rate / 10000;

	pOut->SetTime(&start, &stop);
	pOut->SetMediaTime(NULL, NULL);

	SetInterlaceFlags(pOut, m_is.weave, m_is.tff, m_is.rff);

	hr = m_pOutput->Deliver(pOut);

	return hr;
}

#include "IFilterVersion.h"

HRESULT CMPEG2DecoderFilter::CheckConnect(PIN_DIRECTION dir, IPin* pPin)
{
	if(dir == PINDIR_OUTPUT)
	{
		if(DirectShow::GetCLSID(m_pInput->GetConnected()) == CLSID_DVDNavigator)
		{
			// one of these needed for dynamic format changes

			CLSID clsid = DirectShow::GetCLSID(pPin);

			DWORD ver = 0;

			if(CComQIPtr<IFilterVersion> pFV = DirectShow::GetFilter(pPin))
			{
				ver = pFV->GetFilterVersion();
			}

			CLSID ffdshow, dvobsub, dvobsub_auto;

			Util::ToCLSID(L"{04FE9017-F873-410E-871E-AB91661A4EF7}", ffdshow);
			Util::ToCLSID(L"{93A22E7A-5091-45ef-BA61-6DA26156A5D0}", dvobsub);
			Util::ToCLSID(L"{9852A670-F845-491b-9BE6-EBD841B8A613}", dvobsub_auto);

			if(clsid != CLSID_OverlayMixer
			/*&& clsid != CLSID_OverlayMixer2*/
			&& clsid != CLSID_VideoMixingRenderer 
			&& clsid != CLSID_VideoMixingRenderer9
			&& clsid != CLSID_EnhancedVideoRenderer
			&& clsid != ffdshow
			&& (clsid != dvobsub || ver < 0x0234)
			&& (clsid != dvobsub_auto || ver < 0x0234)
			&& clsid != CLSID_DXR) // Haali's video renderer
			{
				return E_FAIL;
			}
		}
	}

	return __super::CheckConnect(dir, pPin);
}

HRESULT CMPEG2DecoderFilter::CheckInputType(const CMediaType* mtIn)
{
	if(mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
	|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
	|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
	|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
	|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG1Packet
	|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG1Payload)
	{
		if(mtIn->formattype == FORMAT_MPEG2_VIDEO && mtIn->pbFormat)
		{
			MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mtIn->pbFormat;

			if(vih->cbSequenceHeader > 0 && (vih->dwSequenceHeader[0] & 0x00ffffff) != 0x00010000)
			{
				return VFW_E_TYPE_NOT_ACCEPTED;
			}
		}

		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMPEG2DecoderFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	HRESULT hr;

	hr = __super::CheckTransform(mtIn, mtOut);

	if(FAILED(hr))
	{
		return hr;
	}

	bool b = mtOut->subtype == MEDIASUBTYPE_YV12 || mtOut->subtype == MEDIASUBTYPE_I420 || mtOut->subtype == MEDIASUBTYPE_IYUV;

	return !b || IsPlanarYUVEnabled() ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMPEG2DecoderFilter::StartStreaming()
{
	HRESULT hr;
	
	hr = __super::StartStreaming();

	if(FAILED(hr)) 
	{
		return hr;
	}

	m_libmpeg2 = new CLibMPEG2();

	OnDiscontinuity(false);

	return S_OK;
}

HRESULT CMPEG2DecoderFilter::StopStreaming()
{
	delete m_libmpeg2;

	m_libmpeg2 = NULL;

	return __super::StopStreaming();
}

// IMPEG2DecoderFilter

STDMETHODIMP CMPEG2DecoderFilter::SetDeinterlaceMethod(DeinterlaceMethod di)
{
	CAutoLock cAutoLock(&m_csProps);
	
	m_di = di;

	return S_OK;
}

STDMETHODIMP_(IMPEG2DecoderFilter::DeinterlaceMethod) CMPEG2DecoderFilter::GetDeinterlaceMethod()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_di;
}

STDMETHODIMP CMPEG2DecoderFilter::EnableForcedSubtitles(bool enabled)
{
	CAutoLock cAutoLock(&m_csProps);

	m_forcedsubs = enabled;

	return S_OK;
}

STDMETHODIMP_(bool) CMPEG2DecoderFilter::IsForcedSubtitlesEnabled()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_forcedsubs;
}

STDMETHODIMP CMPEG2DecoderFilter::EnablePlanarYUV(bool enabled)
{
	CAutoLock cAutoLock(&m_csProps);
	
	m_planarYUV = enabled;

	return S_OK;
}

STDMETHODIMP_(bool) CMPEG2DecoderFilter::IsPlanarYUVEnabled()
{
	CAutoLock cAutoLock(&m_csProps);
	
	return m_planarYUV;
}

// CMPEG2DecoderInputPin

CMPEG2DecoderInputPin::CMPEG2DecoderInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CDeCSSInputPin(NAME("CMPEG2DecoderInputPin"), pFilter, phr, pName)
{
	m_CorrectTS = 0;
	m_ratechange.Rate = 10000;
	m_ratechange.StartTime = _I64_MAX;
}

// IKsPropertySet

STDMETHODIMP CMPEG2DecoderInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
	{
		return __super::Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);
	}

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	{
		switch(Id)
		{
		case AM_RATE_SimpleRateChange:
			{
				AM_SimpleRateChange* p = (AM_SimpleRateChange*)pPropertyData;

				//if(!m_CorrectTS) return E_PROP_ID_UNSUPPORTED;

				CAutoLock cAutoLock(&m_csRateLock);

				m_ratechange = *p;
			}
			break;
		case AM_RATE_UseRateVersion:
			if(*(WORD*)pPropertyData > 0x0101) 
			{
				return E_PROP_ID_UNSUPPORTED;
			}
			break;
		case AM_RATE_CorrectTS:
			m_CorrectTS = *(LONG*)pPropertyData;
			break;
		default:
			return E_PROP_ID_UNSUPPORTED;
		}
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	{
		switch(Id)
		{
		case AM_RATE_ChangeRate:
			{
				AM_DVD_ChangeRate* p = (AM_DVD_ChangeRate*)pPropertyData;
			}
			break;
		default:
			return E_PROP_ID_UNSUPPORTED;
		}
	}
*/
	return S_OK;
}

STDMETHODIMP CMPEG2DecoderInputPin::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
	{
		return __super::Get(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength, pBytesReturned);
	}

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	{
		switch(Id)
		{
		case AM_RATE_SimpleRateChange:
			{
				AM_SimpleRateChange* p = (AM_SimpleRateChange*)pPropertyData;
				return E_PROP_ID_UNSUPPORTED;
			}
			break;
		case AM_RATE_MaxFullDataRate:
			{
				AM_MaxFullDataRate* p = (AM_MaxFullDataRate*)pPropertyData;
				*p = 8 * 10000;
				*pBytesReturned = sizeof(AM_MaxFullDataRate);
			}
			break;
		case AM_RATE_QueryFullFrameRate:
			{
				AM_QueryRate* p = (AM_QueryRate*)pPropertyData;
				p->lMaxForwardFullFrame = 8 * 10000;
				p->lMaxReverseFullFrame = 8 * 10000;
				*pBytesReturned = sizeof(AM_QueryRate);
			}
			break;
		case AM_RATE_QueryLastRateSegPTS:
			{
				REFERENCE_TIME* p = (REFERENCE_TIME*)pPropertyData;
				return E_PROP_ID_UNSUPPORTED;
			}
			break;
		default:
			return E_PROP_ID_UNSUPPORTED;
		}
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	{
		switch(Id)
		{
		case AM_RATE_FullDataRateMax:
			{
				AM_MaxFullDataRate* p = (AM_MaxFullDataRate*)pPropertyData;
			}
			break;
		case AM_RATE_ReverseDecode:
			{
				LONG* p = (LONG*)pPropertyData;
			}
			break;
		case AM_RATE_DecoderPosition:
			{
				AM_DVD_DecoderPosition* p = (AM_DVD_DecoderPosition*)pPropertyData;
			}
			break;
		case AM_RATE_DecoderVersion:
			{
				LONG* p = (LONG*)pPropertyData;
			}
			break;
		default:
			return E_PROP_ID_UNSUPPORTED;
		}
	}
*/
	return S_OK;
}

STDMETHODIMP CMPEG2DecoderInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
	{
		return __super::QuerySupported(PropSet, Id, pTypeSupport);
	}

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	{
		switch(Id)
		{
		case AM_RATE_SimpleRateChange:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
			break;
		case AM_RATE_MaxFullDataRate:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		case AM_RATE_UseRateVersion:
			*pTypeSupport = KSPROPERTY_SUPPORT_SET;
			break;
		case AM_RATE_QueryFullFrameRate:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		case AM_RATE_QueryLastRateSegPTS:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		case AM_RATE_CorrectTS:
			*pTypeSupport = KSPROPERTY_SUPPORT_SET;
			break;
		default:
			return E_PROP_ID_UNSUPPORTED;
		}
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	{
		switch(Id)
		{
		case AM_RATE_ChangeRate:
			*pTypeSupport = KSPROPERTY_SUPPORT_SET;
			break;
		case AM_RATE_FullDataRateMax:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		case AM_RATE_ReverseDecode:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		case AM_RATE_DecoderPosition:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		case AM_RATE_DecoderVersion:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		default:
			return E_PROP_ID_UNSUPPORTED;
		}
	}
*/
	return S_OK;
}

// CSubpicInputPin

#define PTS2RT(pts) (10000i64 * (pts) / 90)

CSubpicInputPin::CSubpicInputPin(CTransformFilter* pFilter, HRESULT* phr) 
	: CMPEG2DecoderInputPin(pFilter, phr, L"SubPicture")
	, m_spon(TRUE)
	, m_fsppal(false)
	, m_sphli(NULL)
{
	m_sppal[0].Y = 0x00;
	m_sppal[0].U = m_sppal[0].V = 0x80;
	m_sppal[1].Y = 0xe0;
	m_sppal[1].U = m_sppal[1].V = 0x80;
	m_sppal[2].Y = 0x80;
	m_sppal[2].U = m_sppal[2].V = 0x80;
	m_sppal[3].Y = 0x20;
	m_sppal[3].U = m_sppal[3].V = 0x80;
}

CSubpicInputPin::~CSubpicInputPin()
{
	for(auto i = m_sps.begin(); i != m_sps.end(); i++)
	{
		delete *i;
	}

	delete m_sphli;
}

HRESULT CSubpicInputPin::CheckMediaType(const CMediaType* mtIn)
{
	return (mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES
			|| mtIn->majortype == MEDIATYPE_Video) 
		&& (mtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE
			|| mtIn->subtype == MEDIASUBTYPE_CVD_SUBPICTURE
			|| mtIn->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CSubpicInputPin::SetMediaType(const CMediaType* mtIn)
{
	return CBasePin::SetMediaType(mtIn);
}

bool CSubpicInputPin::HasAnythingToRender(REFERENCE_TIME rt)
{
	if(!IsConnected()) return false;

	CAutoLock cAutoLock(&m_csReceive);

	for(auto i = m_sps.begin(); i != m_sps.end(); i++)
	{
		spu* sp = *i;

		if(sp->m_start <= rt && rt < sp->m_stop && (/*sp->m_psphli ||*/ sp->m_forced || m_spon))
		{
			return true;
		}
	}

	return false;
}

void CSubpicInputPin::RenderSubpics(REFERENCE_TIME rt, BYTE** yuv, int w, int h)
{
	CAutoLock cAutoLock(&m_csReceive);

	CMPEG2DecoderFilter* f = (CMPEG2DecoderFilter*)m_pFilter;

	for(auto i = m_sps.begin(); i != m_sps.end(); )
	{
		auto j = i++;

		spu* sp = *j;

		if(sp->m_stop <= rt)
		{
			delete sp;

			m_sps.erase(j);
		}
		else if(sp->m_start <= rt)
		{
			if(m_spon || sp->m_forced && (f->IsForcedSubtitlesEnabled() || sp->m_psphli))
			{
				sp->Render(rt, yuv, w, h, m_sppal, m_fsppal);
			}
		}
	}
}

HRESULT CSubpicInputPin::Transform(IMediaSample* pSample)
{
	HRESULT hr;

	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt != NULL)
	{
		CMediaType mt(*pmt);
		SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	BYTE* pDataIn = NULL;

	hr = pSample->GetPointer(&pDataIn);

	if(FAILED(hr)) 
	{
		return hr;
	}

	long len = pSample->GetActualDataLength();

	StripPacket(pDataIn, len);

	if(len <= 0) 
	{
		return S_FALSE;
	}

	if(m_mt.subtype == MEDIASUBTYPE_SVCD_SUBPICTURE)
	{
		pDataIn += 4;
		len -= 4;
	}

	if(len <= 0) 
	{
		return S_FALSE;
	}

	CAutoLock cAutoLock(&m_csReceive);

	REFERENCE_TIME start = 0;
	REFERENCE_TIME stop = 0;

	hr = pSample->GetTime(&start, &stop);

	bool refresh = false;

	if(FAILED(hr))
	{
		if(!m_sps.empty())
		{
			spu* sp = m_sps.back();
			int size = sp->m_buff.size();
			sp->m_buff.resize(size + len);
			memcpy(&sp->m_buff[size], pDataIn, len);
		}
	}
	else
	{
		for(auto i = m_sps.rbegin(); i != m_sps.rend(); i++)
		{
			spu* sp = *i;

			if(sp->m_stop == _I64_MAX)
			{
				sp->m_stop = start;

				break;
			}
		}

		spu* p = NULL;

		if(m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE) p = new dvdspu();
		else if(m_mt.subtype == MEDIASUBTYPE_CVD_SUBPICTURE) p = new cvdspu();
		else if(m_mt.subtype == MEDIASUBTYPE_SVCD_SUBPICTURE) p = new svcdspu();
		else return E_FAIL;

		p->m_start = start;
		p->m_stop = _I64_MAX;

		p->m_buff.resize(len);

		memcpy(&p->m_buff[0], pDataIn, len);

		if(m_sphli && p->m_start == PTS2RT(m_sphli->StartPTM))
		{
			p->m_psphli = m_sphli;

			m_sphli = NULL;

			refresh = true;
		}

		m_sps.push_back(p);
	}

	if(!m_sps.empty())
	{
		m_sps.back()->Parse();
	}

	if(refresh)
	{
//		((CMPEG2DecoderFilter*)m_pFilter)->Deliver(true);
	}

	return S_FALSE;
}

STDMETHODIMP CSubpicInputPin::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);

	for(auto i = m_sps.begin(); i != m_sps.end(); i++)
	{
		delete *i;
	}

	m_sps.clear();

	return S_OK;
}

// IKsPropertySet

STDMETHODIMP CSubpicInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_DvdSubPic)
	{
		return __super::Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);
	}

	bool refresh = false;

	switch(Id)
	{
	case AM_PROPERTY_DVDSUBPIC_PALETTE:
		{
			CAutoLock cAutoLock(&m_csReceive);

			AM_PROPERTY_SPPAL* pSPPAL = (AM_PROPERTY_SPPAL*)pPropertyData;
			memcpy(m_sppal, pSPPAL->sppal, sizeof(AM_PROPERTY_SPPAL));
			m_fsppal = true;

			DbgLog((LOG_TRACE, 0, _T("new palette")));
		}
		break;
	case AM_PROPERTY_DVDSUBPIC_HLI:
		{
			CAutoLock cAutoLock(&m_csReceive);

			AM_PROPERTY_SPHLI* pSPHLI = (AM_PROPERTY_SPHLI*)pPropertyData;

			delete m_sphli;

			m_sphli = NULL;

			if(pSPHLI->HLISS)
			{
				for(auto i = m_sps.begin(); i != m_sps.end(); i++)
				{
					spu* sp = *i;

					if(sp->m_start <= PTS2RT(pSPHLI->StartPTM) && PTS2RT(pSPHLI->StartPTM) < sp->m_stop)
					{
						refresh = true;
						delete sp->m_psphli;
						sp->m_psphli = new AM_PROPERTY_SPHLI();
						memcpy(sp->m_psphli, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
					}
				}

				if(!refresh) // save it for later, a subpic might be late for this hli
				{
					m_sphli = new AM_PROPERTY_SPHLI();
					memcpy(m_sphli, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
				}
			}
			else
			{
				for(auto i = m_sps.begin(); i != m_sps.end(); i++)
				{
					spu* sp = *i;
					refresh |= sp->m_psphli != NULL;
					delete sp->m_psphli;
					sp->m_psphli = NULL;
				}
			}
		}
		break;
	case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
		{
			CAutoLock cAutoLock(&m_csReceive);

			m_spon = *(AM_PROPERTY_COMPOSIT_ON*)pPropertyData;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}

	if(refresh)
	{
		((CMPEG2DecoderFilter*)m_pFilter)->Deliver();
	}

	return S_OK;
}

STDMETHODIMP CSubpicInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_DvdSubPic)
	{
		return __super::QuerySupported(PropSet, Id, pTypeSupport);
	}

	switch(Id)
	{
	case AM_PROPERTY_DVDSUBPIC_PALETTE:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDSUBPIC_HLI:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
	
	return S_OK;
}

// CSubpicInputPin::spu

static __inline BYTE GetNibble(BYTE* p, DWORD* offset, int& nField, int& fAligned)
{
	BYTE ret = (p[offset[nField]] >> (fAligned << 2)) & 0x0f;
	offset[nField] += 1 - fAligned;
	fAligned = !fAligned;
    return ret;
}

static __inline BYTE GetHalfNibble(BYTE* p, DWORD* offset, int& nField, int& n)
{
	BYTE ret = (p[offset[nField]] >> (n << 1)) & 0x03;
	if(!n) offset[nField]++;
	n = (n - 1 + 4) & 3;
    return ret;
}

static __inline void DrawPixel(BYTE** yuv, Vector2i pt, int pitch, AM_DVD_YUV& c)
{
	if(c.Reserved == 0) return;

	BYTE* p = &yuv[0][pt.y * pitch + pt.x];
//	*p = (*p * (15 - contrast) + sppal[color].Y * contrast) >> 4;
	*p -= (*p - c.Y) * c.Reserved >> 4;

	if(pt.y & 1) return; // since U/V is half res there is no need to overwrite the same line again

	pt.x = (pt.x + 1) / 2;
	pt.y = (pt.y /*+ 1*/) / 2; // only paint the upper field always, don't round it
	pitch /= 2;

	// U/V is exchanged? wierd but looks true when comparing it to the colors of other decoders

	p = &yuv[1][pt.y * pitch + pt.x];
//	*p = (BYTE)(((((int)*p - 0x80) * (15 - contrast) + ((int)sppal[color].V - 0x80) * contrast) >> 4) + 0x80);
	*p -= (*p - c.V) * c.Reserved >> 4;

	p = &yuv[2][pt.y * pitch + pt.x];
//	*p = (BYTE)(((((int)*p - 0x80) * (15 - contrast) + ((int)sppal[color].U - 0x80) * contrast) >> 4) + 0x80);
	*p -= (*p - c.U) * c.Reserved >> 4;

	// Neighter of the blending formulas are accurate (">>4" should be "/15").
	// Even though the second one is a bit worse, since we are scaling the difference only,
	// the error is still not noticable.
}

static __inline void DrawPixels(BYTE** yuv, int pitch, Vector2i pt, int len, AM_DVD_YUV& c, const Vector4i& rc)
{
    if(pt.y < rc.top || pt.y >= rc.bottom) return;
	if(pt.x < rc.left) {len -= rc.left - pt.x; pt.x = rc.left;}
	if(pt.x + len > rc.right) len = rc.right - pt.x;
	if(len <= 0 || pt.x >= rc.right) return;

	if(c.Reserved == 0)
	{
		if(rc.rempty())
		{
			return;
		}
	}

	while(len-- > 0)
	{
		DrawPixel(yuv, pt, pitch, c);

		pt.x++;
	}
}

// CSubpicInputPin::dvdspu

bool CSubpicInputPin::dvdspu::Parse()
{
	m_offsets.clear();

	BYTE* p = &m_buff[0];

	WORD packetsize = (p[0] << 8) | p[1];
	WORD datasize = (p[2] << 8) | p[3];

    if(packetsize > m_buff.size() || datasize > packetsize)
	{
		return false;
	}

	int i, next = datasize;

	#define GetWORD (p[i] << 8) | p[i + 1]; i += 2

	do
	{
		i = next;

		int pts = GetWORD;

		next = GetWORD;

		if(next > packetsize || next < datasize)
		{
			return false;
		}

		REFERENCE_TIME rt = m_start + 1024 * PTS2RT(pts);

		for(bool fBreak = false; !fBreak; )
		{
			int len = 0;

			switch(p[i])
			{
				case 0x00: len = 0; break;
				case 0x01: len = 0; break;
				case 0x02: len = 0; break;
				case 0x03: len = 2; break;
				case 0x04: len = 2; break;
				case 0x05: len = 6; break;
				case 0x06: len = 4; break;
				case 0x07: len = 0; break; // TODO
				default: len = 0; break;
			}

			if(i + len >= packetsize)
			{
				break;
			}

			switch(p[i++])
			{
				case 0x00: // forced start displaying
					m_forced = true;
					break;
				case 0x01: // normal start displaying
					m_forced = false;
					break;
				case 0x02: // stop displaying
					m_stop = rt;
					break;
				case 0x03:
					m_sphli.ColCon.emph2col = p[i] >> 4;
					m_sphli.ColCon.emph1col = p[i] & 0xf;
					m_sphli.ColCon.patcol = p[i + 1] >> 4;
					m_sphli.ColCon.backcol = p[i + 1] & 0xf;
					i += 2;
					break;
				case 0x04:
					m_sphli.ColCon.emph2con = p[i] >> 4;
					m_sphli.ColCon.emph1con = p[i] & 0xf;
					m_sphli.ColCon.patcon = p[i + 1] >> 4;
					m_sphli.ColCon.backcon = p[i + 1] & 0xf;
					i += 2;
					break;
				case 0x05:
					m_sphli.StartX = (p[i] << 4) + (p[i + 1] >> 4);
					m_sphli.StopX = ((p[i + 1] & 0x0f) << 8) + p[i + 2] + 1;
					m_sphli.StartY = (p[i + 3] << 4) + (p[i + 4] >> 4);
					m_sphli.StopY = ((p[i + 4] & 0x0f) << 8) + p[i + 5] + 1;
					i += 6;
					break;
				case 0x06:
					m_offset[0] = GetWORD;
					m_offset[1] = GetWORD;
					break;
				case 0x07:
					// TODO
					break;
				case 0xff: // end of ctrlblk
					fBreak = true;
					continue;
				default: // skip this ctrlblk
					fBreak = true;
					break;
			}
		}

		offset_t o = {rt, m_sphli};

		m_offsets.push_back(o); // is it always going to be sorted?
	}
	while(i <= next && i < packetsize);

	return true;
}

void CSubpicInputPin::dvdspu::Render(REFERENCE_TIME rt, BYTE** yuv, int w, int h, AM_DVD_YUV* sppal, bool fsppal)
{
	BYTE* p = &m_buff[0];

	DWORD offset[2] = {m_offset[0], m_offset[1]};

	AM_PROPERTY_SPHLI sphli = m_sphli;

	Vector2i pt(sphli.StartX, sphli.StartY);
	Vector4i rc(pt.x, pt.y, sphli.StopX, sphli.StopY);
	Vector4i rcclip = Vector4i(0, 0, w, h).rintersect(rc);

	if(m_psphli)
	{
		Vector4i r(m_psphli->StartX, m_psphli->StartY, m_psphli->StopX, m_psphli->StopY);
		
		rcclip = rcclip.rintersect(r);
		
		sphli = *m_psphli;
	}
	else if(m_offsets.size() > 1)
	{
		for(auto i = m_offsets.begin(); i != m_offsets.end(); i++)
		{
			if(rt >= i->rt) 
			{
				sphli = i->sphli; 
				
				break;
			}
		}
	}

	AM_DVD_YUV pal[4];

	pal[0] = sppal[fsppal ? sphli.ColCon.backcol : 0];
	pal[0].Reserved = sphli.ColCon.backcon;
	pal[1] = sppal[fsppal ? sphli.ColCon.patcol : 1];
	pal[1].Reserved = sphli.ColCon.patcon;
	pal[2] = sppal[fsppal ? sphli.ColCon.emph1col : 2];
	pal[2].Reserved = sphli.ColCon.emph1con;
	pal[3] = sppal[fsppal ? sphli.ColCon.emph2col : 3];
	pal[3].Reserved = sphli.ColCon.emph2con;

	int nField = 0;
	int fAligned = 1;

	DWORD end[2] = {offset[1], (p[2] << 8) | p[3]};

	while(offset[nField] < end[nField])
	{
		DWORD code;

		if((code = GetNibble(p, offset, nField, fAligned)) >= 0x4
		|| (code = (code << 4) | GetNibble(p, offset, nField, fAligned)) >= 0x10
		|| (code = (code << 4) | GetNibble(p, offset, nField, fAligned)) >= 0x40
		|| (code = (code << 4) | GetNibble(p, offset, nField, fAligned)) >= 0x100)
		{
			DrawPixels(yuv, w, pt, code >> 2, pal[code & 3], rcclip);

			if((pt.x += code >> 2) < rc.right) 
			{
				continue;
			}
		}

		DrawPixels(yuv, w, pt, rc.right - pt.x, pal[code & 3], rcclip);

		if(!fAligned) GetNibble(p, offset, nField, fAligned); // align to byte

		pt.x = rc.left;
		pt.y++;

		nField = 1 - nField;
	}
}

// CSubpicInputPin::cvdspu

bool CSubpicInputPin::cvdspu::Parse()
{
	BYTE* p = &m_buff[0];

	WORD packetsize = (p[0] << 8) | p[1];
	WORD datasize = (p[2] << 8) | p[3];

    if(packetsize > m_buff.size() || datasize > packetsize)
	{
		return false;
	}

	p = &m_buff[datasize];

	for(int i = datasize, j = packetsize - 4; i <= j; i += 4, p += 4)
	{
		switch(p[0])
		{
		case 0x0c: 
			break;
		case 0x04: 
			m_stop = m_start + 10000i64 * ((p[1] << 16) | (p[2] << 8) | p[3]) / 90;
			break;
		case 0x17: 
			m_sphli.StartX = ((p[1] & 0x0f) << 6) + (p[2] >> 2);
			m_sphli.StartY = ((p[2] & 0x03) << 8) + p[3];
			break;
		case 0x1f: 
			m_sphli.StopX = ((p[1] & 0x0f) << 6) + (p[2] >> 2);
			m_sphli.StopY = ((p[2] & 0x03) << 8) + p[3];
			break;
		case 0x24: case 0x25: case 0x26: case 0x27: 
			m_sppal[0][p[0] - 0x24].Y = p[1];
			m_sppal[0][p[0] - 0x24].U = p[2];
			m_sppal[0][p[0] - 0x24].V = p[3];
			break;
		case 0x2c: case 0x2d: case 0x2e: case 0x2f: 
			m_sppal[1][p[0] - 0x2c].Y = p[1];
			m_sppal[1][p[0] - 0x2c].U = p[2];
			m_sppal[1][p[0] - 0x2c].V = p[3];
			break;
		case 0x37: 
			m_sppal[0][3].Reserved = p[2] >> 4;
			m_sppal[0][2].Reserved = p[2] & 0xf;
			m_sppal[0][1].Reserved = p[3] >> 4;
			m_sppal[0][0].Reserved = p[3] & 0xf;
			break;
		case 0x3f: 
			m_sppal[1][3].Reserved = p[2] >> 4;
			m_sppal[1][2].Reserved = p[2] & 0xf;
			m_sppal[1][1].Reserved = p[3] >> 4;
			m_sppal[1][0].Reserved = p[3] & 0xf;
			break;
		case 0x47: 
			m_offset[0] = (p[2] << 8) | p[3];
			break;
		case 0x4f: 
			m_offset[1] = (p[2] << 8) | p[3];
			break;
		default: 
			break;
		}
	}

	return(true);
}

void CSubpicInputPin::cvdspu::Render(REFERENCE_TIME rt, BYTE** yuv, int w, int h, AM_DVD_YUV* sppal, bool fsppal)
{
	BYTE* p = &m_buff[0];

	DWORD offset[2] = {m_offset[0], m_offset[1]};

	Vector4i rcclip(0, 0, w, h);

	/* FIXME: startx/y looks to be wrong in the sample I tested
	CPoint pt(m_sphli.StartX, m_sphli.StartY);
	CRect rc(pt, CPoint(m_sphli.StopX, m_sphli.StopY));
	*/

	Vector2i size(m_sphli.StopX - m_sphli.StartX, m_sphli.StopY - m_sphli.StartY);
	Vector2i pt((rcclip.width() - size.x) / 2, (rcclip.height() * 3 - size.y * 1) / 4);
	Vector4i rc(pt.x, pt.y, pt.x + size.x, pt.y + size.y);

	int nField = 0;
	int fAligned = 1;

	DWORD end[2] = {offset[1], (p[2] << 8) | p[3]};

	while((nField == 0 && offset[0] < end[0]) || (nField == 1 && offset[1] < end[1]))
	{
		BYTE code;

		if((code = GetNibble(p, offset, nField, fAligned)) >= 0x4)
		{
			DrawPixels(yuv, w, pt, code >> 2, m_sppal[0][code&3], rcclip);
			pt.x += code >> 2;
			continue;
		}

		code = GetNibble(p, offset, nField, fAligned);
		DrawPixels(yuv, w, pt, rc.right - pt.x, m_sppal[0][code&3], rcclip);

		if(!fAligned) GetNibble(p, offset, nField, fAligned); // align to byte

		pt.x = rc.left;
		pt.y++;
		nField = 1 - nField;
	}
}

// CSubpicInputPin::svcdspu

bool CSubpicInputPin::svcdspu::Parse()
{
	BYTE* p = &m_buff[0];
	BYTE* p0 = p;

	if(m_buff.size() < 2)
	{
		return false;
	}

	WORD packetsize = (p[0] << 8) | p[1]; p += 2;

    if(packetsize > m_buff.size())
	{
		return false;
	}

	bool duration = !!(*p++ & 0x04);

	*p++; // unknown

	if(duration)
	{
		m_stop = m_start + 10000i64 * ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]) / 90;

		p += 4;
	}

	m_sphli.StartX = m_sphli.StopX = (p[0] << 8) | p[1]; p += 2;
	m_sphli.StartY = m_sphli.StopY = (p[0] << 8) | p[1]; p += 2;
	m_sphli.StopX += (p[0] << 8) | p[1]; p += 2;
	m_sphli.StopY += (p[0] << 8) | p[1]; p += 2;

	for(int i = 0; i < 4; i++)
	{
		m_sppal[i].Y = *p++;
		m_sppal[i].U = *p++;
		m_sppal[i].V = *p++;
		m_sppal[i].Reserved = *p++ >> 4;
	}

	if(*p++ & 0xc0)
	{
		p += 4; // duration of the shift operation should be here, but it is untested
	}

	m_offset[1] = (p[0] << 8) | p[1]; p += 2;

	m_offset[0] = p - p0;
	m_offset[1] += m_offset[0];

	return true;
}

void CSubpicInputPin::svcdspu::Render(REFERENCE_TIME rt, BYTE** yuv, int w, int h, AM_DVD_YUV* sppal, bool fsppal)
{
	BYTE* p = &m_buff[0];

	DWORD offset[2] = {m_offset[0], m_offset[1]};

	Vector4i rcclip(0, 0, w, h);

	/* FIXME: startx/y looks to be wrong in the sample I tested (yes, this one too!)
	CPoint pt(m_sphli.StartX, m_sphli.StartY);
	CRect rc(pt, CPoint(m_sphli.StopX, m_sphli.StopY));
	*/

	Vector2i size(m_sphli.StopX - m_sphli.StartX, m_sphli.StopY - m_sphli.StartY);
	Vector2i pt((rcclip.width() - size.x) / 2, (rcclip.height() * 3 - size.y * 1) / 4);
	Vector4i rc(pt.x, pt.y, pt.x + size.x, pt.y + size.y);

	int nField = 0;
	int n = 3;

	DWORD end[2] = {offset[1], (p[2] << 8) | p[3]};

	while((nField == 0 && offset[0] < end[0]) || (nField == 1 && offset[1] < end[1]))
	{
		BYTE code = GetHalfNibble(p, offset, nField, n);
		BYTE repeat = 1 + (code == 0 ? GetHalfNibble(p, offset, nField, n) : 0);

		DrawPixels(yuv, w, pt, repeat, m_sppal[code&3], rcclip);

		if((pt.x += repeat) < rc.right)
		{
			continue;
		}

		while(n != 3)
		{
			GetHalfNibble(p, offset, nField, n); // align to byte
		}

		pt.x = rc.left;
		pt.y++;

		nField = 1 - nField;
	}
}

// CClosedCaptionOutputPin

CClosedCaptionOutputPin::CClosedCaptionOutputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(NAME("CClosedCaptionOutputPin"), pFilter, pLock, phr, L"~CC")
{
}

HRESULT CClosedCaptionOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	return mtOut->majortype == MEDIATYPE_AUXLine21Data && mtOut->subtype == MEDIASUBTYPE_Line21_GOPPacket
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CClosedCaptionOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->InitMediaType();
	pmt->majortype = MEDIATYPE_AUXLine21Data;
	pmt->subtype = MEDIASUBTYPE_Line21_GOPPacket;
	pmt->formattype = FORMAT_None;

	return S_OK;
}

HRESULT CClosedCaptionOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 2048;

	return S_OK;
}

HRESULT CClosedCaptionOutputPin::Deliver(const void* ptr, int len)
{
	HRESULT hr = S_FALSE;

	if(len > 4 && ptr && *(DWORD*)ptr == 0xf8014343 && IsConnected())
	{
		CComPtr<IMediaSample> pSample;
		GetDeliveryBuffer(&pSample, NULL, NULL, 0);
		BYTE* pData = NULL;
		pSample->GetPointer(&pData);
		*(DWORD*)pData = 0xb2010000;
		memcpy(pData + 4, ptr, len);
		pSample->SetActualDataLength(len + 4);
		hr = __super::Deliver(pSample);
	}

	return hr;
}
