#pragma once

#include <dshow.h>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <string>
#include <set>
#include "../3rdparty/baseclasses/baseclasses.h"

#define S_CONTINUE 0x7fffffff

#define EC_USER_MAGIC 0x12345678
#define EC_USER_PAUSE 1
#define EC_USER_RESUME 2

struct PIN_COUNT
{
	int in, out;
	struct {int in, out;} disconnected;
	struct {int in, out;} connected;
};

namespace DirectShow
{
	extern IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
	extern IPin* GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
	extern int GetPinCount(IBaseFilter* pBF, PIN_COUNT& count);
	extern IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin = NULL);
	extern IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin = NULL);
	extern IPin* GetConnected(IPin* pPin);
	extern PIN_DIRECTION GetDirection(IPin* pPin);

	extern IFilterGraph* GetGraph(IBaseFilter* pBF);
	extern IBaseFilter* GetFilter(IPin* pPin);
	extern CLSID GetCLSID(IBaseFilter* pBF);
	extern CLSID GetCLSID(IPin* pPin);
	extern std::wstring GetName(IBaseFilter* pBF);
	extern std::wstring GetName(IPin* pPin);

	extern bool IsSplitter(IBaseFilter* pBF, bool connected = false);
	extern bool IsMultiplexer(IBaseFilter* pBF, bool connected = false);
	extern bool IsVideoRenderer(IBaseFilter* pBF);
	extern bool IsAudioWaveRenderer(IBaseFilter* pBF);
	extern bool IsStreamStart(IBaseFilter* pBF);
	extern bool IsStreamEnd(IBaseFilter* pBF);

	extern int AACInitData(BYTE* data, int profile, int freq, int channels);
	extern bool Mpeg2InitData(CMediaType& mt, BYTE* seqhdr, int len, int w, int h);

	extern bool GetTempFileName(std::wstring& path, LPCWSTR subdir = NULL);

	extern std::wstring StripDVBName(const char* str, int len);

	extern bool Match(HANDLE file, const std::wstring& pattern);

	extern UINT64 GetFileVersion(LPCWSTR fn);

	extern HRESULT LoadExternalObject(LPCWSTR path, REFCLSID clsid, REFIID iid, void** ppv);

	extern void ShowPropertyPage(IUnknown* pUnk, HWND hParentWnd);

	typedef enum {Drive_NotFound, Drive_Audio, Drive_VideoCD, Drive_DVDVideo, Drive_Unknown};

	extern int GetDriveType(wchar_t drive);
	extern int GetDriveType(wchar_t drive, std::list<std::wstring>& files);

	extern std::wstring GetDriveLabel(wchar_t drive);

	extern DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps = 0);
	extern REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps = 0);

	template<class T> CUnknown* WINAPI CreateInstance(IUnknown* lpunk, HRESULT* phr)
	{
		*phr = S_OK;
	    CUnknown* punk = new T(lpunk, phr);
	    if(punk == NULL) *phr = E_OUTOFMEMORY;
		return punk;
	}
	
	bool DeleteRegKey(LPCWSTR pszKey, LPCWSTR pszSubkey);
	bool SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValueName, LPCWSTR pszValue);
	bool SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValue);
	void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype, LPCWSTR chkbytes, LPCWSTR ext, ...);
	void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype, const std::list<std::wstring>& chkbytes, LPCTSTR ext, ...);
	void UnRegisterSourceFilter(const GUID& subtype);
};

class CPinInfo : public PIN_INFO
{
public:
	CPinInfo() {pFilter = NULL;}
	~CPinInfo() {if(pFilter) pFilter->Release();}
};

class CFilterInfo : public FILTER_INFO
{
public:
	CFilterInfo() {pGraph = NULL;}
	~CFilterInfo() {if(pGraph) pGraph->Release();}
};

#define countof(a) (sizeof(a) / sizeof(a[0]))

template<class T> HRESULT Foreach(IFilterGraph* pFG, T f)
{
	HRESULT hr = S_CONTINUE;

	CComPtr<IEnumFilters> pEF;

	if(pFG != NULL && SUCCEEDED(pFG->EnumFilters(&pEF)))
	{
		for(CComPtr<IBaseFilter> pBF; hr == S_CONTINUE && pEF->Next(1, &pBF, 0) == S_OK; pBF = NULL)
		{
			hr = f(pBF);
		}
	}

	return hr;
}

template<class I, class T> HRESULT ForeachInterface(IFilterGraph* pFG, T f)
{
	HRESULT hr = S_CONTINUE;

	CComPtr<IEnumFilters> pEF;

	if(pFG != NULL && SUCCEEDED(pFG->EnumFilters(&pEF)))
	{
		for(CComPtr<IBaseFilter> pBF; hr == S_CONTINUE && pEF->Next(1, &pBF, 0) == S_OK; pBF = NULL)
		{
			if(CComQIPtr<I> p = pBF)
			{
				hr = f(pBF, p);
			}
		}
	}

	return hr;
}

template<class T> HRESULT Foreach(IGraphConfig* pGC, T f)
{
	HRESULT hr = S_CONTINUE;

	CComPtr<IEnumFilters> pEF;

	if(pGC != NULL && SUCCEEDED(pGC->EnumCacheFilter(&pEF)))
	{
		for(CComPtr<IBaseFilter> pBF; hr == S_CONTINUE && pEF->Next(1, &pBF, 0) == S_OK; pBF = NULL)
		{
			hr = f(pBF);
		}
	}

	return hr;
}

template<class T> HRESULT Foreach(IBaseFilter* pBF, PIN_DIRECTION dir, T f)
{
	HRESULT hr = S_CONTINUE;

	CComPtr<IEnumPins> pEP;

	if(pBF != NULL && SUCCEEDED(pBF->EnumPins(&pEP)))
	{
		for(CComPtr<IPin> pPin; hr == S_CONTINUE && pEP->Next(1, &pPin, 0) == S_OK; pPin = NULL)
		{
			if(dir == -1 || DirectShow::GetDirection(pPin) == dir)
			{
				hr = f(pPin);
			}
		}
	}

	return hr;
}

template<class T> HRESULT Foreach(IBaseFilter* pBF, T f)
{
	return Foreach(pBF, (PIN_DIRECTION)-1, f);
}

template<class T> HRESULT Foreach(IPin* pPin, T f)
{
	HRESULT hr = S_CONTINUE;

	CComPtr<IEnumMediaTypes> pEMT;

	if(pPin != NULL && SUCCEEDED(pPin->EnumMediaTypes(&pEMT)))
	{
		AM_MEDIA_TYPE* pmt = NULL;

		while(hr == S_CONTINUE && pEMT->Next(1, &pmt, NULL) == S_OK)
		{
			hr = f(pmt);

			DeleteMediaType(pmt);

			pmt = NULL;
		}
	}

	return hr;
}

template<class T> HRESULT ForeachUnique(IPin* pPin, T f)
{
	HRESULT hr = S_CONTINUE;

	std::list<CMediaType> mts;

	CComPtr<IEnumMediaTypes> pEMT;

	if(pPin != NULL && SUCCEEDED(pPin->EnumMediaTypes(&pEMT)))
	{
		AM_MEDIA_TYPE* pmt = NULL;

		while(hr == S_CONTINUE && pEMT->Next(1, &pmt, NULL) == S_OK)
		{
			auto i = std::find_if(mts.begin(), mts.end(), [&pmt] (const CMediaType& mt)
			{
				return mt.majortype == pmt->majortype && mt.subtype == pmt->subtype;
			});
			
			if(i == mts.end())
			{
				mts.push_back(CMediaType(*pmt));

				hr = f(pmt);
			}

			DeleteMediaType(pmt);

			pmt = NULL;
		}
	}

	return hr;
}

template<class T> HRESULT Foreach(const CLSID& category, T f)
{
	HRESULT hr = S_CONTINUE;

	CComPtr<ICreateDevEnum> pDE;

	pDE.CoCreateInstance(CLSID_SystemDeviceEnum);

	CComPtr<IEnumMoniker> pCE;

	if(pDE != NULL && SUCCEEDED(pDE->CreateClassEnumerator(category, &pCE, 0)) && pCE != NULL)
	{
		for(CComPtr<IMoniker> moniker; hr == S_CONTINUE && pCE->Next(1, &moniker, 0) == S_OK; moniker = NULL)
		{
			std::wstring id, name;

			LPOLESTR dispname = NULL;

			hr = moniker->GetDisplayName(0, 0, &dispname);

			if(FAILED(hr))
			{
				continue;
			}

			id = dispname;

			CoTaskMemFree(dispname);

			CComPtr<IPropertyBag> pb;

			hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pb);

			if(FAILED(hr))
			{
				continue;
			}

			CComVariant var;

			hr = pb->Read(CComBSTR(L"FriendlyName"), &var, NULL);

			if(SUCCEEDED(hr))
			{
				name = var.bstrVal;
			}

			hr = f(moniker, id.c_str(), name.c_str());
		}
	}

	return hr;
}

