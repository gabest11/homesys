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
#include "ImageGrabber.h"
#include "DirectShow.h"
#include "JPEGEncoder.h"
#include <initguid.h>
#include "moreuuids.h"

CImageGrabberFilter::CImageGrabberFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(_T("CImageGrabberFilter"), lpunk, this, __uuidof(*this))
{
	m_pInput = new CImageGrabberInputPin(this, phr);
}

CImageGrabberFilter::~CImageGrabberFilter()
{
	delete m_pInput;
}

STDMETHODIMP CImageGrabberFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IImageGrabberFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CImageGrabberFilter::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->majortype != MEDIATYPE_Video || pmt->subtype != MEDIASUBTYPE_RGB32)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CImageGrabberFilter::Receive(IMediaSample* pSample)
{
	CAutoLock cAutoLock(&m_csReceive);

	const CMediaType& mt = m_pInput->CurrentMediaType();

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;

	size_t hdrlen = mt.cbFormat - ((BYTE*)&vih->bmiHeader - mt.pbFormat);

	BYTE* src = NULL;

	pSample->GetPointer(&src);

	long len = pSample->GetActualDataLength();

	if(m_buff.size() != hdrlen + len)
	{
		m_buff.resize(hdrlen + len);
	}

	BYTE* dst = &m_buff[0];

	memcpy(dst, &vih->bmiHeader, hdrlen);

	dst += hdrlen;

	memcpy(dst, src, len);

	return S_OK;
}

// IImageGrabberFilter

STDMETHODIMP CImageGrabberFilter::GetJPEG(std::vector<BYTE>& data)
{
	CAutoLock cAutoLock(&m_csReceive);

	CJPEGEncoderMem enc;

	return enc.Encode(&m_buff[0], data) ? S_OK : E_FAIL;
}

// CImageGrabberInputPin

CImageGrabberFilter::CImageGrabberInputPin::CImageGrabberInputPin(CImageGrabberFilter* pFilter, HRESULT* phr)
	: CBaseInputPin(_T("Input"), pFilter, pFilter, phr, L"Input")
{
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI2(IMixerPinConfig)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CImageGrabberFilter::CImageGrabberInputPin::CheckMediaType(const CMediaType* pmt)
{
	return ((CImageGrabberFilter*)m_pFilter)->CheckMediaType(pmt);
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::Receive(IMediaSample* pSample)
{
	return ((CImageGrabberFilter*)m_pFilter)->Receive(pSample);
}

// IMixerPinConfig

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::SetRelativePosition(DWORD dwLeft, DWORD dwTop, DWORD dwRight, DWORD dwBottom)
{
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::GetRelativePosition(DWORD* pdwLeft, DWORD* pdwTop, DWORD* pdwRight, DWORD* pdwBottom)
{
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::SetZOrder(DWORD dwZOrder)
{
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::GetZOrder(DWORD* pdwZOrder)
{
	*pdwZOrder = 0;

	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::SetColorKey(COLORKEY* pColorKey)
{
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::GetColorKey(COLORKEY* pColorKey, DWORD* pColor)
{
	*pColor = 0;

	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::SetBlendingParameter(DWORD dwBlendingParameter)
{
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::GetBlendingParameter(DWORD* pdwBlendingParameter)
{
	*pdwBlendingParameter = 0;

	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::SetAspectRatioMode(AM_ASPECT_RATIO_MODE amAspectRatioMode)
{
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::GetAspectRatioMode(AM_ASPECT_RATIO_MODE* pamAspectRatioMode)
{
	*pamAspectRatioMode = AM_ARMODE_LETTER_BOX;
	
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::SetStreamTransparent(BOOL bStreamTransparent)
{
	return S_OK;
}

STDMETHODIMP CImageGrabberFilter::CImageGrabberInputPin::GetStreamTransparent(BOOL* pbStreamTransparent)
{
	*pbStreamTransparent = FALSE;

	return S_OK;
}
