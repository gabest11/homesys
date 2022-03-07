/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <evr9.h>
#include "BaseVideo.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include "BitBlt.h"
#include <initguid.h>
#include "moreuuids.h"

// CBaseVideoFilter

CBaseVideoFilter::CBaseVideoFilter(TCHAR* pName, LPUNKNOWN lpunk, HRESULT* phr, REFCLSID clsid, long buffers) 
	: CTransformFilter(pName, lpunk, clsid)
	, m_buffers(buffers)
	, m_evr(false)
	, m_dvd(false)
	, m_late(0)
{
	if(phr) *phr = S_OK;

	m_pInput = new CBaseVideoInputPin(NAME("CBaseVideoInputPin"), this, phr, L"Video");
	m_pOutput = new CBaseVideoOutputPin(NAME("CBaseVideoOutputPin"), this, phr, L"Output");

	memset(&m_isize, 0, sizeof(m_isize));
	memset(&m_iar, 0, sizeof(m_iar));
	memset(&m_osize, 0, sizeof(m_osize));
	memset(&m_oar, 0, sizeof(m_oar));
	memset(&m_size, 0, sizeof(m_size));
	memset(&m_ar, 0, sizeof(m_ar));
}

CBaseVideoFilter::~CBaseVideoFilter()
{
}

void CBaseVideoFilter::SetAspectRatio(const Vector2i& ar)
{
	m_ar = ar;
}

int CBaseVideoFilter::GetPinCount()
{
	return 2;
}

CBasePin* CBaseVideoFilter::GetPin(int n)
{
	switch(n)
	{
	case 0: return m_pInput;
	case 1: return m_pOutput;
	}

	return NULL;
}

HRESULT CBaseVideoFilter::Receive(IMediaSample* pIn)
{
	_mm_empty(); // just for safety

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

HRESULT CBaseVideoFilter::GetDeliveryBuffer(int w, int h, IMediaSample** ppOut)
{
	CheckPointer(ppOut, E_POINTER);

	HRESULT hr;

	hr = ReconnectOutput(w, h);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pOutput->GetDeliveryBuffer(ppOut, NULL, NULL, 0);

	if(FAILED(hr))
	{
		return hr;
	}

	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED((*ppOut)->GetMediaType(&pmt)) && pmt != NULL)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	(*ppOut)->SetDiscontinuity(FALSE);
	(*ppOut)->SetSyncPoint(TRUE);

	// FIXME: hell knows why but without this the overlay mixer starts very skippy
	// (don't enable this for other renderers, the old for example will go crazy if you do)

	if(DirectShow::GetCLSID(m_pOutput->GetConnected()) == CLSID_OverlayMixer)
	{
		(*ppOut)->SetDiscontinuity(TRUE);
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::ReconnectOutput(int w, int h)
{
	CMediaType& mt = m_pOutput->CurrentMediaType();

	Vector2i size = m_size;

	bool reconnect = false;

	if(size != Vector2i(w, h))
	{
		reconnect = true;

		m_size.x = w;
		m_size.y = h;
	}
	else if(m_size != m_osize || m_ar.x * m_oar.y != m_oar.x * m_ar.y)
	{
		reconnect = true;
	}

	HRESULT hr = S_OK;

	if(reconnect)
	{
		if(DirectShow::GetCLSID(m_pOutput->GetConnected()) == CLSID_VideoRenderer)
		{
			NotifyEvent(EC_ERRORABORT, 0, 0);

			return E_FAIL;
		}

		BITMAPINFOHEADER* bih = NULL;

		if(mt.formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.Format();

			SetRect(&vih->rcSource, 0, 0, m_realsize.x, m_realsize.y);
			SetRect(&vih->rcTarget, 0, 0, m_size.x, m_size.y);

			vih->bmiHeader.biXPelsPerMeter = m_size.x * m_ar.y;
			vih->bmiHeader.biYPelsPerMeter = m_size.y * m_ar.x;

			bih = &vih->bmiHeader;
		}
		else if(mt.formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.Format();

			SetRect(&vih->rcSource, 0, 0, m_realsize.x, m_realsize.y);
			SetRect(&vih->rcTarget, 0, 0, m_size.x, m_size.y);

			vih->dwPictAspectRatioX = m_ar.x;
			vih->dwPictAspectRatioY = m_ar.y;

			bih = &vih->bmiHeader;
		}

		bih->biWidth = m_size.x;
		bih->biHeight = m_size.y;
		bih->biSizeImage = m_size.x * m_size.y * bih->biBitCount >> 3;

		hr = m_pOutput->GetConnected()->QueryAccept(&mt);

		ASSERT(SUCCEEDED(hr)); // should better not fail, after all "mt" is the current media type, just with a different resolution

		HRESULT hr1 = 0, hr2 = 0;

		CComPtr<IPin> pInput = m_pOutput->GetConnected();
		CComPtr<IMediaSample> pOut;
		
		if(SUCCEEDED(hr1 = pInput->ReceiveConnection(m_pOutput, &mt))
		&& SUCCEEDED(hr2 = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0)))
		{
			AM_MEDIA_TYPE* pmt = NULL;

			if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt != NULL)
			{
				CMediaType mt = *pmt;
				m_pOutput->SetMediaType(&mt);
				DeleteMediaType(pmt);
			}
			else if(DirectShow::GetCLSID(pInput) == CLSID_OverlayMixer) // stupid overlay mixer won't let us know the new pitch...
			{
				bih->biWidth = pOut->GetSize() / bih->biHeight * 8 / bih->biBitCount;
			}
			else
			{
				pOut = NULL;

				if(CanResetAllocator())
				{
					hr = ResetAllocator(); // FIXME: dxva
				}

				if(FAILED(hr))
				{
					return hr;
				}
			}
		}
		else
		{
			m_size = size;

			return E_FAIL;
		}

		m_osize = m_size;
		m_oar = m_ar;

		// some renderers don't send this

		NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(m_size.x, m_size.y), 0);

		return S_OK;
	}

	return S_FALSE;
}

HRESULT CBaseVideoFilter::ResetAllocator()
{
	CComQIPtr<IMemInputPin> pMemInput = m_pOutput->GetConnected();

	if(pMemInput == NULL)
	{
		return E_FAIL;
	}

	HRESULT hr;

	CComPtr<IMemAllocator> pAllocator;

	hr = pMemInput->GetAllocator(&pAllocator);

	if(FAILED(hr))
	{
		return hr;
	}

	ALLOCATOR_PROPERTIES props;

	memset(&props, 0, sizeof(props));

	pMemInput->GetAllocatorRequirements(&props);

	if(props.cbAlign == 0) 
	{
		props.cbAlign = 1;
	}

	hr = pAllocator->Decommit();

	if(FAILED(hr)) 
	{
		return hr;
	}

	hr = m_pOutput->DecideBufferSize(pAllocator, &props);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = pAllocator->Commit();

	if(FAILED(hr))
	{
		return hr;
	}

	hr = pMemInput->NotifyAllocator(pAllocator, FALSE);
	
	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::CopyBuffer(BYTE* pOut, BYTE* pIn, int w, int h, int pitchIn, const GUID& subtype, bool interlaced)
{
	int abs_h = abs(h);

	BYTE* yuv[3] = 
	{
		pIn, 
		pIn + pitchIn * abs_h, 
		pIn + pitchIn * abs_h + (pitchIn >> 1) * (abs_h >> 1)
	};

	return CopyBuffer(pOut, yuv, w, h, pitchIn, subtype, interlaced);
}

HRESULT CBaseVideoFilter::CopyBuffer(BYTE* pOut, BYTE** ppIn, int w, int h, int pitchIn, const GUID& subtype, bool interlaced)
{
	BITMAPINFOHEADER bihOut;

	CMediaTypeEx(m_pOutput->CurrentMediaType()).ExtractBIH(&bihOut);

	int pitchOut = 0;

	if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
	{
		pitchOut = bihOut.biWidth * bihOut.biBitCount >> 3;

		if(bihOut.biHeight > 0)
		{
			pOut += pitchOut * (h - 1);
			pitchOut = -pitchOut;
			if(h < 0) h = -h;
		}
	}

	if(h < 0)
	{
		h = -h;
		ppIn[0] += pitchIn * (h - 1);
		ppIn[1] += (pitchIn >> 1) * ((h >> 1) - 1);
		ppIn[2] += (pitchIn >> 1) * ((h >> 1) - 1);
		pitchIn = -pitchIn;
	}

	if(subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV || subtype == MEDIASUBTYPE_YV12)
	{
		BYTE* y = ppIn[0];
		BYTE* u = ppIn[1];
		BYTE* v = ppIn[2];

		if(subtype == MEDIASUBTYPE_YV12) {BYTE* tmp = u; u = v; v = tmp;}

		BYTE* pOutU = pOut + bihOut.biWidth * h;
		BYTE* pOutV = pOut + bihOut.biWidth * h * 5 / 4;

		if(bihOut.biCompression == '21VY') {BYTE* tmp = u; u = v; v = tmp;}

		ASSERT(w <= abs(pitchIn));

		if(bihOut.biCompression == '2YUY')
		{
			Image::I420ToYUY2(w, h, pOut, bihOut.biWidth * 2, y, u, v, pitchIn, interlaced);
		}
		else if(bihOut.biCompression == '024I' || bihOut.biCompression == 'VUYI' || bihOut.biCompression == '21VY')
		{
			Image::I420ToI420(w, h, pOut, pOutU, pOutV, bihOut.biWidth, y, u, v, pitchIn);
		}
		else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
		{
			if(!Image::I420ToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, y, u, v, pitchIn))
			{
				for(DWORD y = 0; y < h; y++, pOut += pitchOut)
				{
					memset(pOut, 0, pitchOut);
				}
			}
		}
	}
	else if(subtype == MEDIASUBTYPE_YUY2)
	{
		if(bihOut.biCompression == '2YUY')
		{
			Image::YUY2ToYUY2(w, h, pOut, bihOut.biWidth * 2, ppIn[0], pitchIn);
		}
		else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
		{
			if(!Image::YUY2ToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, ppIn[0], pitchIn))
			{
				for(DWORD y = 0; y < h; y++, pOut += pitchOut)
				{
					memset(pOut, 0, pitchOut);
				}
			}
		}
	}
	else if(subtype == MEDIASUBTYPE_ARGB32 || subtype == MEDIASUBTYPE_RGB32 || subtype == MEDIASUBTYPE_RGB24 || subtype == MEDIASUBTYPE_RGB565)
	{
		int sbpp = 
			subtype == MEDIASUBTYPE_ARGB32 || subtype == MEDIASUBTYPE_RGB32 ? 32 :
			subtype == MEDIASUBTYPE_RGB24 ? 24 :
			subtype == MEDIASUBTYPE_RGB565 ? 16 : 0;

		if(bihOut.biCompression == '2YUY')
		{
			// TODO
			// BitBltFromRGBToYUY2();
		}
		else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
		{
			if(!Image::RGBToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, ppIn[0], pitchIn, sbpp))
			{
				for(DWORD y = 0; y < h; y++, pOut += pitchOut)
				{
					memset(pOut, 0, pitchOut);
				}
			}
		}
	}
	else
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

void CBaseVideoFilter::SetInterlaceFlags(IMediaSample* pMS, bool weave, bool tff, bool rff)
{
	if(CComQIPtr<IMediaSample2> pMS2 = pMS)
	{
		AM_SAMPLE2_PROPERTIES props;

		if(SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props)))
		{
			props.dwTypeSpecificFlags &= ~0x7f;

			const CMediaType& mt = m_pOutput->CurrentMediaType();

			if(mt.formattype == FORMAT_VideoInfo2)
			{
				if(weave)
				{
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_WEAVE;
				}
				else
				{
					if(tff)
					{
						props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_FIELD1FIRST;
					}

					if(rff)
					{
						props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_REPEAT_FIELD;
					}
				}
			}

			pMS2->SetProperties(sizeof(props), (BYTE*)&props);
		}
	}
}

HRESULT CBaseVideoFilter::CheckInputType(const CMediaType* mtIn)
{
	BITMAPINFOHEADER bih;

	CMediaTypeEx(*mtIn).ExtractBIH(&bih);

	return mtIn->majortype == MEDIATYPE_Video 
		&& (mtIn->subtype == MEDIASUBTYPE_YV12 
		 || mtIn->subtype == MEDIASUBTYPE_I420 
		 || mtIn->subtype == MEDIASUBTYPE_IYUV
		 || mtIn->subtype == MEDIASUBTYPE_YUY2
		 || mtIn->subtype == MEDIASUBTYPE_ARGB32
		 || mtIn->subtype == MEDIASUBTYPE_RGB32
		 || mtIn->subtype == MEDIASUBTYPE_RGB24
 		 || mtIn->subtype == MEDIASUBTYPE_RGB565)
		&& (mtIn->formattype == FORMAT_VideoInfo 
		 || mtIn->formattype == FORMAT_VideoInfo2)
		&& bih.biHeight > 0
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CBaseVideoFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(FAILED(CheckInputType(mtIn)) || mtOut->majortype != MEDIATYPE_Video)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if(mtIn->majortype == MEDIATYPE_Video 
	&& (mtIn->subtype == MEDIASUBTYPE_YV12 
	 || mtIn->subtype == MEDIASUBTYPE_I420 
	 || mtIn->subtype == MEDIASUBTYPE_IYUV))
	{
		if(mtOut->subtype != MEDIASUBTYPE_YV12
		&& mtOut->subtype != MEDIASUBTYPE_I420
		&& mtOut->subtype != MEDIASUBTYPE_IYUV
		&& mtOut->subtype != MEDIASUBTYPE_YUY2
		&& mtOut->subtype != MEDIASUBTYPE_ARGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB24
		&& mtOut->subtype != MEDIASUBTYPE_RGB565)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	else if(mtIn->majortype == MEDIATYPE_Video 
	&& (mtIn->subtype == MEDIASUBTYPE_YUY2))
	{
		if(mtOut->subtype != MEDIASUBTYPE_YUY2
		&& mtOut->subtype != MEDIASUBTYPE_ARGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB24
		&& mtOut->subtype != MEDIASUBTYPE_RGB565)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	else if(mtIn->majortype == MEDIATYPE_Video 
	&& (mtIn->subtype == MEDIASUBTYPE_ARGB32
	|| mtIn->subtype == MEDIASUBTYPE_RGB32
	|| mtIn->subtype == MEDIASUBTYPE_RGB24
	|| mtIn->subtype == MEDIASUBTYPE_RGB565))
	{
		if(mtOut->subtype != MEDIASUBTYPE_ARGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB24
		&& mtOut->subtype != MEDIASUBTYPE_RGB565)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::CheckOutputType(const CMediaType& mtOut)
{
	Vector4i dim;

	return CMediaTypeEx(mtOut).ExtractDim(dim)
		&& m_size.y == abs(dim.y)
		&& mtOut.subtype != MEDIASUBTYPE_ARGB32
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CBaseVideoFilter::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	BITMAPINFOHEADER bih;

	CMediaTypeEx(m_pOutput->CurrentMediaType()).ExtractBIH(&bih);

	int buffers = m_pOutput->CurrentMediaType().formattype == FORMAT_VideoInfo ? 1 : m_buffers;

	pProperties->cBuffers = m_buffers;
	pProperties->cbBuffer = bih.biSizeImage;

	return S_OK;
}

CBaseVideoFilter::VIDEO_OUTPUT_FORMATS CBaseVideoFilter::m_formats[] =
{
	{&MEDIASUBTYPE_YV12, 3, 12, '21VY'},
	{&MEDIASUBTYPE_I420, 3, 12, '024I'},
	{&MEDIASUBTYPE_IYUV, 3, 12, 'VUYI'},
	{&MEDIASUBTYPE_YUY2, 1, 16, '2YUY'},
	{&MEDIASUBTYPE_ARGB32, 1, 32, BI_RGB},
	{&MEDIASUBTYPE_RGB32, 1, 32, BI_RGB},
	{&MEDIASUBTYPE_RGB24, 1, 24, BI_RGB},
	{&MEDIASUBTYPE_RGB565, 1, 16, BI_RGB},
	{&MEDIASUBTYPE_RGB555, 1, 16, BI_RGB},
	{&MEDIASUBTYPE_ARGB32, 1, 32, BI_BITFIELDS},
	{&MEDIASUBTYPE_RGB32, 1, 32, BI_BITFIELDS},
	{&MEDIASUBTYPE_RGB24, 1, 24, BI_BITFIELDS},
	{&MEDIASUBTYPE_RGB565, 1, 16, BI_BITFIELDS},
	{&MEDIASUBTYPE_RGB555, 1, 16, BI_BITFIELDS},
};

void CBaseVideoFilter::GetOutputFormats(int& count, VIDEO_OUTPUT_FORMATS** formats)
{
	count = sizeof(m_formats) / sizeof(m_formats[0]);
	*formats = m_formats;
}

HRESULT CBaseVideoFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	// this will make sure we won't connect to the old renderer in dvd mode
	// that renderer can't switch the format dynamically

	bool dvdnav = false;

	CComPtr<IBaseFilter> pBF = this;
	CComPtr<IPin> pPin = m_pInput;

	for(; !dvdnav && (pBF = DirectShow::GetUpStreamFilter(pBF, pPin)); pPin = DirectShow::GetFirstPin(pBF))
	{
		dvdnav = !!(DirectShow::GetCLSID(pBF) == CLSID_DVDNavigator);
	}

	if(dvdnav || m_pInput->CurrentMediaType().formattype == FORMAT_VideoInfo2)
	{
		iPosition = iPosition * 2;
	}

	//

	VIDEO_OUTPUT_FORMATS* fmts = NULL;
	int count = 0;

	GetOutputFormats(count, &fmts);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= 2 * count) return VFW_S_NO_MORE_ITEMS;

	Vector2i size = m_isize;
	Vector2i ar = m_iar;
	Vector2i realsize = m_isize;

	GetOutputSize(size, ar, realsize);

	int i = iPosition / 2;

	CMediaTypeEx mt;

	mt.majortype = MEDIATYPE_Video;
	mt.subtype = *fmts[i].subtype;

	BITMAPINFOHEADER bih;

	memset(&bih, 0, sizeof(bih));

	bih.biSize = sizeof(bih);
	bih.biWidth = size.x;
	bih.biHeight = size.y;
	bih.biPlanes = fmts[i].biPlanes;
	bih.biBitCount = fmts[i].biBitCount;
	bih.biCompression = fmts[i].biCompression;
	bih.biSizeImage = size.x * size.y * bih.biBitCount >> 3;

	if(iPosition & 1)
	{
		mt.formattype = FORMAT_VideoInfo;

		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));

		memset(vih, 0, sizeof(VIDEOINFOHEADER));

		vih->bmiHeader = bih;
		vih->bmiHeader.biXPelsPerMeter = vih->bmiHeader.biWidth * ar.y;
		vih->bmiHeader.biYPelsPerMeter = vih->bmiHeader.biHeight * ar.x;

		SetRect(&vih->rcSource, 0, 0, realsize.x, realsize.y);
		SetRect(&vih->rcTarget, 0, 0, size.x, size.y);
	}
	else
	{
		mt.formattype = FORMAT_VideoInfo2;

		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));

		memset(vih, 0, sizeof(VIDEOINFOHEADER2));

		vih->bmiHeader = bih;
		vih->dwPictAspectRatioX = ar.x;
		vih->dwPictAspectRatioY = ar.y;
		vih->dwInterlaceFlags = AMINTERLACE_IsInterlaced;

		SetRect(&vih->rcSource, 0, 0, realsize.x, realsize.y);
		SetRect(&vih->rcTarget, 0, 0, size.x, size.y);
	}

	CMediaType& mtIn = m_pInput->CurrentMediaType();

	// these fields have the same field offset in all four structs

	((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame = ((VIDEOINFOHEADER*)mtIn.Format())->AvgTimePerFrame;
	((VIDEOINFOHEADER*)mt.Format())->dwBitRate = ((VIDEOINFOHEADER*)mtIn.Format())->dwBitRate;
	((VIDEOINFOHEADER*)mt.Format())->dwBitErrorRate = ((VIDEOINFOHEADER*)mtIn.Format())->dwBitErrorRate;

	mt.FixMediaType();

	*pmt = mt;

	return S_OK;
}

HRESULT CBaseVideoFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	if(dir == PINDIR_INPUT)
	{
		memset(&m_size, 0, sizeof(m_size));
		memset(&m_ar, 0, sizeof(m_ar));

		Vector4i dim;

		CMediaTypeEx(*pmt).ExtractDim(dim);

		m_isize = m_size = dim.tl;
		m_iar = m_ar = dim.br;
		m_realsize = dim.tl;

		GetOutputSize(m_size, m_ar, m_realsize);
	}
	else if(dir == PINDIR_OUTPUT)
	{
		Vector4i dim;

		CMediaTypeEx(*pmt).ExtractDim(dim);

		if(m_size == dim.tl && m_ar == dim.br)
		{
			m_osize = dim.tl;
			m_oar = dim.br;
		}
	}

	return __super::SetMediaType(dir, pmt);
}

HRESULT CBaseVideoFilter::CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin)
{
	if(dir == PINDIR_INPUT)
	{
		m_dvd = DirectShow::GetCLSID(pReceivePin) == CLSID_DVDNavigator;
	}
	else if(dir == PINDIR_OUTPUT)
	{
		m_evr = DirectShow::GetCLSID(pReceivePin) == CLSID_EnhancedVideoRenderer;
	}

	return __super::CompleteConnect(dir, pReceivePin);
}

HRESULT CBaseVideoFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	
	m_late = 0;

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CBaseVideoFilter::AlterQuality(Quality q)
{
	m_late = (int)(q.Late / 10000);

	return S_OK;
}

// CBaseVideoInputAllocator
	
CBaseVideoInputAllocator::CBaseVideoInputAllocator(HRESULT* phr)
	: CMemAllocator(NAME("CBaseVideoInputAllocator"), NULL, phr)
{
	if(phr) *phr = S_OK;
}

void CBaseVideoInputAllocator::SetMediaType(const CMediaType& mt)
{
	m_mt = mt;
}

STDMETHODIMP CBaseVideoInputAllocator::GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	if(!m_bCommitted)
	{
        return VFW_E_NOT_COMMITTED;
	}

	HRESULT hr;
	
	hr = __super::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if(SUCCEEDED(hr) && m_mt.majortype != GUID_NULL)
	{
		(*ppBuffer)->SetMediaType(&m_mt);

		m_mt.majortype = GUID_NULL;
	}

	return hr;
}

// CBaseVideoInputPin

CBaseVideoInputPin::CBaseVideoInputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName) 
	: CTransformInputPin(pObjectName, pFilter, phr, pName)
	, m_pAllocator(NULL)
{
}

CBaseVideoInputPin::~CBaseVideoInputPin()
{
	delete m_pAllocator;
}

STDMETHODIMP CBaseVideoInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
    CheckPointer(ppAllocator, E_POINTER);

    if(m_pAllocator == NULL)
	{
		HRESULT hr = S_OK;

        m_pAllocator = new CBaseVideoInputAllocator(&hr);
        m_pAllocator->AddRef();
    }

    (*ppAllocator = m_pAllocator)->AddRef();

    return S_OK;
} 

STDMETHODIMP CBaseVideoInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cObjectLock(m_pLock);

	if(m_Connected != NULL)
	{
		CMediaType mt(*pmt);

		if(FAILED(CheckMediaType(&mt)))
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		ALLOCATOR_PROPERTIES props, actual;

		CComPtr<IMemAllocator> pMemAllocator;

		if(FAILED(GetAllocator(&pMemAllocator))
		|| FAILED(pMemAllocator->Decommit())
		|| FAILED(pMemAllocator->GetProperties(&props)))
		{
			return E_FAIL;
		}

		BITMAPINFOHEADER bih;

		if(CMediaTypeEx(*pmt).ExtractBIH(&bih) && bih.biSizeImage)
		{
			props.cbBuffer = bih.biSizeImage;
		}

		if(FAILED(pMemAllocator->SetProperties(&props, &actual))
		|| FAILED(pMemAllocator->Commit())
		|| props.cbBuffer != actual.cbBuffer)
		{
			return E_FAIL;
		}

		if(m_pAllocator != NULL)
		{
			m_pAllocator->SetMediaType(mt);
		}

		return SetMediaType(&mt) == S_OK ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
	}

	return __super::ReceiveConnection(pConnector, pmt);
}

// CBaseVideoOutputPin

CBaseVideoOutputPin::CBaseVideoOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CTransformOutputPin(pObjectName, pFilter, phr, pName)
{
}

HRESULT CBaseVideoOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	if(IsConnected())
	{
		HRESULT hr;
		
		hr = ((CBaseVideoFilter*)m_pFilter)->CheckOutputType(*mtOut);

		if(FAILED(hr)) 
		{
			return hr;
		}
	}

	return __super::CheckMediaType(mtOut);
}