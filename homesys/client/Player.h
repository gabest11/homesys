#pragma once

#include "IFlash.h"
#include "../../DirectShow/DirectShow.h"
#include "../../DirectShow/GenericPlayer.h"

[uuid("AABC5F5F-B7E8-4BD2-8A3C-4BF79E3C31C4")]
interface ITunerPlayer : public IUnknown
{
	STDMETHOD_(GUID, GetTunerId)() PURE;
	STDMETHOD(SetPreset)(int presetId) PURE;
	STDMETHOD(SetPreviousPreset)() PURE;
	STDMETHOD(Reactivate)() PURE;
	STDMETHOD(Update)() PURE;
};

class TunerPlayer 
	: public GenericPlayer
	, public ITunerPlayer
	, public CCritSec
{
	CComPtr<IBaseFilter> m_source;

protected:
	GUID m_tunerId;
	bool m_overscan;
	bool m_disabled;
	DXUI::Thread m_thread;
	Homesys::TimeshiftFile m_tf;
	Homesys::Channel m_channel;
	REFERENCE_TIME m_paused_pos;
	clock_t m_paused_at;
	int m_prevPresetId;

	void StepPreset(int dir);
	void UpdateDisabled();

	HRESULT Render(IPin* pin);

	bool OnTimer(DXUI::Control* c, UINT64 n);

public:
	TunerPlayer(const GUID& tunerId, HRESULT& hr);
	virtual ~TunerPlayer();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPlayer

	STDMETHODIMP GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop);
	STDMETHODIMP Stop();
	STDMETHODIMP Prev();
	STDMETHODIMP Next();
	STDMETHODIMP ReRender();
	STDMETHODIMP GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src);
	STDMETHODIMP Activate(float volume);

	// ITunerPlayer
	
	STDMETHODIMP_(GUID) GetTunerId();
	STDMETHODIMP SetPreset(int presetId);
	STDMETHODIMP SetPreviousPreset();
	STDMETHODIMP Reactivate();
	STDMETHODIMP Update();
};

[uuid("3FEA4E31-4C51-4EC5-88DD-726D72925A04")]
interface IImagePlayer : public IUnknown
{
	STDMETHOD_(LPCWSTR, GetFileName)() = 0;
	STDMETHOD_(int, GetWidth)() = 0;
	STDMETHOD_(int, GetHeight)() = 0;
};

class ImagePlayer 
	: public CUnknown
	, public IGenericPlayer
	, public IImagePlayer
{
	std::list<std::wstring> m_items;
	std::list<std::wstring>::iterator m_pos;
	Util::ImageDecompressor m_id;
	VideoPresenterParam m_param;
	CComPtr<IDirect3DTexture9> m_texture;
	float m_volume;
	std::wstring m_file;
	clock_t m_start;
	OAFilterState m_state;
	DXUI::Thread m_thread;
	std::list<int> m_messages;

	HRESULT Open(int step);

public:
	ImagePlayer(LPCWSTR path, HRESULT& hr);
	virtual ~ImagePlayer();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPlayer

	STDMETHODIMP GetState(OAFilterState& fs);
	STDMETHODIMP GetCurrent(REFERENCE_TIME& rt);
	STDMETHODIMP GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop);
	STDMETHODIMP Seek(REFERENCE_TIME rt);
	STDMETHODIMP Play();
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();
	STDMETHODIMP PlayPause();
	STDMETHODIMP Suspend() {return E_NOTIMPL;}
	STDMETHODIMP Resume() {return E_NOTIMPL;}
	STDMETHODIMP Prev();
	STDMETHODIMP Next();
	STDMETHODIMP FrameStep();
	STDMETHODIMP GetVolume(float& volume);
	STDMETHODIMP SetVolume(float volume);
	STDMETHODIMP GetTrackCount(int type, int& count);
	STDMETHODIMP GetTrackInfo(int type, int index, TrackInfo& info);
	STDMETHODIMP GetTrack(int type, int& index);
	STDMETHODIMP SetTrack(int type, int index);
	STDMETHODIMP SetEOF() {return S_OK;}
	STDMETHODIMP IsEOF() {return S_FALSE;}
	STDMETHODIMP Render(const VideoPresenterParam& param);
	STDMETHODIMP ReRender();
	STDMETHODIMP ResetDevice(bool before);
	STDMETHODIMP GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src);
	STDMETHODIMP SetVideoPosition(const Vector4i& r);
	STDMETHODIMP GetSubtitleTexture(IDirect3DTexture9** ppt, const Vector2i& s, Vector4i& r, bool& bbox) {return E_NOTIMPL;}
	STDMETHODIMP OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
	STDMETHODIMP GetEvent(long* plEventCode, LONG_PTR* plParam1, LONG_PTR* plParam2);
	STDMETHODIMP FreeEvent(long lEventCode, LONG_PTR lParam1, LONG_PTR lParam2);
	STDMETHODIMP Activate(float volume);
	STDMETHODIMP Deactivate();
	STDMETHODIMP GetFilterGraph(IFilterGraph** ppFG);
	STDMETHODIMP HasVideo() {return S_OK;}
	STDMETHODIMP Poke() {return E_NOTIMPL;}
	STDMETHODIMP DrawSubtitle(const Vector4i& wr, const Vector4i& vr) {return E_NOTIMPL;}

	// IImagePlayer

	STDMETHODIMP_(LPCWSTR) GetFileName();
	STDMETHODIMP_(int) GetWidth();
	STDMETHODIMP_(int) GetHeight();
};

[uuid("1C6044E1-1161-4C7B-AB61-5212268B13D2")]
interface IFlashPlayer : public IGenericPlayer
{
};

[uuid("d27cdb6d-ae6d-11cf-96b8-444553540000")]
interface __IShockwaveFlashEvents : public IDispatch
{
	STDMETHOD(OnReadyStateChange)(long newState) PURE;
	STDMETHOD(OnProgress)(long percentDone) PURE;
	STDMETHOD(FSCommand)(BSTR command, BSTR args) PURE;
	STDMETHOD(FlashCall)(BSTR request) PURE;
};

class FlashPlayer 
	: public AlignedClass<16>
	, public CUnknown
	, public IFlashPlayer
	, public IFilePlayer
	, public __IShockwaveFlashEvents
{
	HWND m_wnd;
	VideoPresenterParam m_param;
	CComPtr<IDirect3DTexture9> m_texture;

	CComPtr<IShockwaveFlash> m_flash;
	CComQIPtr<IViewObject> m_view;
	CComQIPtr<IViewObject> m_rtview;
	CComPtr<IStream> m_stream;
	CComPtr<IConnectionPoint> m_conpt;
	DWORD m_conptid;

	HWND m_parent;
	std::wstring m_path;
	OAFilterState m_state;
	Vector4i m_rect1;
	Vector4i m_rect2;
	bool m_loaded;
	int m_frames;

	bool Open();
	void Close();

	DXUI::Thread m_thread;

	bool OnThreadInit(DXUI::Control* c);
	bool OnThreadExit(DXUI::Control* c);
	bool OnThreadMessage(DXUI::Control* c, const MSG& msg);
	bool OnThreadTimer(DXUI::Control* c, UINT64 n);

public:
	FlashPlayer(LPCWSTR path, HWND parent, HRESULT& hr);
	virtual ~FlashPlayer();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPlayer

	STDMETHODIMP GetState(OAFilterState& fs);
	STDMETHODIMP GetCurrent(REFERENCE_TIME& rt);
	STDMETHODIMP GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop);
	STDMETHODIMP Seek(REFERENCE_TIME rt);
	STDMETHODIMP Play();
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();
	STDMETHODIMP PlayPause();
	STDMETHODIMP Suspend() {return E_NOTIMPL;}
	STDMETHODIMP Resume() {return E_NOTIMPL;}
	STDMETHODIMP Prev() {return E_NOTIMPL;}
	STDMETHODIMP Next() {return E_NOTIMPL;}
	STDMETHODIMP FrameStep() {return E_NOTIMPL;}
	STDMETHODIMP GetVolume(float& volume);
	STDMETHODIMP SetVolume(float volume);
	STDMETHODIMP GetTrackCount(int type, int& count) {return E_NOTIMPL;}
	STDMETHODIMP GetTrackInfo(int type, int index, TrackInfo& info) {return E_NOTIMPL;}
	STDMETHODIMP GetTrack(int type, int& index) {return E_NOTIMPL;}
	STDMETHODIMP SetTrack(int type, int index) {return E_NOTIMPL;}
	STDMETHODIMP SetEOF() {return S_OK;}
	STDMETHODIMP IsEOF() {return S_FALSE;}
	STDMETHODIMP Render(const VideoPresenterParam& param);
	STDMETHODIMP ReRender();
	STDMETHODIMP ResetDevice(bool before);
	STDMETHODIMP GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src);
	STDMETHODIMP SetVideoPosition(const Vector4i& r);
	STDMETHODIMP GetSubtitleTexture(IDirect3DTexture9** ppt, const Vector2i& s, Vector4i& r, bool& bbox) {return E_NOTIMPL;}
	STDMETHODIMP OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
	STDMETHODIMP GetEvent(long* plEventCode, LONG_PTR* plParam1, LONG_PTR* plParam2);
	STDMETHODIMP FreeEvent(long lEventCode, LONG_PTR lParam1, LONG_PTR lParam2);
	STDMETHODIMP Activate(float volume);
	STDMETHODIMP Deactivate();
	STDMETHODIMP GetFilterGraph(IFilterGraph** ppFG);
	STDMETHODIMP HasVideo() {return S_OK;}
	STDMETHODIMP Poke() {return E_NOTIMPL;}
	STDMETHODIMP DrawSubtitle(const Vector4i& wr, const Vector4i& vr) {return E_NOTIMPL;}

	// IFilePlayer

	STDMETHODIMP_(LPCWSTR) GetFileName();

	// __IShockwaveFlashEvents

	STDMETHODIMP OnReadyStateChange(long newState);
	STDMETHODIMP OnProgress(long percentDone);
	STDMETHODIMP FSCommand(BSTR command, BSTR args);
	STDMETHODIMP FlashCall(BSTR request);

	// IDispatch

	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) {return S_OK;}
};


