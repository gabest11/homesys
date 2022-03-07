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

#include "StdAfx.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"
#include "AudioSwitcher.h"
#include "BaseSplitter.h"

#pragma warning(disable: 4355)

#define MINPACKETS 10
#define MINPACKETSIZE 100 * 1024
#define MAXPACKETS 10000
#define MAXPACKETSIZE 1024 * 1024 * 5

// CPacketQueue

CPacketQueue::CPacketQueue() 
	: m_size(0)
	, m_start(0)
	, m_stop(0)
{
}

void CPacketQueue::Push(CPacket* p)
{
	CAutoLock cAutoLock(this);

	m_size += p->GetDataSize();
	
	if((p->flags & CPacket::TimeValid) != 0)
	{
		if(m_start < 0) m_start = p->start;

		m_stop = p->start;
	}

	if(p->IsAppendable() && !m_packets.empty() && (m_packets.back()->flags & CPacket::TimeValid) != 0)
	{
		CPacket* tail = m_packets.back();

		tail->buff.insert(tail->buff.end(), p->buff.begin(), p->buff.end());

		return;
	}

	m_packets.push_back(p);
}

CPacket* CPacketQueue::Pop()
{
	CAutoLock cAutoLock(this);

	CPacket* p = m_packets.front();

	m_packets.pop_front();

	m_size -= p->GetDataSize();

	if((p->flags & CPacket::TimeValid) != 0)
	{
		m_start = m_stop;

		for(auto i = m_packets.begin(); i != m_packets.end(); i++)
		{
			if(((*i)->flags & CPacket::TimeValid) != 0)
			{
				m_start = (*i)->start;

				break;
			}
		}
	}

	return p;
}

void CPacketQueue::RemoveAll()
{
	CAutoLock cAutoLock(this);

	m_size = 0;

	std::for_each(m_packets.begin(), m_packets.end(), [] (CPacket* p) {delete p;});

	m_packets.clear();
}

int CPacketQueue::GetCount()
{
	CAutoLock cAutoLock(this);

	return m_packets.size();
}

int CPacketQueue::GetSize()
{
	CAutoLock cAutoLock(this);

	return m_size;
}

REFERENCE_TIME CPacketQueue::GetDuration()
{
	CAutoLock cAutoLock(this);

	return m_stop - m_start;
}

// CBaseSplitterInputPin

CBaseSplitterInputPin::CBaseSplitterInputPin(TCHAR* pName, CBaseSplitterFilter* pFilter, HRESULT* phr)
	: CBasePin(pName, pFilter, pFilter, phr, L"Input", PINDIR_INPUT)
{
}

CBaseSplitterInputPin::~CBaseSplitterInputPin()
{
}

HRESULT CBaseSplitterInputPin::GetAsyncReader(IAsyncReader** ppAsyncReader)
{
	CheckPointer(ppAsyncReader, E_POINTER);

	*ppAsyncReader = NULL;

	CheckPointer(m_reader, VFW_E_NOT_CONNECTED);

	(*ppAsyncReader = m_reader)->AddRef();

	return S_OK;
}

STDMETHODIMP CBaseSplitterInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CBaseSplitterInputPin::CheckMediaType(const CMediaType* pmt)
{
	return S_OK;

	// return pmt->majortype == MEDIATYPE_Stream ? S_OK : E_INVALIDARG;
}

HRESULT CBaseSplitterInputPin::CheckConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CheckConnect(pPin)))
	{
		return hr;
	}

	return CComQIPtr<IAsyncReader>(pPin) ? S_OK : E_NOINTERFACE;
}

HRESULT CBaseSplitterInputPin::BreakConnect()
{
	HRESULT hr;

	hr = __super::BreakConnect();

	if(FAILED(hr))
	{
		return hr;
	}

	hr = ((CBaseSplitterFilter*)m_pFilter)->BreakConnect(PINDIR_INPUT, this);

	if(FAILED(hr))
	{
		return hr;
	}

	m_reader.Release();

	return S_OK;
}

HRESULT CBaseSplitterInputPin::CompleteConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CompleteConnect(pPin)))
	{
		return hr;
	}

	CheckPointer(pPin, E_POINTER);

	m_reader = pPin;

	CheckPointer(m_reader, E_NOINTERFACE);

	if(FAILED(hr = ((CBaseSplitterFilter*)m_pFilter)->CompleteConnect(PINDIR_INPUT, this)))
	{
		return hr;
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterInputPin::BeginFlush()
{
	return E_UNEXPECTED;
}

STDMETHODIMP CBaseSplitterInputPin::EndFlush()
{
	return E_UNEXPECTED;
}

//
// CBaseSplitterOutputPin
//

CBaseSplitterOutputPin::CBaseSplitterOutputPin(const std::vector<CMediaType>& mts, LPCWSTR pName, CBaseSplitterFilter* pFilter, HRESULT* phr, int buffers)
	: CBaseOutputPin(NAME("CBaseSplitterOutputPin"), pFilter, pFilter, phr, pName)
	, m_hrDeliver(S_OK) // just in case it were asked before the worker thread could be created and reset it
	, m_flushing(false)
	, m_endflush(TRUE)
{
	m_mts = mts;
	m_buffers = max(buffers, 1);
	memset(&m_brs, 0, sizeof(m_brs));
	m_brs.rtLastDeliverTime = _I64_MIN;
}

CBaseSplitterOutputPin::CBaseSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int buffers)
	: CBaseOutputPin(NAME("CBaseSplitterOutputPin"), pFilter, pLock, phr, pName)
	, m_hrDeliver(S_OK) // just in case it were asked before the worker thread could be created and reset it
	, m_flushing(false)
	, m_endflush(TRUE)
{
	m_buffers = max(buffers, 1);
	memset(&m_brs, 0, sizeof(m_brs));
	m_brs.rtLastDeliverTime = _I64_MIN;
}

CBaseSplitterOutputPin::~CBaseSplitterOutputPin()
{
}

STDMETHODIMP CBaseSplitterOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
//		riid == __uuidof(IMediaSeeking) ? m_pFilter->QueryInterface(riid, ppv) : 
		QI(IMediaSeeking)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		QI(IBitRateInfo)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CBaseSplitterOutputPin::SetName(LPCWSTR name)
{
	CheckPointer(name, E_POINTER);

	if(m_pName) delete [] m_pName;

	m_pName = new WCHAR[wcslen(name) + 1];

	CheckPointer(m_pName, E_OUTOFMEMORY);

	wcscpy(m_pName, name);

	return S_OK;
}

HRESULT CBaseSplitterOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
    HRESULT hr = NOERROR;

	pProperties->cBuffers = m_buffers;
	pProperties->cbBuffer = max(m_mt.lSampleSize, 1);

	if(m_mt.subtype == MEDIASUBTYPE_Vorbis && m_mt.formattype == FORMAT_VorbisFormat)
	{
		// oh great, the oggds vorbis decoder assumes there will be two at least, stupid thing...

		pProperties->cBuffers = max(pProperties->cBuffers, 2);
	}

    return S_OK;
}

HRESULT CBaseSplitterOutputPin::CheckMediaType(const CMediaType* pmt)
{
	auto i = std::find_if(m_mts.begin(), m_mts.end(), [&pmt] (const CMediaType& mt) {return mt == *pmt;});

	return i != m_mts.end() ? S_OK : E_INVALIDARG;
}

HRESULT CBaseSplitterOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pLock);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= m_mts.size()) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mts[iPosition];

	return S_OK;
}

STDMETHODIMP CBaseSplitterOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}

HRESULT CBaseSplitterOutputPin::Active()
{
    CAutoLock cAutoLock(m_pLock);

	if(m_Connected) 
	{
		Create();
	}

	return __super::Active();
}

HRESULT CBaseSplitterOutputPin::Inactive()
{
    CAutoLock cAutoLock(m_pLock);

	if(ThreadExists())
	{
		CallWorker(CMD_EXIT);
	}

	return __super::Inactive();
}

HRESULT CBaseSplitterOutputPin::DeliverBeginFlush()
{
	m_endflush.Reset();

	m_flushed = false;
	m_flushing = true;

	m_hrDeliver = S_FALSE;

	m_queue.RemoveAll();

	HRESULT hr = S_OK;
	
	if(IsConnected()) 
	{
		hr = GetConnected()->BeginFlush();

		m_endflush.Set();
	}

	return hr;
}

HRESULT CBaseSplitterOutputPin::DeliverEndFlush()
{
	if(!ThreadExists()) 
	{
		return S_FALSE;
	}

	HRESULT hr = S_OK;

	if(IsConnected())
	{
		hr = GetConnected()->EndFlush();
	}

	m_hrDeliver = S_OK;

	m_flushing = false;
	m_flushed = true;

	m_endflush.Set();

	return hr;
}

HRESULT CBaseSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_brs.rtLastDeliverTime = _I64_MIN;

	if(m_flushing) 
	{
		return S_FALSE;
	}

	m_start = tStart;

	if(!ThreadExists()) 
	{
		return S_FALSE;
	}

	HRESULT hr;
	
	hr = __super::DeliverNewSegment(tStart, tStop, dRate);

	if(S_OK != hr) 
	{
		return hr;
	}

	MakeISCRHappy();

	return hr;
}

int CBaseSplitterOutputPin::QueueCount()
{
	return m_queue.GetCount();
}

int CBaseSplitterOutputPin::QueueSize()
{
	return m_queue.GetSize();
}

REFERENCE_TIME CBaseSplitterOutputPin::QueueDuration()
{
	return m_queue.GetDuration();
}

HRESULT CBaseSplitterOutputPin::QueueEndOfStream()
{
	CPacket* p = new CPacket();

	p->flags |= CPacket::EndOfStream;

	return QueuePacket(p);
}

HRESULT CBaseSplitterOutputPin::QueuePacket(CPacket* p)
{
	if(!ThreadExists())
	{
		delete p;

		return S_FALSE;
	}

	CBaseSplitterFilter* f = (CBaseSplitterFilter*)m_pFilter;

	while(S_OK == m_hrDeliver && (!f->IsAnyPinDrying() || m_queue.GetSize() > MAXPACKETSIZE * 100))
	{
		Sleep(1);
	}

	if(S_OK != m_hrDeliver)
	{
		delete p;

		return m_hrDeliver;
	}

	m_queue.Push(p);

	return m_hrDeliver;
}

bool CBaseSplitterOutputPin::IsDiscontinuous()
{
	return m_mt.majortype == MEDIATYPE_Text
		|| m_mt.majortype == MEDIATYPE_ScriptCommand
		|| m_mt.majortype == MEDIATYPE_Subtitle 
		|| m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE 
		|| m_mt.subtype == MEDIASUBTYPE_CVD_SUBPICTURE 
		|| m_mt.subtype == MEDIASUBTYPE_SVCD_SUBPICTURE;
}

bool CBaseSplitterOutputPin::IsActive()
{
	CComPtr<IPin> pPin = this;

	do
	{
		CComPtr<IPin> pPinTo;
		CComQIPtr<IStreamSwitcherInputPin> pSSIP;

		if(S_OK == pPin->ConnectedTo(&pPinTo) && (pSSIP = pPinTo) && !pSSIP->IsActive())
		{
			return false;
		}

		pPin = DirectShow::GetFirstPin(DirectShow::GetFilter(pPinTo), PINDIR_OUTPUT);
	}
	while(pPin != NULL);

	return true;
}

DWORD CBaseSplitterOutputPin::ThreadProc()
{
	m_hrDeliver = S_OK;

	m_flushing = false;
	m_flushed = false;

	m_endflush.Set();

	while(1)
	{
		Sleep(1);

		if(CheckRequest(NULL))
		{
			m_hThread = NULL;

			GetRequest();

			Reply(S_OK);

			return 0;
		}

		int count = 0;

		do
		{
			CPacket* p = NULL;

			{
				CAutoLock cAutoLock(&m_queue);

				if((count = m_queue.GetCount()) > 0)
				{
					p = m_queue.Pop();
				}
			}

			if(S_OK == m_hrDeliver && count > 0)
			{
				ASSERT(!m_flushing);

				m_flushed = false;

				// flushing can still start here, to release a blocked deliver call

				HRESULT hr;

				if(p->flags & CPacket::EndOfStream)
				{
					hr = DeliverEndOfStream();
				}
				else
				{
					hr = DeliverPacket(p);
					
					p = NULL; // not ours anymore, don't delete it
				}

				m_endflush.Wait(); // .. so we have to wait until it is done

				if(hr != S_OK && !m_flushed) // and only report the error in m_hrDeliver if we didn't flush the stream
				{
					// CAutoLock cAutoLock(&m_csQueueLock);

					m_hrDeliver = hr;

					count = 0;
				}
			}

			delete p;
		}
		while(--count > 0);
	}
}

HRESULT CBaseSplitterOutputPin::DeliverPacket(CPacket* p)
{
	HRESULT hr = DeliverPacketInternal(p);

	delete p; // Queue/DeliverPacket calls transfer ownership of p, this is its final destination

	return hr;
}

HRESULT CBaseSplitterOutputPin::DeliverPacketInternal(CPacket* p)
{
	HRESULT hr;

	int bytes = p->buff.size();

	if(bytes == 0)
	{
		return S_OK;
	}

	m_brs.nBytesSinceLastDeliverTime += bytes;

	if(p->flags & CPacket::TimeValid)
	{
		if(m_brs.rtLastDeliverTime == _I64_MIN)
		{
			m_brs.rtLastDeliverTime = p->start;
			m_brs.nBytesSinceLastDeliverTime = 0;
		}

		if(m_brs.rtLastDeliverTime + 10000000 < p->start)
		{
			REFERENCE_TIME diff = p->start - m_brs.rtLastDeliverTime;

			double secs, bits;

			secs = (double)diff / 10000000;
			bits = 8.0 * m_brs.nBytesSinceLastDeliverTime;
			m_brs.nCurrentBitRate = (DWORD)(bits / secs);

			m_brs.rtTotalTimeDelivered += diff;
			m_brs.nTotalBytesDelivered += m_brs.nBytesSinceLastDeliverTime;

			secs = (double)m_brs.rtTotalTimeDelivered / 10000000;
			bits = 8.0 * m_brs.nTotalBytesDelivered;
			m_brs.nAverageBitRate = (DWORD)(bits / secs);

			m_brs.rtLastDeliverTime = p->start;
			m_brs.nBytesSinceLastDeliverTime = 0;

			// TRACE(_T("[%d] c: %d kbps, a: %d kbps\n"), p->TrackNumber, (m_brs.nCurrentBitRate + 500) / 1000, (m_brs.nAverageBitRate + 500) / 1000);
		}

		double rate = 1.0;

		if(SUCCEEDED(((CBaseSplitterFilter*)m_pFilter)->GetRate(&rate)) && rate != 1.0)
		{
			p->start = (REFERENCE_TIME)((double)p->start / rate);
			p->stop = (REFERENCE_TIME)((double)p->stop / rate);
		}
	}

	CComPtr<IMediaSample> pSample;

	hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);

	if(S_OK != hr) return hr;

	if(bytes > pSample->GetSize())
	{
		pSample.Release();

		ALLOCATOR_PROPERTIES props, actual;

		hr = hr = m_pAllocator->GetProperties(&props);

		if(S_OK != hr) return hr;

		props.cbBuffer = bytes * 3 / 2;

		if(props.cBuffers > 1)
		{
			hr = __super::DeliverBeginFlush();

			if(S_OK != hr) return hr;
			
			hr = __super::DeliverEndFlush();

			if(S_OK != hr) return hr;
		}

		hr = m_pAllocator->Decommit();

		if(S_OK != hr) return hr;
		
		hr = m_pAllocator->SetProperties(&props, &actual);
		
		if(S_OK != hr) return hr;
		
		hr = m_pAllocator->Commit();
		
		if(S_OK != hr) return hr;
		
		hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
		
		if(S_OK != hr) return hr;
	}

	if(p->mt.IsValid())
	{
		pSample->SetMediaType(&p->mt);

		p->flags |= CPacket::Discontinuity;

	    CAutoLock cAutoLock(m_pLock);

		m_mts.clear();
		m_mts.push_back(p->mt);
	}

	BYTE* data = NULL;

	hr = pSample->GetPointer(&data);
	
	if(S_OK != hr || data == NULL) return hr;
	
	memcpy(data, &p->buff[0], bytes);

	hr = pSample->SetActualDataLength(bytes);

	ASSERT((p->flags & CPacket::SyncPoint) == 0 || (p->flags & CPacket::TimeValid) != 0);

	if(p->flags & CPacket::TimeValid)
	{
		hr = pSample->SetTime(&p->start, &p->stop);
	}
	else
	{
		hr = pSample->SetTime(NULL, NULL);
	}

	hr = pSample->SetMediaTime(NULL, NULL);

	hr = pSample->SetDiscontinuity((p->flags & CPacket::Discontinuity) ? TRUE : FALSE);

	hr = pSample->SetSyncPoint((p->flags & CPacket::SyncPoint) ? TRUE : FALSE);

	hr = pSample->SetPreroll((p->flags & CPacket::TimeValid) && p->start < 0 ? TRUE : FALSE);

	return Deliver(pSample);
}

[uuid("48025243-2D39-11CE-875D-00608CB78066")]
class InternalScriptRenderer {};

void CBaseSplitterOutputPin::MakeISCRHappy()
{
	CComPtr<IPin> pPinTo = this;
	CComPtr<IPin> pTmp;

	while(pPinTo && SUCCEEDED(pPinTo->ConnectedTo(&pTmp)) && (pPinTo = pTmp))
	{
		pTmp = NULL;

		CComPtr<IBaseFilter> pBF = DirectShow::GetFilter(pPinTo);

		if(DirectShow::GetCLSID(pBF) == __uuidof(InternalScriptRenderer))
		{
			CPacket* p = new CPacket();

			p->id = (DWORD)-1;
			p->start = -1; p->stop = 0;
			p->flags |= CPacket::TimeValid;
			p->SetData(" ", 2);

			QueuePacket(p);

			break;
		}

		pPinTo = DirectShow::GetFirstPin(pBF, PINDIR_OUTPUT);
	}
}

HRESULT CBaseSplitterOutputPin::GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	return __super::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
}

HRESULT CBaseSplitterOutputPin::Deliver(IMediaSample* pSample)
{
	return __super::Deliver(pSample);
}

// IMediaSeeking

STDMETHODIMP CBaseSplitterOutputPin::GetCapabilities(DWORD* pCapabilities)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetCapabilities(pCapabilities);
}

STDMETHODIMP CBaseSplitterOutputPin::CheckCapabilities(DWORD* pCapabilities)
{
	return ((CBaseSplitterFilter*)m_pFilter)->CheckCapabilities(pCapabilities);
}

STDMETHODIMP CBaseSplitterOutputPin::IsFormatSupported(const GUID* pFormat)
{
	return ((CBaseSplitterFilter*)m_pFilter)->IsFormatSupported(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::QueryPreferredFormat(GUID* pFormat)
{
	return ((CBaseSplitterFilter*)m_pFilter)->QueryPreferredFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::GetTimeFormat(GUID* pFormat)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::IsUsingTimeFormat(const GUID* pFormat)
{
	return ((CBaseSplitterFilter*)m_pFilter)->IsUsingTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::SetTimeFormat(const GUID* pFormat)
{
	return ((CBaseSplitterFilter*)m_pFilter)->SetTimeFormat(pFormat);
}
STDMETHODIMP CBaseSplitterOutputPin::GetDuration(LONGLONG* pDuration)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetDuration(pDuration);
}

STDMETHODIMP CBaseSplitterOutputPin::GetStopPosition(LONGLONG* pStop)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetStopPosition(pStop);
}

STDMETHODIMP CBaseSplitterOutputPin::GetCurrentPosition(LONGLONG* pCurrent)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetCurrentPosition(pCurrent);
}

STDMETHODIMP CBaseSplitterOutputPin::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return ((CBaseSplitterFilter*)m_pFilter)->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return ((CBaseSplitterFilter*)m_pFilter)->SetPositionsInternal(this, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CBaseSplitterOutputPin::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetPositions(pCurrent, pStop);
}

STDMETHODIMP CBaseSplitterOutputPin::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetAvailable(pEarliest, pLatest);
}

STDMETHODIMP CBaseSplitterOutputPin::SetRate(double dRate)
{
	return ((CBaseSplitterFilter*)m_pFilter)->SetRate(dRate);
}

STDMETHODIMP CBaseSplitterOutputPin::GetRate(double* pdRate)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetRate(pdRate);
}

STDMETHODIMP CBaseSplitterOutputPin::GetPreroll(LONGLONG* pllPreroll)
{
	return ((CBaseSplitterFilter*)m_pFilter)->GetPreroll(pllPreroll);
}

// CBaseSplitterFilter

CBaseSplitterFilter::CBaseSplitterFilter(LPCTSTR pName, LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(pName, pUnk, this, clsid)
	, m_rtDuration(0), m_rtStart(0), m_rtStop(0), m_rtCurrent(0)
	, m_dRate(1.0)
	, m_nOpenProgress(100)
	, m_abort(false)
	, m_rtLastStart(_I64_MIN)
	, m_rtLastStop(_I64_MIN)
	, m_priority(THREAD_PRIORITY_NORMAL)
{
	if(phr) *phr = S_OK;

	m_pInput = new CBaseSplitterInputPin(NAME("CBaseSplitterInputPin"), this, phr);
}

CBaseSplitterFilter::~CBaseSplitterFilter()
{
	CAutoLock cAutoLock(this);

	CAMThread::CallWorker(CMD_EXIT);
	CAMThread::Close();

	delete m_pInput;

	DeleteOutputPins();

	for(auto i = m_pRetiredOutputs.begin(); i != m_pRetiredOutputs.end(); i++)
	{
		delete *i;
	}
}

STDMETHODIMP CBaseSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	if(m_pInput && riid == __uuidof(IFileSourceFilter)) 
	{
		return E_NOINTERFACE;
	}

	return 
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
		QI(IAMOpenProgress)
		QI2(IAMMediaContent)
		QI2(IAMExtendedSeeking)
		QI(IKeyFrameInfo)
		QI(IBufferInfo)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

CBaseSplitterOutputPin* CBaseSplitterFilter::GetOutputPin(DWORD id)
{
	CAutoLock cAutoLock(&m_csPinMap);

	auto pair = m_pPinMap.find(id);

	if(pair != m_pPinMap.end())
	{
		return pair->second;
	}

	return NULL;
}

DWORD CBaseSplitterFilter::GetOutputId(CBaseSplitterOutputPin* pPin)
{
	CAutoLock cAutoLock(&m_csPinMap);

	auto pair = std::find_if(m_pPinMap.begin(), m_pPinMap.end(), [&pPin] (const std::pair<DWORD, CBaseSplitterOutputPin*>& pair)
	{
		return pair.second == pPin;
	});

	if(pair != m_pPinMap.end())
	{
		return pair->first;
	}

	return 0xffffffff;
}

HRESULT CBaseSplitterFilter::RenameOutputPin(DWORD src, DWORD dst, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(&m_csPinMap);

	auto pair = m_pPinMap.find(src);

	if(pair != m_pPinMap.end())
	{
		CBaseSplitterOutputPin* pin = pair->second;

		if(IPin* pPinTo = pin->GetConnected())
		{
			if(pmt != NULL && S_OK != pPinTo->QueryAccept(pmt))
			{
				return VFW_E_TYPE_NOT_ACCEPTED;
			}
		}

		m_pPinMap.erase(pair);
		m_pPinMap[dst] = pin;

		if(pmt != NULL)
		{
			CAutoLock cAutoLock(&m_csmtnew);

			m_mtnew[dst] = *pmt;
		}

		return S_OK;
	}

	return E_FAIL;
}

HRESULT CBaseSplitterFilter::AddOutputPin(DWORD id, CBaseSplitterOutputPin* pPin)
{
	CAutoLock cAutoLock(&m_csPinMap);

	if(pPin == NULL) 
	{
		return E_INVALIDARG;
	}

	m_pPinMap[id] = pPin;
	
	m_pOutputs.push_back(pPin);

	return S_OK;
}

HRESULT CBaseSplitterFilter::DeleteOutputPins()
{
	m_rtDuration = 0;

	for(auto i = m_pRetiredOutputs.begin(); i != m_pRetiredOutputs.end(); i++)
	{
		delete *i;
	}

	m_pRetiredOutputs.clear();
	
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped) 
	{
		return VFW_E_NOT_STOPPED;
	}

	std::for_each(m_pOutputs.begin(), m_pOutputs.end(), [this] (CBaseSplitterOutputPin* pin)
	{
		if(IPin* pPinTo = pin->GetConnected())
		{
			pPinTo->Disconnect();
		}

		pin->Disconnect();

		m_pRetiredOutputs.push_back(pin);
	});

	m_pOutputs.clear();

	CAutoLock cAutoLock2(&m_csPinMap);

	m_pPinMap.clear();

	CAutoLock cAutoLock3(&m_csmtnew);

	m_mtnew.clear();

	RemoveAll();
	ResRemoveAll();
	ChapRemoveAll();

	m_font.Uninstall();

	m_reader.Release();

	return S_OK;
}

void CBaseSplitterFilter::DeliverBeginFlush()
{
	m_flushing = true;

	for(auto i = m_pOutputs.begin(); i != m_pOutputs.end(); i++)
	{
		(*i)->DeliverBeginFlush();
	}
}

void CBaseSplitterFilter::DeliverEndFlush()
{
	for(auto i = m_pOutputs.begin(); i != m_pOutputs.end(); i++)
	{
		(*i)->DeliverEndFlush();
	}

	m_flushing = false;

	m_endflush.Set();
}

DWORD CBaseSplitterFilter::ThreadProc()
{
	if(m_reader) 
	{
		m_reader->SetBreakEvent(GetRequestHandle());
	}

	if(!DemuxInit())
	{
		while(1)
		{
			bool exit = GetRequest() == CMD_EXIT;
			
			if(exit) 
			{
				CAMThread::m_hThread = NULL;
			}

			Reply(S_OK);

			if(exit)			
			{
				return 0;
			}
		}
	}

	m_endflush.Set();

	m_flushing = false;

	for(DWORD cmd = -1; ; cmd = GetRequest())
	{
		if(cmd == CMD_EXIT)
		{
			m_hThread = NULL;

			Reply(S_OK);

			return 0;
		}

		m_priority = THREAD_PRIORITY_NORMAL;

		SetThreadPriority(m_hThread, m_priority);

		m_rtStart = m_rtNewStart;
		m_rtStop = m_rtNewStop;

		DemuxSeek(m_rtStart);

		if(cmd != -1)
		{
			Reply(S_OK);
		}

		m_endflush.Wait();

		m_pActivePins.clear();

		for(auto i = m_pOutputs.begin(); i != m_pOutputs.end() && !m_flushing; i++)
		{
			CBaseSplitterOutputPin* pin = *i;

			if(pin->IsConnected() && pin->IsActive())
			{
				m_pActivePins.insert(pin);

				pin->DeliverNewSegment(m_rtStart, m_rtStop, m_dRate);
			}
		}

		do 
		{
			m_bDiscontinuitySent.clear();
		}
		while(!DemuxLoop());

		for(auto i = m_pActivePins.begin(); i != m_pActivePins.end() && !CheckRequest(&cmd); i++)
		{
			(*i)->QueueEndOfStream();
		}
	}

	ASSERT(0); // we should only exit via CMD_EXIT

	m_hThread = NULL;

	return 0;
}

HRESULT CBaseSplitterFilter::DeliverPacket(CPacket* p)
{
	HRESULT hr = S_FALSE;

	CBaseSplitterOutputPin* pin = GetOutputPin(p->id);

	if(pin == NULL || !pin->IsConnected() || m_pActivePins.find(pin) == m_pActivePins.end())
	{
		delete p;

		return S_FALSE;
	}

	if(p->flags & CPacket::TimeValid)
	{
		m_rtCurrent = p->start;

		p->start -= m_rtStart;
		p->stop -= m_rtStart;

		ASSERT(p->start <= p->stop);
	}

	{
		CAutoLock cAutoLock(&m_csmtnew);

		CMediaType mt;

		auto pair = m_mtnew.find(p->id);

		if(pair != m_mtnew.end())
		{
			p->mt = pair->second;

			m_mtnew.erase(pair);
		}
	}

	if(m_bDiscontinuitySent.find(p->id) == m_bDiscontinuitySent.end())
	{
		p->flags |= CPacket::Discontinuity;
	}

	DWORD id = p->id;

	bool discontinuity = (p->flags & CPacket::Discontinuity) != 0;

	hr = pin->QueuePacket(p);

	if(S_OK != hr)
	{
		m_pActivePins.erase(pin);

		// only die when all pins are down

		if(!m_pActivePins.empty())
		{
			hr = S_OK;
		}

		return hr;
	}

	if(discontinuity)
	{
		m_bDiscontinuitySent.insert(id);
	}

	return hr;
}

bool CBaseSplitterFilter::IsAnyPinDrying()
{
	int totalcount = 0, totalsize = 0;

	for(auto i = m_pActivePins.begin(); i != m_pActivePins.end(); i++)
	{
		CBaseSplitterOutputPin* pin = *i;

		int count = pin->QueueCount();
		int size = pin->QueueSize();

		if(!pin->IsDiscontinuous() && (count < MINPACKETS || size < MINPACKETSIZE))
		{
//			if(m_priority != THREAD_PRIORITY_ABOVE_NORMAL && (count < MINPACKETS / 3 || size < MINPACKETSIZE / 3))
			if(m_priority != THREAD_PRIORITY_BELOW_NORMAL && (count < MINPACKETS / 3 || size < MINPACKETSIZE / 3))
			{
				// SetThreadPriority(m_hThread, m_priority = THREAD_PRIORITY_ABOVE_NORMAL);

				std::for_each(m_pOutputs.begin(), m_pOutputs.end(), [] (CBaseSplitterOutputPin* pin)
				{
					pin->SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
				});

				m_priority = THREAD_PRIORITY_BELOW_NORMAL;
			}

			return true;
		}

		totalcount += count;
		totalsize += size;
	}

	if(m_priority != THREAD_PRIORITY_NORMAL && (totalcount > MAXPACKETS * 2 / 3 || totalsize > MAXPACKETSIZE * 2 / 3))
	{
//		SetThreadPriority(m_hThread, m_priority = THREAD_PRIORITY_NORMAL);

		std::for_each(m_pOutputs.begin(), m_pOutputs.end(), [] (CBaseSplitterOutputPin* pin)
		{
			pin->SetThreadPriority(THREAD_PRIORITY_NORMAL);
		});

		m_priority = THREAD_PRIORITY_NORMAL;
	}

	return totalcount < MAXPACKETS && totalsize < MAXPACKETSIZE;
}

HRESULT CBaseSplitterFilter::BreakConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		DeleteOutputPins();
	}
	else if(dir == PINDIR_OUTPUT)
	{
	}
	else
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT CBaseSplitterFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CBaseSplitterInputPin* pIn = (CBaseSplitterInputPin*)pPin;

		HRESULT hr;

		CComPtr<IAsyncReader> pAsyncReader;

		if(FAILED(hr = pIn->GetAsyncReader(&pAsyncReader))
		|| FAILED(hr = DeleteOutputPins())
		|| FAILED(hr = CreateOutputPins(pAsyncReader)))
		{
			return hr;
		}

		ChapSort();

		m_reader = pAsyncReader;
	}
	else if(dir == PINDIR_OUTPUT)
	{
		std::for_each(m_pRetiredOutputs.begin(), m_pRetiredOutputs.end(), [] (CBaseSplitterOutputPin* pin) {delete pin;});

		m_pRetiredOutputs.clear();
	}
	else
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

int CBaseSplitterFilter::GetPinCount()
{
	return (m_pInput ? 1 : 0) + m_pOutputs.size();
}

CBasePin* CBaseSplitterFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(m_pInput != NULL)
	{
		if(n == 0)
		{
			return m_pInput;
		}

		n--;
	}

	if(n >= 0 && n < m_pOutputs.size())
	{
		return m_pOutputs[n];
	}

	return NULL;
}

STDMETHODIMP CBaseSplitterFilter::Stop()
{
	CAutoLock cAutoLock(this);

	DeliverBeginFlush();

	CallWorker(CMD_EXIT);

	DeliverEndFlush();

	HRESULT hr;

	hr = __super::Stop();

	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::Pause()
{
	CAutoLock cAutoLock(this);

	bool stopped = m_State == State_Stopped;

	HRESULT hr;

	hr = __super::Pause();

	if(FAILED(hr))
	{
		return hr;
	}

	if(stopped)
	{
		Create();
	}

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CBaseSplitterFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CheckPointer(pszFileName, E_POINTER);

	HRESULT hr = E_FAIL;
	
	CComPtr<IAsyncReader> pAsyncReader = (IAsyncReader*)new CAsyncFileReader(pszFileName, hr);

	if(FAILED(hr))
	{
		return hr;
	}

	if(FAILED(hr = DeleteOutputPins()) 
	|| FAILED(hr = CreateOutputPins(pAsyncReader)))
	{
		return hr;
	}

	ChapSort();

	m_fn = pszFileName;
	m_reader = pAsyncReader;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.size() + 1) * sizeof(WCHAR))))
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_fn.c_str());

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CBaseSplitterFilter::GetCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	*pCapabilities = 
		AM_SEEKING_CanGetStopPos |
		AM_SEEKING_CanGetDuration |
		AM_SEEKING_CanSeekAbsolute |
		AM_SEEKING_CanSeekForwards |
		AM_SEEKING_CanSeekBackwards;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	if(*pCapabilities == 0)
	{
		return S_OK;
	}

	DWORD caps;

	GetCapabilities(&caps);

	if((caps & *pCapabilities) == 0)
	{
		return E_FAIL;
	}

	if(caps == *pCapabilities) 
	{
		return S_OK;
	}

	return S_FALSE;
}

STDMETHODIMP CBaseSplitterFilter::IsFormatSupported(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CBaseSplitterFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterFilter::GetTimeFormat(GUID* pFormat) 
{
	CheckPointer(pFormat, E_POINTER);

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CBaseSplitterFilter::SetTimeFormat(const GUID* pFormat) 
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CBaseSplitterFilter::GetDuration(LONGLONG* pDuration) 
{
	CheckPointer(pDuration, E_POINTER); 
	
	*pDuration = m_rtDuration; 
	
	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetStopPosition(LONGLONG* pStop) 
{
	return GetDuration(pStop);
}

STDMETHODIMP CBaseSplitterFilter::GetCurrentPosition(LONGLONG* pCurrent) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseSplitterFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseSplitterFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return SetPositionsInternal(this, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CBaseSplitterFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent) 
	{
		*pCurrent = m_rtCurrent;
	}

	if(pStop)
	{
		*pStop = m_rtStop;
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest)
	{
		*pEarliest = 0;
	}

	return GetDuration(pLatest);
}

STDMETHODIMP CBaseSplitterFilter::SetRate(double dRate)
{
	if(dRate > 0)
	{
		m_dRate = dRate;

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CBaseSplitterFilter::GetRate(double* pdRate) 
{
	CheckPointer(pdRate, E_POINTER);

	*pdRate = m_dRate;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetPreroll(LONGLONG* pllPreroll)
{
	CheckPointer(pllPreroll, E_POINTER);

	*pllPreroll = 0;

	return S_OK;
}

HRESULT CBaseSplitterFilter::SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	CAutoLock cAutoLock(this);

	if(pCurrent == NULL && pStop == NULL
	|| (dwCurrentFlags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
		return S_OK;

	REFERENCE_TIME rtCurrent = m_rtCurrent;
	REFERENCE_TIME rtStop = m_rtStop;

	if(pCurrent != NULL)
	{
		switch(dwCurrentFlags & AM_SEEKING_PositioningBitsMask)
		{
		case AM_SEEKING_NoPositioning: break;
		case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
		case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
		case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
		}
	}

	if(pStop != NULL)
	{
		switch(dwStopFlags & AM_SEEKING_PositioningBitsMask)
		{
		case AM_SEEKING_NoPositioning: break;
		case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
		case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
		case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
		}
	}

	if(m_rtCurrent == rtCurrent && m_rtStop == rtStop)
	{
		return S_OK;
	}

	if(m_rtLastStart == rtCurrent && m_rtLastStop == rtStop && std::find(m_LastSeekers.begin(), m_LastSeekers.end(), id) == m_LastSeekers.end())
	{
		m_LastSeekers.push_back(id);

		return S_OK;
	}

	m_rtLastStart = rtCurrent;
	m_rtLastStop = rtStop;

	m_LastSeekers.clear();
	m_LastSeekers.push_back(id);

	m_rtCurrent = rtCurrent;

	m_rtNewStart = rtCurrent;
	m_rtNewStop = rtStop;

	if(ThreadExists())
	{
		DeliverBeginFlush();

		CallWorker(CMD_SEEK);

		DeliverEndFlush();
	}

	return S_OK;
}

// IAMOpenProgress

STDMETHODIMP CBaseSplitterFilter::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	CheckPointer(pllTotal, E_POINTER);
	CheckPointer(pllCurrent, E_POINTER);

	*pllTotal = 100;
	*pllCurrent = m_nOpenProgress;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::AbortOperation()
{
	m_abort = true;

	return S_OK;
}

// IAMMediaContent

STDMETHODIMP CBaseSplitterFilter::get_AuthorName(BSTR* pbstrAuthorName)
{
	return GetProperty(L"AUTH", pbstrAuthorName);
}

STDMETHODIMP CBaseSplitterFilter::get_Title(BSTR* pbstrTitle)
{
	return GetProperty(L"TITL", pbstrTitle);
}

STDMETHODIMP CBaseSplitterFilter::get_Rating(BSTR* pbstrRating)
{
	return GetProperty(L"RTNG", pbstrRating);
}

STDMETHODIMP CBaseSplitterFilter::get_Description(BSTR* pbstrDescription)
{
	return GetProperty(L"DESC", pbstrDescription);
}

STDMETHODIMP CBaseSplitterFilter::get_Copyright(BSTR* pbstrCopyright)
{
	return GetProperty(L"CPYR", pbstrCopyright);
}

// IAMExtendedSeeking

STDMETHODIMP CBaseSplitterFilter::get_ExSeekCapabilities(long* pExCapabilities)
{
	CheckPointer(pExCapabilities, E_POINTER);

	*pExCapabilities = AM_EXSEEK_CANSEEK;

	if(ChapGetCount())
	{
		*pExCapabilities |= AM_EXSEEK_MARKERSEEK;
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::get_MarkerCount(long* pMarkerCount)
{
	CheckPointer(pMarkerCount, E_POINTER);

	*pMarkerCount = (long)ChapGetCount();

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::get_CurrentMarker(long* pCurrentMarker)
{
	CheckPointer(pCurrentMarker, E_POINTER);

	REFERENCE_TIME rt = m_rtCurrent;

	long i = ChapLookup(&rt);

	if(i < 0) return E_FAIL;

	*pCurrentMarker = i + 1;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetMarkerTime(long MarkerNum, double* pMarkerTime)
{
	CheckPointer(pMarkerTime, E_POINTER);

	REFERENCE_TIME rt;

	if(FAILED(ChapGet((int)MarkerNum - 1, &rt)))
	{
		return E_FAIL;
	}

	*pMarkerTime = (double)rt / 10000000;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetMarkerName(long MarkerNum, BSTR* pbstrMarkerName)
{
	return ChapGet((int)MarkerNum - 1, NULL, pbstrMarkerName);
}

// IKeyFrameInfo

STDMETHODIMP CBaseSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	return E_NOTIMPL;
}

// IBufferInfo

STDMETHODIMP_(int) CBaseSplitterFilter::GetCount()
{
	CAutoLock cAutoLock(m_pLock);

	return m_pOutputs.size();
}

STDMETHODIMP CBaseSplitterFilter::GetStatus(int i, int& samples, int& size)
{
	CAutoLock cAutoLock(m_pLock);

	if(i >= 0 && i < m_pOutputs.size())
	{
		CBaseSplitterOutputPin* pin = m_pOutputs[i];

		samples = pin->QueueCount();
		size = pin->QueueSize();

		return pin->IsConnected() ? S_OK : S_FALSE;
	}

	return E_INVALIDARG;
}

STDMETHODIMP_(DWORD) CBaseSplitterFilter::GetPriority()
{
    return m_priority;
}
