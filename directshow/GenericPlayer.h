#pragma once

#include <time.h>
#include "FGManager.h"
#include "SubPassThruFilter.h"
#include "IVideoPresenter.h"

struct TrackType
{
	enum
	{
		None,
		Video,
		Audio,
		Subtitle,
	};
};

struct TrackInfo 
{
	CMediaType mt; 
	std::wstring name;
	std::wstring lang;
	std::wstring iso6392; 
	bool dxva;
	bool broadcom; 
	
	struct TrackInfo() 
		: dxva(false)
		, broadcom(false)
	{
	}
};

[uuid("73E05A71-8E4D-4378-B3DB-8063247C7BD4")]
interface IGenericPlayer : public IUnknown
{
	STDMETHOD(GetState)(OAFilterState& fs) PURE;
	STDMETHOD(GetCurrent)(REFERENCE_TIME& rt) PURE;
	STDMETHOD(GetAvailable)(REFERENCE_TIME& start, REFERENCE_TIME& stop) PURE;
	STDMETHOD(Seek)(REFERENCE_TIME rt) PURE;
	STDMETHOD(Play)() PURE;
	STDMETHOD(Pause)() PURE;
	STDMETHOD(Stop)() PURE;
	STDMETHOD(PlayPause)() PURE;
	STDMETHOD(Suspend)() PURE;
	STDMETHOD(Resume)() PURE;
	STDMETHOD(Prev)() PURE;
	STDMETHOD(Next)() PURE;
	STDMETHOD(FrameStep)() PURE;
	STDMETHOD(GetVolume)(float& volume) PURE;
	STDMETHOD(SetVolume)(float volume) PURE;
	STDMETHOD(GetTrackCount)(int type, int& count) PURE;
	STDMETHOD(GetTrackInfo)(int type, int index, TrackInfo& info) PURE;
	STDMETHOD(GetTrack)(int type, int& index) PURE;
	STDMETHOD(SetTrack)(int type, int index) PURE;
	STDMETHOD(SetEOF)() PURE;
	STDMETHOD(IsEOF)() PURE;
	STDMETHOD(Render)(const VideoPresenterParam& param) PURE;
	STDMETHOD(ReRender)() PURE;
	STDMETHOD(ResetDevice)(bool before) PURE;
	STDMETHOD(GetVideoTexture)(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src) PURE;
	STDMETHOD(SetVideoPosition)(const Vector4i& r) PURE;
	STDMETHOD(GetSubtitleTexture)(IDirect3DTexture9** ppt, const Vector2i& s, Vector4i& r, bool& bbox) PURE;
	STDMETHOD(OnMessage)(UINT message, WPARAM wParam, LPARAM lParam) PURE;
	STDMETHOD(GetEvent)(long* plEventCode, LONG_PTR* plParam1, LONG_PTR* plParam2) PURE;
	STDMETHOD(FreeEvent)(long lEventCode, LONG_PTR lParam1, LONG_PTR lParam2) PURE;
	STDMETHOD(Activate)(float volume) PURE;
	STDMETHOD(Deactivate)() PURE;
	STDMETHOD(GetFilterGraph)(IFilterGraph** ppFG) PURE;
	STDMETHOD(HasVideo)() PURE;
	STDMETHOD(Poke)() PURE;
};

class GenericPlayer 
	: public CUnknown
	, public IGenericPlayer
	, public CSubPassThruFilter::ICallback
{
	struct
	{
		//CCritSec lock;
		std::list<CAdapt<CComPtr<ISubStream>>> streams;
		CComPtr<ISubProvider> provider;
		CComPtr<ISubPicQueue> queue;
		int track;
		bool bbox;
	} m_sub;

	void LoadSubtitleStreams();
	void SetSubtitle(ISubStream* ss);

	// CSubPassThruFilter::ICallback

	void ReplaceSubtitle(ISubStream* ssold, ISubStream* ssnew);
	void InvalidateSubtitle(REFERENCE_TIME start, ISubStream* ss);

protected:
	CComQIPtr<IGraphBuilder2> m_graph;
	std::list<CAdapt<CComPtr<IPin>>> m_outputs;
	VideoPresenterParam m_param;
	CComPtr<IBaseFilter> m_renderer;
	CComPtr<IVideoPresenter> m_presenter;
	CComPtr<IPin> m_renderer_pin;
	OAFilterState m_state;
	OAFilterState m_tmp_state;
	REFERENCE_TIME m_tmp_pos;
	bool m_suspended;
	bool m_eof;
	REFERENCE_TIME m_pos;
	bool m_active;
	clock_t m_lastactivity;
	bool m_hasvideo;
	std::wstring m_cfgkey;

	HRESULT CreateRenderer();
	HRESULT GetOutputs(IBaseFilter* pBF);
	HRESULT NukeOutputs();
	IBaseFilter* GetAudioSwitcher();
	void LoadSubtitles(LPCWSTR path);
	bool LoadSubtitle(LPCWSTR path);

	virtual HRESULT Render(IPin* pin);

	virtual DWORD GetSourceConfig();
	virtual DWORD GetDecoderConfig();

public:
	GenericPlayer();
	virtual ~GenericPlayer();

	static bool IsSubtitleExt(LPCWSTR ext);

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
	STDMETHODIMP Suspend();
	STDMETHODIMP Resume();
	STDMETHODIMP Prev();
	STDMETHODIMP Next();
	STDMETHODIMP FrameStep();
	STDMETHODIMP GetVolume(float& volume);
	STDMETHODIMP SetVolume(float volume);
	STDMETHODIMP GetTrackCount(int type, int& count);
	STDMETHODIMP GetTrackInfo(int type, int index, TrackInfo& info);
	STDMETHODIMP GetTrack(int type, int& index);
	STDMETHODIMP SetTrack(int type, int index);
	STDMETHODIMP SetEOF();
	STDMETHODIMP IsEOF();
	STDMETHODIMP Render(const VideoPresenterParam& param);
	STDMETHODIMP ReRender();
	STDMETHODIMP ResetDevice(bool before);
	STDMETHODIMP GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src);
	STDMETHODIMP SetVideoPosition(const Vector4i& r);
	STDMETHODIMP GetSubtitleTexture(IDirect3DTexture9** ppt, const Vector2i& s, Vector4i& r, bool& bbox);
	STDMETHODIMP OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
	STDMETHODIMP GetEvent(long* plEventCode, LONG_PTR* plParam1, LONG_PTR* plParam2);
	STDMETHODIMP FreeEvent(long lEventCode, LONG_PTR lParam1, LONG_PTR lParam2);
	STDMETHODIMP Activate(float volume);
	STDMETHODIMP Deactivate();
	STDMETHODIMP GetFilterGraph(IFilterGraph** ppFG);
	STDMETHODIMP HasVideo();
	STDMETHODIMP Poke();
};

[uuid("7E7A5A11-7DB8-4610-933A-BE780FC1BF47")]
interface IFilePlayer : public IUnknown
{
	STDMETHOD_(LPCWSTR, GetFileName)() = 0;
};

[uuid("B13CD466-AF17-4D7C-8C25-F476224301C7")]
interface ISubtitleFilePlayer : public IUnknown
{
	STDMETHOD (AddSubtitle)(LPCWSTR fn) = 0;
};

class FilePlayer 
	: public GenericPlayer
	, public IFilePlayer
	, public ISubtitleFilePlayer
{
	std::wstring m_fn;
	DWORD m_hash;

	static DWORD HashFile(LPCWSTR path);

public:
	FilePlayer(LPCWSTR path, LPCWSTR mime, HRESULT& hr);
	virtual ~FilePlayer();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPlayer

	STDMETHODIMP Stop();

	// IFilePlayer

	STDMETHODIMP_(LPCWSTR) GetFileName();

	// ISubtitleFilePlayer

	STDMETHODIMP AddSubtitle(LPCWSTR fn);
};

[uuid("BE53AD41-D06D-40CA-B9FE-75A8604E1AED")]
interface IDVDPlayer : public IUnknown
{
	STDMETHOD(Menu)(DVD_MENU_ID id) PURE;
	STDMETHOD(GetDVDDirectory)(std::wstring& dir) PURE;
};

class DVDPlayer : public GenericPlayer, public IDVDPlayer
{
	CComQIPtr<IDvdControl2> m_control;
	CComQIPtr<IDvdInfo2> m_info;
	
	HRESULT InTitle();

public:
	DVDPlayer(LPCWSTR path, HRESULT& hr);
	virtual ~DVDPlayer();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPlayer

	STDMETHODIMP GetCurrent(REFERENCE_TIME& rt);
	STDMETHODIMP GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop);
	STDMETHODIMP Seek(REFERENCE_TIME rt);
	STDMETHODIMP Play();
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();
	STDMETHODIMP Prev();
	STDMETHODIMP Next();
	STDMETHODIMP OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

	// IDVDPlayer

	STDMETHODIMP Menu(DVD_MENU_ID id);
	STDMETHODIMP GetDVDDirectory(std::wstring& dir);
};

[uuid("F5B86C44-0EED-4E56-BC84-5734664DF216")]
interface IPlaylistPlayer : public IFilePlayer
{
};

class PlaylistPlayer 
	: public GenericPlayer
	, public IPlaylistPlayer
{
	std::list<std::wstring>::iterator m_pos;
	std::list<std::wstring> m_files;

	HRESULT Open(int step);

public:
	PlaylistPlayer(LPCWSTR path, HRESULT& hr);
	virtual ~PlaylistPlayer();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPlayer

	STDMETHODIMP Prev();
	STDMETHODIMP Next();

	// IPlaylistPlayer

	STDMETHODIMP_(LPCWSTR) GetFileName();
};
