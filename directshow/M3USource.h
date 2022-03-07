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

#include "BaseSplitterFile.h"
#include "ThreadSafeQueue.h"
#include "DSMPropertyBag.h"
#include "../util/Socket.h"

[uuid("4DFEF71D-0899-4EBC-8755-E3A02D8E8E18")]
class CM3USourceFilter 
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IFileSourceFilter
	, public IDSMPropertyBagImpl
{
	class CM3USourceOutputPin 
		: public CBaseOutputPin
		, protected CAMThread
	{
		CThreadSafeQueue<std::vector<BYTE>*> m_queue;
		CAMEvent m_exit;
		bool m_flushing;
		std::vector<BYTE> m_buff;
		int m_size;
		REFERENCE_TIME m_start;

		DWORD ThreadProc();

		void DeliverSample(CBaseSplitterFile& f, int len, REFERENCE_TIME dur);

		HRESULT Deliver(IMediaSample* pSample) {ASSERT(0); return E_UNEXPECTED;}

	public:
		CM3USourceOutputPin(CM3USourceFilter* pFilter, HRESULT* phr);
		virtual ~CM3USourceOutputPin();

		HRESULT Active();
		HRESULT Inactive();

		HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, float dRate);
		HRESULT Deliver(std::vector<BYTE>* buff);
		HRESULT DeliverBeginFlush();
		HRESULT DeliverEndFlush();

	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}
	};

protected:
	CM3USourceOutputPin* m_pOutput;
	std::wstring m_url;

	enum {CMD_EXIT, CMD_FLUSH, CMD_START};

	DWORD ThreadProc();

	HRESULT Load(Util::Socket& s, LPCSTR url, int& meta);

public:
	CM3USourceFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CM3USourceFilter();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);
};
