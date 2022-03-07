#pragma once

#include "FGFilter.h"
#include "IGraphBuilder2.h"
#include "VMR9AllocatorPresenter.h"
#include "FontInstaller.h"

enum
{
	SRC_CDDA = 1 << 0, 
	SRC_CDXA = 1 << 1,
	SRC_VTS = 1 << 2,
	SRC_FLIC = 1 << 3,
	SRC_D2V = 1 << 4,
	SRC_DTSAC3 = 1 << 5,
	SRC_MATROSKA = 1 << 6,
	SRC_SHOUTCAST = 1 << 7,
	SRC_REALMEDIA = 1 << 8,
	SRC_AVI = 1 << 9,
	SRC_RADGT = 1 << 10,
	SRC_ROQ = 1 << 11,
	SRC_OGG = 1 << 12,
	SRC_NUT = 1 << 13,
	SRC_MPEG = 1 << 14,
	SRC_DIRAC = 1 << 15,
	SRC_MPA = 1 << 16,
	SRC_DSM = 1 << 17,
	SRC_SUBS = 1 << 18,
	SRC_MP4 = 1 << 19,
	SRC_FLV = 1 << 20,
	SRC_HTTP = 1 << 21,
	SRC_TS = 1 << 22,
	SRC_FLAC = 1 << 23,
	SRC_LAST = 1 << 24,
};

enum
{
	TRA_MPEG1 = 1 << 0,
	TRA_MPEG2 = 1 << 1,
	TRA_RV = 1 << 2,
	TRA_RA = 1 << 3,
	TRA_MPA = 1 << 4,
	TRA_LPCM = 1 << 5,
	TRA_AC3 = 1 << 6,
	TRA_DTS = 1 << 7,
	TRA_AAC = 1 << 8,
	TRA_PS2AUD = 1 << 9,
	TRA_DIRAC = 1 << 10,
	TRA_VORBIS = 1 << 11,
	TRA_FLV4 = 1 << 12,
	TRA_VP62 = 1 << 13,
	TRA_H264 = 1 << 14,
	TRA_DXVA = 1 << 15,
	TRA_FLAC = 1 << 16,
	TRA_LAST = 1 << 17,
};

class CFGManager
	: public CUnknown
	, public IGraphBuilder2
	, public CCritSec
{
private:
	CComPtr<IUnknown> m_inner;
	DWORD m_cookie;
	CFontInstaller m_fontinstaller;

protected:
	CComPtr<IFilterMapper2> m_fm;
	std::list<CAdapt<CComPtr<IUnknown>>> m_unks;
	std::list<CFGFilter*> m_source;
	std::list<CFGFilter*> m_transform;
	std::list<CFGFilter*> m_override;

	HRESULT EnumSourceFilters(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrMime, CFGFilterList& filters);
	HRESULT AddSourceFilter(CFGFilter* pFGF, LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppBF);
	void SplitFilename(LPCWSTR str, std::wstring& protocol, std::wstring& filename, std::wstring& extension);
	HRESULT Connect(IPin* pPinOut, IPin* pPinIn, bool recursive);

	// IFilterGraph

	STDMETHODIMP AddFilter(IBaseFilter* pFilter, LPCWSTR pName);
	STDMETHODIMP RemoveFilter(IBaseFilter* pFilter);
	STDMETHODIMP EnumFilters(IEnumFilters** ppEnum);
	STDMETHODIMP FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter);
	STDMETHODIMP ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP Reconnect(IPin* ppin);
	STDMETHODIMP Disconnect(IPin* ppin);
	STDMETHODIMP SetDefaultSyncSource();

	// IGraphBuilder

	STDMETHODIMP Connect(IPin* pPinOut, IPin* pPinIn);
	STDMETHODIMP Render(IPin* pPinOut);
	STDMETHODIMP RenderFile(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrPlayList);
	STDMETHODIMP AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);
	STDMETHODIMP SetLogFile(DWORD_PTR hFile);
	STDMETHODIMP Abort();
	STDMETHODIMP ShouldOperationContinue();

	// IFilterGraph2

	STDMETHODIMP AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);
	STDMETHODIMP ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP RenderEx(IPin* pPinOut, DWORD dwFlags, DWORD* pvContext);

	// IGraphBuilder2

	STDMETHODIMP RenderFileMime(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrMimeType);
	STDMETHODIMP AddSourceFilterMime(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, LPCWSTR lpcwstrMimeType, IBaseFilter** ppFilter);
	STDMETHODIMP HasRegisteredSourceFilter(LPCWSTR fn);
	STDMETHODIMP AddFilterForDisplayName(LPCWSTR dispname, IBaseFilter** ppBF);
	STDMETHODIMP NukeDownstream(IUnknown* pUnk);
	STDMETHODIMP ConnectFilter(IPin* pPinOut);
	STDMETHODIMP ConnectFilter(IBaseFilter* pBF, IPin* pPinIn);
	STDMETHODIMP ConnectFilter(IPin* pPinOut, IBaseFilter* pBF);
	STDMETHODIMP ConnectFilterDirect(IBaseFilter* pBFOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP FindInterface(REFIID iid, void** ppv, bool remove);
	STDMETHODIMP AddToROT();
	STDMETHODIMP RemoveFromROT();
	STDMETHODIMP Reset();
	STDMETHODIMP Clean();
	STDMETHODIMP Dump();
	STDMETHODIMP GetVolume(float& volume);
	STDMETHODIMP SetVolume(float volume);
	STDMETHODIMP GetBalance(float& balance);
	STDMETHODIMP SetBalance(float balance);

public:
	CFGManager(LPCTSTR name, LPUNKNOWN pUnk, bool mt = true);
	virtual ~CFGManager();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};

class CFGManagerCustom : public CFGManager
{
	void InitSourceFilters(UINT src);
	void InitTransformFilters(UINT src);

public:
	CFGManagerCustom(LPCTSTR name, LPUNKNOWN pUnk, UINT src = ~0, UINT tra = ~0, bool mt = true);

	// IFilterGraph

	STDMETHODIMP AddFilter(IBaseFilter* pFilter, LPCWSTR pName);
};

class CFGManagerPlayer : public CFGManagerCustom
{
protected:
	UINT64 m_vrmerit, m_armerit;

	// IFilterGraph

	STDMETHODIMP ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt);

public:
	CFGManagerPlayer(LPCTSTR name, LPUNKNOWN pUnk, UINT src, UINT tra, bool mt = true);
};

class CFGManagerDVD : public CFGManagerPlayer
{
protected:

	// IGraphBuilder

	STDMETHODIMP RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList);
	STDMETHODIMP AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);

public:
	CFGManagerDVD(LPCTSTR name, LPUNKNOWN pUnk, UINT src, UINT tra, bool mt = true);
};

//

class CFGAggregator : public CUnknown
{
protected:
	CComPtr<IUnknown> m_inner;

public:
	CFGAggregator(const CLSID& clsid, LPCTSTR name, LPUNKNOWN pUnk, HRESULT& hr);
	virtual ~CFGAggregator();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};
