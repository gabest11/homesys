//
// MuxFilter.h
//
// Declaration of classes for DirectShow MPEG-4 Multiplexor filter
//
// Geraint Davies, May 2004
//
// Copyright (c) GDCL 2004-6. All Rights Reserved. 
// You are free to re-use this as the basis for your own filter development,
// provided you retain this copyright notice in the source.
// http://www.gdcl.co.uk
//////////////////////////////////////////////////////////////////////

#pragma once

//#include "../../3rdparty/baseclasses/baseclasses.h"
#include "MovieWriter.h"
#include <queue>

// forward declarations
class Mpeg4Mux;
class MuxInput;
class MuxOutput;
class MuxAllocator;

// input pin, receives data corresponding to one
// media track in the file.
// Pins are created and deleted dynamically to 
// ensure that there is always one unconnected pin.
class MuxInput 
: public CBaseInputPin,
  public IAMStreamControl
{
public:
    MuxInput(Mpeg4Mux* pFilter, CCritSec* pLock, HRESULT* phr, LPCWSTR pName, int index);

    // lifetime management for pins is normally delegated to the filter, but
    // we need to be able to create and delete them independently, so keep 
    // a separate refcount.
    STDMETHODIMP_(ULONG) NonDelegatingRelease()
    {
        return CUnknown::NonDelegatingRelease();
    }
    STDMETHODIMP_(ULONG) NonDelegatingAddRef()
    {
        return CUnknown::NonDelegatingAddRef();
    }

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void** ppv)
	{
		if (iid == IID_IAMStreamControl)
		{
			return GetInterface((IAMStreamControl*) this, ppv);
		}
		return __super::NonDelegatingQueryInterface(iid, ppv);
	}

    // CBasePin overrides
    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    
    // input
    STDMETHODIMP Receive(IMediaSample* pSample);
    STDMETHODIMP EndOfStream();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();

    // state change
    HRESULT Active();
    HRESULT Inactive();

    // connection management -- used to maintain one free pin
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin *pReceivePin);

	// IAMStreamControl methods
	STDMETHOD(StartAt)(const REFERENCE_TIME* ptStart, DWORD dwCookie);
	STDMETHOD(StopAt)(const REFERENCE_TIME* ptStop, BOOL bSendExtra, DWORD dwCookie);
	STDMETHOD(GetInfo)(AM_STREAM_INFO* pInfo);

private:
	bool ShouldDiscard(IMediaSample* pSample);

private:
    Mpeg4Mux* m_pMux;
    int m_index;
    TrackWriter* m_pTrack;

	CCritSec m_csStreamControl;
	AM_STREAM_INFO m_StreamInfo;
};


// output pin, writes multiplexed data downstream
// using IMemOutputPin while running, and then writes 
// metadata using IStream::Write when stopping.
class MuxOutput 
: public CBaseOutputPin,
  public AtomWriter
{
public:
    MuxOutput(Mpeg4Mux* pFilter, CCritSec* pLock, HRESULT* phr);

    // CBaseOutputPin overrides
    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES * pprop);

    // called from filter
    void Reset();
    void UseIStream();
    void FillSpace();

    // AtomWriter methods
    LONGLONG Length();
    LONGLONG Position();
    HRESULT Replace(LONGLONG pos, const BYTE* pBuffer, long cBytes);
    HRESULT Append(const BYTE* pBuffer, long cBytes);
private:
    Mpeg4Mux* m_pMux;
    CCritSec m_csWrite;
    bool m_bUseIStream;
    LONGLONG m_llBytes;
};

// To pass seeking calls upstream we must try all connected input pins.
// Where two input pins lead to the same splitter, only one will be
// allowed to SetTimeFormat at once, so we must call this on all
// pins and store them, then call SetTimeFormat(NULL) once the
// operation is complete.
// 
// This class manages that list of seekable pins. 
// It is also used for seeking calls that do not need to set 
// the time format.
class SeekingAggregator
{
public:
    SeekingAggregator(CBaseFilter* pFilter, bool bSetTimeFormat = false);
    ~SeekingAggregator();

    typedef std::list<IMediaSeeking*>::iterator iterator;
    iterator Begin()
    {
        return m_Pins.begin();
    }
    iterator End()
    {
        return m_Pins.end();
    }

private:
    bool m_bSetTimeFormat;
    std::list<IMediaSeeking*> m_Pins;

};



class DECLSPEC_UUID("5FD85181-E542-4e52-8D9D-5D613C30131B")
Mpeg4Mux 
: public CBaseFilter,
  public IMediaSeeking
{
public:
    Mpeg4Mux(LPUNKNOWN pUnk, HRESULT* phr);
    ~Mpeg4Mux();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void** ppv);

    // CBaseFilter methods
    int GetPinCount();
    CBasePin *GetPin(int n);
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();

    void CreateInput();

    // called from input pin
    void OnDisconnect(int index);
    void OnConnect(int index);
    bool CanReceive(const CMediaType* pmt);
    TrackWriter* MakeTrack(int index, const CMediaType* pmt);
    void OnEOS();
	REFERENCE_TIME Start() { return m_tStart;}

    // we implement IMediaSeeking to allow encoding
    // of specific portions of an input clip, and
    // to report progress via the current position.
    // Calls (apart from current position) are
    // passed upstream to any pins that support seeking
// IMediaSeeking
public:
    STDMETHODIMP GetCapabilities(DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities(DWORD * pCapabilities );
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                              LONGLONG    Source, const GUID * pSourceFormat );
    STDMETHODIMP SetPositions(LONGLONG * pCurrent, DWORD dwCurrentFlags
			, LONGLONG * pStop, DWORD dwStopFlags );
    STDMETHODIMP GetPositions(LONGLONG * pCurrent,
                              LONGLONG * pStop );
    STDMETHODIMP GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG * pllPreroll);

private:
    CCritSec m_csFilter;
    CCritSec m_csTracks;
    MuxOutput* m_pOutput;
    std::vector<MuxInput*> m_pInputs;
    smart_ptr<MovieWriter> m_pMovie;

    // for reporting (via GetCurrentPosition) after completion
    REFERENCE_TIME m_tWritten;
};

