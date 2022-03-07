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

#include "SubInputPin.h"

[uuid("7E5781CD-5863-48A4-AD1E-745C246E687E")]
class CSubPassThruFilter : public CBaseFilter, public CCritSec
{
	class CInputPin : public CSubInputPin
	{
		CComPtr<ISubStream> m_ssold;

	protected:
		void AddSubStream(ISubStream* ss);
		void RemoveSubStream(ISubStream* ss);
		void InvalidateSubtitle(REFERENCE_TIME start, ISubStream* ss);

	public:
		CInputPin(CSubPassThruFilter* pFilter, CCritSec* pLock, HRESULT* phr);
		
		STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
		STDMETHODIMP Receive(IMediaSample* pSample);
	    STDMETHODIMP EndOfStream();
	    STDMETHODIMP BeginFlush();
	    STDMETHODIMP EndFlush();
	};

	class COutputPin : public CBaseOutputPin
	{
	public:
		COutputPin(CSubPassThruFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	    HRESULT CheckMediaType(const CMediaType* mtOut);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);
		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}
	};

public:
	interface ICallback
	{
		virtual void ReplaceSubtitle(ISubStream* ssold, ISubStream* ssnew) = 0;
		virtual void InvalidateSubtitle(REFERENCE_TIME start, ISubStream* ss) = 0;
	};

private:
	CInputPin* m_input;
	COutputPin* m_output;

	ICallback* m_cb;

public:
	CSubPassThruFilter(ICallback* cb);
	virtual ~CSubPassThruFilter();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);
};
