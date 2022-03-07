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

#pragma once

#include "../3rdparty/baseclasses/baseclasses.h"
#include <mpconfig.h>

[uuid("C09511C9-F473-4116-8F2C-0B8F85CFBA4C")]
interface IImageGrabberFilter : public IUnknown
{
	STDMETHOD(GetJPEG)(std::vector<BYTE>& data) PURE;
};

[uuid("F29F5E18-E501-4E95-9FCD-979563A82FC8")]
class CImageGrabberFilter
	: public CBaseFilter
	, public CCritSec
	, public IImageGrabberFilter
{
	class CImageGrabberInputPin 
		: public CBaseInputPin
		, public IMixerPinConfig
	{
	public:
		CImageGrabberInputPin(CImageGrabberFilter* pFilter, HRESULT* phr);

		DECLARE_IUNKNOWN	
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		HRESULT CheckMediaType(const CMediaType* pmt);

		STDMETHODIMP Receive(IMediaSample* pSample);

		// IMixerPinConfig

		STDMETHODIMP SetRelativePosition(DWORD dwLeft, DWORD dwTop, DWORD dwRight, DWORD dwBottom);
		STDMETHODIMP GetRelativePosition(DWORD* pdwLeft, DWORD* pdwTop, DWORD* pdwRight, DWORD* pdwBottom);
		STDMETHODIMP SetZOrder(DWORD dwZOrder);
		STDMETHODIMP GetZOrder(DWORD* pdwZOrder);
		STDMETHODIMP SetColorKey(COLORKEY* pColorKey);
		STDMETHODIMP GetColorKey(COLORKEY* pColorKey, DWORD* pColor);
		STDMETHODIMP SetBlendingParameter(DWORD dwBlendingParameter);
		STDMETHODIMP GetBlendingParameter(DWORD* pdwBlendingParameter);
		STDMETHODIMP SetAspectRatioMode(AM_ASPECT_RATIO_MODE amAspectRatioMode);
		STDMETHODIMP GetAspectRatioMode(AM_ASPECT_RATIO_MODE* pamAspectRatioMode);
		STDMETHODIMP SetStreamTransparent(BOOL bStreamTransparent);
		STDMETHODIMP GetStreamTransparent(BOOL* pbStreamTransparent);
	};

	CImageGrabberInputPin* m_pInput;
	CCritSec m_csReceive;
	std::vector<BYTE> m_buff;

public:
	CImageGrabberFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CImageGrabberFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT Receive(IMediaSample* pSample);

	int GetPinCount() {return 1;}
	CBasePin* GetPin(int n) {return n == 0 ? m_pInput : NULL;}

	// IImageGrabberFilter

	STDMETHODIMP GetJPEG(std::vector<BYTE>& data);
};