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

#include <qnetwork.h>
#include "BaseSplitterFile.h"
#include "AsyncReader.h"
#include "IKeyFrameInfo.h"
#include "IBufferInfo.h"
#include "IBitRateInfo.h"
#include "DSMPropertyBag.h"
#include "FontInstaller.h"

class CPacket
{
public:
	enum {None = 0, Discontinuity = 1, SyncPoint = 2, Appendable = 4, TimeValid = 8, EndOfStream = 16};
	
	int id;
	int flags;
	REFERENCE_TIME start, stop;
	std::vector<BYTE> buff;
	CMediaType mt;

	CPacket(int size = 0) : id(0), flags(0) {if(size > 0) buff.resize(size);}
	virtual ~CPacket() {}

	void SetData(const void* ptr, DWORD len) {buff.resize(len); memcpy(&buff[0], ptr, len);}
	bool IsAppendable() {return (flags & Appendable) != 0 && (flags & (Discontinuity | TimeValid | EndOfStream)) == 0 && !mt.IsValid();}

	virtual int GetDataSize() {return buff.size();}
};

class CPacketQueue : public CCritSec
{
	int m_size;
	std::list<CPacket*> m_packets;
	REFERENCE_TIME m_start;
	REFERENCE_TIME m_stop;

public:
	CPacketQueue();

	void RemoveAll();

	void Push(CPacket* p);
	CPacket* Pop();

	int GetCount();
	int GetSize();
	REFERENCE_TIME GetDuration();
};

class CBaseSplitterFilter;

class CBaseSplitterInputPin : public CBasePin
{
protected:
	CComQIPtr<IAsyncReader> m_reader;

public:
	CBaseSplitterInputPin(TCHAR* pName, CBaseSplitterFilter* pFilter, HRESULT* phr);
	virtual ~CBaseSplitterInputPin();

	HRESULT GetAsyncReader(IAsyncReader** ppAsyncReader);

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT CheckMediaType(const CMediaType* pmt);

    HRESULT CheckConnect(IPin* pPin);
    HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin* pPin);

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
};

class CBaseSplitterOutputPin 
	: public CBaseOutputPin
	, public IDSMPropertyBagImpl
	, protected CAMThread
	, public IMediaSeeking
	, public IBitRateInfo
{
protected:
	std::vector<CMediaType> m_mts;
	int m_buffers;

private:
	CPacketQueue m_queue;

	HRESULT m_hrDeliver;

	bool m_flushing;
	bool m_flushed;
	CAMEvent m_endflush;

	enum {CMD_EXIT};
    DWORD ThreadProc();

	void MakeISCRHappy();

	HRESULT DeliverPacketInternal(CPacket* p);

	// please only use DeliverPacket from the derived class

    HRESULT GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags);
    HRESULT Deliver(IMediaSample* pSample);

	// bitrate stats

	struct 
	{
		UINT64 nTotalBytesDelivered;
		REFERENCE_TIME rtTotalTimeDelivered;
		UINT64 nBytesSinceLastDeliverTime;
		REFERENCE_TIME rtLastDeliverTime;
		DWORD nCurrentBitRate;
		DWORD nAverageBitRate;
	} m_brs;

protected:
	REFERENCE_TIME m_start;

	// override this if you need some second level stream specific demuxing (optional)
	// the default implementation will send the sample as is

	virtual HRESULT DeliverPacket(CPacket* p);

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

public:
	CBaseSplitterOutputPin(const std::vector<CMediaType>& mts, LPCWSTR pName, CBaseSplitterFilter* pFilter, HRESULT* phr, int buffers = 0);
	CBaseSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int buffers = 0);
	virtual ~CBaseSplitterOutputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT SetName(LPCWSTR pName);

    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);

	// Queueing

	HANDLE GetThreadHandle() {ASSERT(m_hThread != NULL); return m_hThread;}
	void SetThreadPriority(int nPriority) {if(m_hThread) ::SetThreadPriority(m_hThread, nPriority);}

	HRESULT Active();
    HRESULT Inactive();

    HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	int QueueCount();
	int QueueSize();
	REFERENCE_TIME QueueDuration();

    HRESULT QueueEndOfStream();
	HRESULT QueuePacket(CPacket* p);

	// returns true for everything which (the lack of) would not block other streams (subtitle streams, basically)

	virtual bool IsDiscontinuous();

	// returns IStreamsSwitcherInputPin::IsActive(), when it can find one downstream

	bool IsActive();

	// IBitRateInfo

	STDMETHODIMP_(DWORD) GetCurrentBitRate() {return m_brs.nCurrentBitRate;}
	STDMETHODIMP_(DWORD) GetAverageBitRate() {return m_brs.nAverageBitRate;}
};

class CBaseSplitterFilter 
	: public CBaseFilter
	, public CCritSec
	, public IDSMPropertyBagImpl
	, public IDSMResourceBagImpl
	, public IDSMChapterBagImpl
	, protected CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
	, public IAMOpenProgress
	, public IAMMediaContent
	, public IAMExtendedSeeking
	, public IKeyFrameInfo
	, public IBufferInfo
{
	CCritSec m_csPinMap;
	std::map<DWORD, CBaseSplitterOutputPin*> m_pPinMap;

	CCritSec m_csmtnew;
	std::map<DWORD, CMediaType> m_mtnew;

	std::list<CBaseSplitterOutputPin*> m_pRetiredOutputs;

	CComQIPtr<ISyncReader> m_reader;

protected:
	std::wstring m_fn;

	CBaseSplitterInputPin* m_pInput;
	std::vector<CBaseSplitterOutputPin*> m_pOutputs;

	CBaseSplitterOutputPin* GetOutputPin(DWORD id);
	DWORD GetOutputId(CBaseSplitterOutputPin* pPin);
	HRESULT AddOutputPin(DWORD id, CBaseSplitterOutputPin* pPin);
	HRESULT RenameOutputPin(DWORD src, DWORD dst, const AM_MEDIA_TYPE* pmt);
	virtual HRESULT DeleteOutputPins();
	virtual HRESULT CreateOutputPins(IAsyncReader* pAsyncReader) = 0; // override this ...

	LONGLONG m_nOpenProgress;
	bool m_abort;

	REFERENCE_TIME m_rtDuration; // derived filter should set this at the end of CreateOutputs
	REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent, m_rtNewStart, m_rtNewStop;
	double m_dRate;

	std::set<UINT64> m_bDiscontinuitySent;
	std::set<CBaseSplitterOutputPin*> m_pActivePins;

	CAMEvent m_endflush;
	bool m_flushing;

	void DeliverBeginFlush();
	void DeliverEndFlush();
	HRESULT DeliverPacket(CPacket* p);

	DWORD m_priority;

	CFontInstaller m_font;

protected:
	enum {CMD_EXIT, CMD_SEEK};
    DWORD ThreadProc();

	// ... and also override all these too
	virtual bool DemuxInit() = 0;
	virtual void DemuxSeek(REFERENCE_TIME rt) = 0;
	virtual bool DemuxLoop() = 0;

public:
	CBaseSplitterFilter(LPCTSTR pName, LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid);
	virtual ~CBaseSplitterFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	bool IsAnyPinDrying();

	HRESULT BreakConnect(PIN_DIRECTION dir, CBasePin* pPin);
	HRESULT CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();

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

protected:
	friend class CBaseSplitterOutputPin;
	virtual HRESULT SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

private:
	REFERENCE_TIME m_rtLastStart, m_rtLastStop;
	std::list<void*> m_LastSeekers;

public:
	// IAMOpenProgress

	STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
	STDMETHODIMP AbortOperation();

	// IDispatch

	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) {return E_NOTIMPL;}

	// IAMMediaContent

	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName);
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating);
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright);
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) {return E_NOTIMPL;}

	// IAMExtendedSeeking

	STDMETHODIMP get_ExSeekCapabilities(long* pExCapabilities);
	STDMETHODIMP get_MarkerCount(long* pMarkerCount);
	STDMETHODIMP get_CurrentMarker(long* pCurrentMarker);
	STDMETHODIMP GetMarkerTime(long MarkerNum, double* pMarkerTime);
	STDMETHODIMP GetMarkerName(long MarkerNum, BSTR* pbstrMarkerName);
	STDMETHODIMP put_PlaybackSpeed(double Speed) {return E_NOTIMPL;}
	STDMETHODIMP get_PlaybackSpeed(double* pSpeed) {return E_NOTIMPL;}

	// IKeyFrameInfo

	STDMETHODIMP_(HRESULT) GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP_(HRESULT) GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);

	// IBufferInfo

	STDMETHODIMP_(int) GetCount();
	STDMETHODIMP GetStatus(int i, int& samples, int& size);
	STDMETHODIMP_(DWORD) GetPriority();
};

