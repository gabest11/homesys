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
#include "ThreadSafeQueue.h"
#include "DSMPropertyBag.h"
#include "BitStream.h"
#include <list>
#include <vector>

class CBaseMuxerInputPin;
class CBaseMuxerFilter;

class CMuxerPacket
{
public:
	DWORD packet;
	DWORD flags;
	CBaseMuxerInputPin* pin;
	REFERENCE_TIME start, stop;
	std::vector<BYTE> buff;

	enum 
	{
		None = 0, 
		TimeValid = 1, 
		SyncPoint = 2, 
		Discontinuity = 4, 
		EoS = 8, 
		Bogus = 16
	};

	CMuxerPacket(CBaseMuxerInputPin* p) {pin = p; packet = ~0; flags = None; start = stop = _I64_MIN;}

	bool IsTimeValid() const {return (flags & TimeValid) != 0;}
	bool IsSyncPoint() const {return (flags & SyncPoint) != 0;}
	bool IsDiscontinuity() const {return (flags & Discontinuity) != 0;}
	bool IsEOS() const {return (flags & EoS) != 0;}
	bool IsBogus() const {return (flags & Bogus) != 0;}
};

[uuid("EE6F2741-7DB4-4AAD-A3CB-545208EE4C0A")]
interface IBaseMuxerRelatedPin : public IUnknown
{
	STDMETHOD(SetRelatedPin) (CBasePin* pPin) = 0;
	STDMETHOD_(CBasePin*, GetRelatedPin) () = 0;
};

class CBaseMuxerRelatedPin : public IBaseMuxerRelatedPin
{
	CBasePin* m_other; // should not hold a reference because it would be circular

public:
	CBaseMuxerRelatedPin();

	// IBaseMuxerRelatedPin

	STDMETHODIMP SetRelatedPin(CBasePin* pPin);
	STDMETHODIMP_(CBasePin*) GetRelatedPin();
};

class CBaseMuxerInputPin : public CBaseInputPin, public CBaseMuxerRelatedPin, public IDSMPropertyBagImpl
{
private:
	int m_id;
	REFERENCE_TIME m_current;
	REFERENCE_TIME m_duration;
	bool m_eos;
	int m_packet;
	CCritSec m_csReceive;

	CThreadSafeQueue<CMuxerPacket*> m_queue;

	friend class CBaseMuxerFilter;

public:
	CBaseMuxerInputPin(LPCWSTR pName, CBaseMuxerFilter* pFilter, HRESULT* phr);
	virtual ~CBaseMuxerInputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	REFERENCE_TIME GetDuration() {return m_duration;}
	int GetID() {return m_id;}

	bool IsSubtitleStream();

    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin* pReceivePin);

	HRESULT Active();
	HRESULT Inactive();

	STDMETHODIMP Receive(IMediaSample* pSample);
    STDMETHODIMP EndOfStream();
};

class CBaseMuxerOutputPin : public CBaseOutputPin
{
	CComPtr<IBitStream> m_stream;

public:
	CBaseMuxerOutputPin(LPCWSTR pName, CBaseMuxerFilter* pFilter, HRESULT* phr);
	virtual ~CBaseMuxerOutputPin() {}

	IBitStream* GetBitStream();

	HRESULT BreakConnect();

	HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

    HRESULT DeliverEndOfStream();

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};

class CBaseMuxerRawOutputPin : public CBaseMuxerOutputPin, public CBaseMuxerRelatedPin
{
	struct IndexEntry {REFERENCE_TIME rt; __int64 fp;};

	std::list<IndexEntry> m_index;

public:
	CBaseMuxerRawOutputPin(LPCWSTR pName, CBaseMuxerFilter* pFilter, HRESULT* phr);
	virtual ~CBaseMuxerRawOutputPin() {}

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	virtual void MuxHeader(const CMediaType& mt);
	virtual void MuxPacket(const CMediaType& mt, const CMuxerPacket* p);
	virtual void MuxFooter(const CMediaType& mt);
};

class CBaseMuxerFilter
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IMediaSeeking
	, public IDSMPropertyBagImpl
	, public IDSMResourceBagImpl
	, public IDSMChapterBagImpl
{
	std::vector<CBaseMuxerInputPin*> m_pInputs;
	CBaseMuxerOutputPin* m_pOutput;
	std::vector<CBaseMuxerRawOutputPin*> m_pRawOutputs;
	std::list<CBaseMuxerInputPin*> m_pActivePins;
	REFERENCE_TIME m_current;

	enum {CMD_EXIT, CMD_RUN};

	DWORD ThreadProc();

	CMuxerPacket* GetPacket();

	void MuxHeaderInternal();
	void MuxPacketInternal(const CMuxerPacket* p);
	void MuxFooterInternal();

protected:
	std::list<CBaseMuxerInputPin*> m_pins;

	CBaseMuxerOutputPin* GetOutputPin() const {return m_pOutput;}

	virtual void MuxInit() = 0;

	// only called when the output pin is connected

	virtual void MuxHeader(IBitStream* pBS) {}
	virtual void MuxPacket(IBitStream* pBS, const CMuxerPacket* p) {}
	virtual void MuxFooter(IBitStream* pBS) {}

	// always called (useful if the derived class wants to write somewhere else than downstream)

	virtual void MuxHeader() {}
	virtual void MuxPacket(const CMuxerPacket* p) {}
	virtual void MuxFooter() {}

	// allows customized pins in derived classes

	virtual HRESULT CreateInput(LPCWSTR name, CBaseMuxerInputPin** ppPin);
	virtual HRESULT CreateRawOutput(LPCWSTR name, CBaseMuxerRawOutputPin** ppPin);

public:
	CBaseMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid);
	virtual ~CBaseMuxerFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	void AddInput();

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();

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

