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

#include "BaseSplitter.h"
#include "../3rdparty/rtmp/rtmp.h"
#include "../DirectShow/ThreadSafeQueue.h"
#include <vector>
#include <string>

[uuid("C4A104F5-521F-4DDC-893D-CD045F09D4E2")]
class CRTMPSourceFilter
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
{
	class CRTMPSourceOutputPin 
		: public CBaseOutputPin
		, protected CAMThread
	{
		CThreadSafeQueue<CPacket*> m_queue;
		CAMEvent m_exit;
		bool m_flushing;

		DWORD ThreadProc();

		HRESULT Deliver(IMediaSample* pSample) {ASSERT(0); return E_UNEXPECTED;}

	public:
		CRTMPSourceOutputPin(CRTMPSourceFilter* pFilter, const CMediaType& mt, HRESULT* phr);
		virtual ~CRTMPSourceOutputPin();

		HRESULT Active();
		HRESULT Inactive();

		HRESULT Deliver(CPacket* p);
		HRESULT DeliverBeginFlush();
		HRESULT DeliverEndFlush();

	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}
	};

	std::vector<CRTMPSourceOutputPin*> m_outputs;
	CRTMPSourceOutputPin* m_video;
	CRTMPSourceOutputPin* m_audio;
	std::wstring m_url;
	REFERENCE_TIME m_current;
	REFERENCE_TIME m_duration;
	REFERENCE_TIME m_start;
	std::list<CPacket*> m_h264queue;
	std::vector<BYTE> m_h264seqhdr;
	bool m_waitkf;
	std::string m_playpath;

	void DeleteOutputs();

	enum {CMD_EXIT, CMD_FLUSH, CMD_START};

	DWORD ThreadProc();

	void Deliver(CRTMPSourceOutputPin* pin, int type, __int64 ts, const BYTE* ptr, int size);

public:
	CRTMPSourceFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CRTMPSourceFilter();

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

	// IMediaSeeking

	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);
};
