#pragma once

#include "IVideoPresenter.h"
#include <evr9.h>

[uuid("BC8AA335-3D1E-4FC8-879B-361418AE0024")]
class CEVRMixerPresenter
	: public CUnknown
	, public CCritSec
	, public CAMThread
	, public IMFVideoPresenter
	, public IMFGetService
	, public IMFTopologyServiceLookupClient
	, public IMFVideoDeviceID
	, public IMFVideoDisplayControl
	, public IEVRTrustedVideoPlugin
	, public IVideoPresenter
{
protected:
	enum RENDER_STATE
	{
	    RENDER_STATE_STARTED = 1,
	    RENDER_STATE_STOPPED,
	    RENDER_STATE_PAUSED,
	    RENDER_STATE_SHUTDOWN,
	};
 
	enum FRAMESTEP_STATE 
	{
		FRAMESTEP_NONE,             // Not frame stepping. 
		FRAMESTEP_WAITING_START,    // Frame stepping, but the clock is not started. 
		FRAMESTEP_PENDING,          // Clock is started. Waiting for samples. 
		FRAMESTEP_SCHEDULED,        // Submitted a sample for rendering. 
		FRAMESTEP_COMPLETE          // Sample was rendered.  
	};

	VideoPresenterParam m_param; 
	UINT m_token;
	CComPtr<IDirect3DDeviceManager9> m_manager;
	CComPtr<IMFClock> m_clock;
	CComPtr<IMFTransform> m_mixer;
	CComPtr<IMediaEventSink> m_sink;
	CComPtr<IMFMediaType> m_mt;
	std::list<IMFSample*> m_samples;
	std::list<IMFSample*> m_scheduled;
	CComPtr<IMFSample> m_current;
	RENDER_STATE m_state;
	LONGLONG m_error;
	bool m_streaming;
	bool m_eos;
	bool m_prerolled;
	bool m_stepping;
	int m_steps;
	int m_late;
	bool m_first;
	Vector2i m_size;
	Vector2i m_ar;
	RECT m_dst;
	double m_fps;
	FILE* m_log;

	HRESULT GetTexture(IMFSample* sample, IDirect3DTexture9** ppt);

	HRESULT RenegotiateMediaType();
	HRESULT SetMediaType(IMFMediaType* mt);
	HRESULT BeginStreaming();
	HRESULT EndStreaming();
	HRESULT ProcessInputNotify();
	HRESULT Flush();
	HRESULT Step(int steps);
	HRESULT CancelStep();
	HRESULT CheckEndOfStream();

	HRESULT AllocSamples();
	void FreeSamples();
	
	DWORD ThreadProc();

public:
	CEVRMixerPresenter(const VideoPresenterParam& param, HRESULT& hr);
	virtual ~CEVRMixerPresenter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	static HRESULT CreateInstance(const VideoPresenterParam& param, IBaseFilter** ppBF, IVideoPresenter** ppVP);

	// IMFClockStateSink

	STDMETHODIMP OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
    STDMETHODIMP OnClockStop(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockPause(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockRestart(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockSetRate(MFTIME hnsSystemTime, float flRate);

	// IMFVideoPresenter

	STDMETHODIMP ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
	STDMETHODIMP GetCurrentMediaType(IMFVideoMediaType** ppMediaType);

	// IMFGetService
	
	STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID* ppvObject);

	// IMFTopologyServiceLookupClient

	STDMETHODIMP InitServicePointers(IMFTopologyServiceLookup* pLookup);
	STDMETHODIMP ReleaseServicePointers();

	// IMFVideoDeviceID

	STDMETHODIMP GetDeviceID(IID* pDeviceID);

	// IMFVideoDisplayControl

    STDMETHODIMP GetNativeVideoSize(SIZE* pszVideo, SIZE* pszARVideo);    
    STDMETHODIMP GetIdealVideoSize(SIZE* pszMin, SIZE* pszMax);
    STDMETHODIMP SetVideoPosition(const MFVideoNormalizedRect* pnrcSource, const LPRECT prcDest);
    STDMETHODIMP GetVideoPosition(MFVideoNormalizedRect* pnrcSource, LPRECT prcDest);
    STDMETHODIMP SetAspectRatioMode(DWORD dwAspectRatioMode);
    STDMETHODIMP GetAspectRatioMode(DWORD* pdwAspectRatioMode);
    STDMETHODIMP SetVideoWindow(HWND hwndVideo);
    STDMETHODIMP GetVideoWindow(HWND* phwndVideo);
    STDMETHODIMP RepaintVideo();
    STDMETHODIMP GetCurrentImage(BITMAPINFOHEADER* pBih, BYTE** pDib, DWORD* pcbDib, LONGLONG* pTimeStamp);
    STDMETHODIMP SetBorderColor(COLORREF Clr);
    STDMETHODIMP GetBorderColor(COLORREF* pClr);
    STDMETHODIMP SetRenderingPrefs(DWORD dwRenderFlags);
    STDMETHODIMP GetRenderingPrefs(DWORD* pdwRenderFlags);
    STDMETHODIMP SetFullscreen(BOOL fFullscreen);
    STDMETHODIMP GetFullscreen(BOOL* pfFullscreen);

	// IEVRTrustedVideoPlugin

	STDMETHODIMP IsInTrustedVideoMode(BOOL* pYes);
    STDMETHODIMP CanConstrict(BOOL* pYes);
    STDMETHODIMP SetConstriction(DWORD dwKPix);
    STDMETHODIMP DisableImageExport(BOOL bDisable);

	// IVideoPresenter

	STDMETHODIMP GetVideoTexture(IDirect3DTexture9** ppt, Vector2i* ar, Vector4i* src);
	STDMETHODIMP SetFlipEvent(HANDLE hFlipEvent);
	STDMETHODIMP GetFPS(double& fps);
};
