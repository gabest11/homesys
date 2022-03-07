#include "stdafx.h"
#include "FGManager.h"
#include "DirectShow.h"
#include "Filters.h"
#include "MediaTypeEx.h"
//#include "wst_font.inc"
//#include "../3rdparty/zlib/zlib.h"
#include <initguid.h>
#include "moreuuids.h"
#include "Monogram_x264.h"
#include "Monogram_aac.h"

// CFGManager

CFGManager::CFGManager(LPCTSTR name, LPUNKNOWN pUnk, bool mt)
	: CUnknown(name, pUnk)
	, m_cookie(0)
{
	m_inner.CoCreateInstance(mt ? CLSID_FilterGraph : CLSID_FilterGraphNoThread, GetOwner());
	m_fm.CoCreateInstance(CLSID_FilterMapper2);
	/*
	for(int i = 0; i < sizeof(wst_fonts) / sizeof(wst_fonts[0]); i++)
	{
		uLongf dlen = (wst_fonts[i][0] << 8) | wst_fonts[i][1];
		uLongf slen = (wst_fonts[i][2] << 8) | wst_fonts[i][3];

		BYTE* buff = new BYTE[dlen];

		if(uncompress(buff, &dlen, &wst_fonts[i][4], slen) == Z_OK)
		{
			m_fontinstaller.Install(buff, dlen);
		}

		delete [] buff;
	}
	*/
}

CFGManager::~CFGManager()
{
	CComPtr<IUnknown> inner = m_inner;

	{
		CAutoLock cAutoLock(this);

		for(auto i = m_source.begin(); i != m_source.end(); i++) delete *i;
		for(auto i = m_transform.begin(); i != m_transform.end(); i++) delete *i;
		for(auto i = m_override.begin(); i != m_override.end(); i++) delete *i;

		m_unks.clear();
		m_inner.Release();
	}
}

STDMETHODIMP CFGManager::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return
		QI(IFilterGraph)
		QI(IGraphBuilder)
		QI(IFilterGraph2)
		QI(IGraphBuilder2)
		m_inner && (riid != IID_IUnknown && SUCCEEDED(m_inner->QueryInterface(riid, ppv))) ? S_OK :
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CFGManager::EnumSourceFilters(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrMime, CFGFilterList& filters)
{
	CheckPointer(lpcwstrFileName, E_POINTER);

	filters.RemoveAll();

	std::wstring protocol;
	std::wstring filename;
	std::wstring extension;

	SplitFilename(lpcwstrFileName, protocol, filename, extension);

	HANDLE file = INVALID_HANDLE_VALUE;

	if(protocol.empty() || protocol == L"file")
	{
		file = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, (HANDLE)NULL);

		if(file == INVALID_HANDLE_VALUE)
		{
			return VFW_E_NOT_FOUND;
		}
	}

	// exceptions first

	if(extension == L".dvr-ms" || extension == L".wtv") // doh, this is stupid 
	{
		filters.Insert(new CFGFilterRegistry(CLSID_StreamBufferSource, MERIT64_PREFERRED), 0);
	}

	// internal

	if(lpcwstrMime != NULL)
	{
		// mime

		for(auto i = m_source.begin(); i != m_source.end(); i++)
		{
			CFGFilter* f = *i;

			if(std::find(f->m_mimes.begin(), f->m_mimes.end(), lpcwstrMime) != f->m_mimes.end())
			{
				filters.Insert(f, -1, false, false);
			}
		}
	}

	if(file == INVALID_HANDLE_VALUE)
	{
		// protocol

		for(auto i = m_source.begin(); i != m_source.end(); i++)
		{
			CFGFilter* f = *i;

			if(std::find(f->m_protocols.begin(), f->m_protocols.end(), protocol) != f->m_protocols.end())
			{
				if(!f->m_mimes.empty())
				{
					if(lpcwstrMime != NULL && std::find(f->m_mimes.begin(), f->m_mimes.end(), lpcwstrMime) == f->m_mimes.end())
					{
						continue;
					}
				}

				filters.Insert(f, 0, false, false);
			}
		}
	}
	else
	{
		// checkbytes

		for(auto i = m_source.begin(); i != m_source.end(); i++)
		{
			CFGFilter* f = *i;

			for(auto j = f->m_chkbytes.begin(); j != f->m_chkbytes.end(); j++)
			{
				const std::wstring& pattern = *j;

				if(DirectShow::Match(file, pattern))
				{
					filters.Insert(f, 1, false, false);

					break;
				}
			}
		}
	}

	if(!extension.empty())
	{
		// extension

		for(auto i = m_source.begin(); i != m_source.end(); i++)
		{
			CFGFilter* f = *i;

			if(std::find(f->m_extensions.begin(), f->m_extensions.end(), extension) != f->m_extensions.end())
			{
				filters.Insert(f, 2, false, false);
			}
		}
	}

	{
		// the rest

		for(auto i = m_source.begin(); i != m_source.end(); i++)
		{
			CFGFilter* f = *i;

			if(f->m_protocols.empty() && f->m_chkbytes.empty() && f->m_extensions.empty())
			{
				filters.Insert(f, 3, false, false);
			}
		}
	}

	// registry

	WCHAR buff[256], buff2[256];
	ULONG len, len2;

	if(file == INVALID_HANDLE_VALUE)
	{
		// protocol
	
		CRegKey key;

		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, protocol.c_str(), KEY_READ))
		{
			CRegKey exts;

			if(ERROR_SUCCESS == exts.Open(key, L"Extensions", KEY_READ))
			{
				len = countof(buff);

				if(ERROR_SUCCESS == exts.QueryStringValue(extension.c_str(), buff, &len))
				{
					CLSID clsid;

					if(Util::ToCLSID(buff, clsid))
					{
						filters.Insert(new CFGFilterRegistry(clsid), 4);
					}
				}
			}

			{
				len = countof(buff);

				if(ERROR_SUCCESS == key.QueryStringValue(L"Source Filter", buff, &len))
				{
					CLSID clsid;

					if(Util::ToCLSID(buff, clsid))
					{
						filters.Insert(new CFGFilterRegistry(clsid), 5);
					}
				}
			}
		}

		filters.Insert(new CFGFilterRegistry(CLSID_URLReader), 6);
	}
	else
	{
		// checkbytes

		CRegKey key;

		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"Media Type", KEY_READ))
		{
			FILETIME ft;

			len = countof(buff);

			for(DWORD i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len, &ft); i++, len = countof(buff))
			{
				CLSID majortype;

				if(!Util::ToCLSID(buff, majortype))
				{
					continue;
				}

				CRegKey majorkey;

				if(ERROR_SUCCESS == majorkey.Open(key, buff, KEY_READ))
				{
					len = countof(buff);

					for(DWORD j = 0; ERROR_SUCCESS == majorkey.EnumKey(j, buff, &len, &ft); j++, len = countof(buff))
					{
						GUID subtype;

						if(!Util::ToCLSID(buff, subtype))
						{
							continue;
						}

						CRegKey subkey;

						if(ERROR_SUCCESS == subkey.Open(majorkey, buff, KEY_READ))
						{
							len = countof(buff);

							if(ERROR_SUCCESS != subkey.QueryStringValue(L"Source Filter", buff, &len))
							{
								continue;
							}

							CLSID clsid;

							if(!Util::ToCLSID(buff, clsid) || clsid == GUID_NULL)
							{
								continue;
							}

							len = countof(buff);
							len2 = sizeof(buff2);

							DWORD type;

							for(DWORD i = 0; ERROR_SUCCESS == RegEnumValue(subkey, i, buff2, &len2, 0, &type, (BYTE*)buff, &len); i++)
							{
								if(DirectShow::Match(file, buff))
								{
									CFGFilter* f = new CFGFilterRegistry(clsid);
									
									f->AddType(majortype, subtype);
									
									filters.Insert(f, 7);

									break;
								}

								len = countof(buff);
								len2 = sizeof(buff2);
							}
						}
					}
				}
			}
		}
	}

	if(!extension.empty())
	{
		// extension

		CRegKey key;

		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, (L"Media Type\\Extensions\\" + extension).c_str(), KEY_READ))
		{
			len = countof(buff);

			LONG ret = key.QueryStringValue(_T("Source Filter"), buff, &len); // QueryStringValue can return ERROR_INVALID_DATA on bogus strings (radlight mpc v1003, fixed in v1004)

			CLSID clsid;

			Util::ToCLSID(buff, clsid);

			if(ERROR_SUCCESS == ret || ERROR_INVALID_DATA == ret && clsid != GUID_NULL)
			{
				CLSID majortype, subtype;

				len = countof(buff);

				if(ERROR_SUCCESS == key.QueryStringValue(L"Media Type", buff, &len))
				{
					Util::ToCLSID(buff, majortype);
				}

				len = countof(buff);

				if(ERROR_SUCCESS == key.QueryStringValue(_T("Subtype"), buff, &len))
				{
					Util::ToCLSID(buff, subtype);
				}

				CFGFilter* f = new CFGFilterRegistry(clsid);

				f->AddType(majortype, subtype);

				filters.Insert(f, 8);
			}
		}
	}

	if(file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
	}

	CFGFilter* f = new CFGFilterRegistry(CLSID_AsyncReader);

	f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_NULL);

	filters.Insert(f, 9);

	return S_OK;
}

HRESULT CFGManager::AddSourceFilter(CFGFilter* f, LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppBF)
{
	CheckPointer(lpcwstrFileName, E_POINTER);
	CheckPointer(ppBF, E_POINTER);

	ASSERT(*ppBF == NULL);

	HRESULT hr;

	CComPtr<IBaseFilter> pBF;
	std::list<CAdapt<CComPtr<IUnknown>>> unks;

	hr = f->Create(&pBF, unks);

	if(FAILED(hr))
	{
		return hr;
	}

	CComQIPtr<IFileSourceFilter> pFSF = pBF;

	if(pFSF == NULL)
	{
		return E_NOINTERFACE;
	}

	hr = AddFilter(pBF, lpcwstrFilterName);

	if(FAILED(hr))
	{
		return hr;
	}

	const AM_MEDIA_TYPE* pmt = NULL;

	CMediaType mt;

	const std::list<CLSID>& types = f->GetTypes();

	if(types.size() == 2 && (types.front() != GUID_NULL || types.back() != GUID_NULL))
	{
		mt.majortype = types.front();
		mt.subtype = types.back();

		pmt = &mt;
	}

	hr = pFSF->Load(lpcwstrFileName, pmt);

	if(FAILED(hr))
	{
		RemoveFilter(pBF);

		return hr;
	}

	// doh

	{
		std::vector<CLSID> netshowtypes;

		netshowtypes.resize(3);

		Util::ToCLSID(L"{640999A0-A946-11D0-A520-000000000000}", netshowtypes[0]);
		Util::ToCLSID(L"{640999A1-A946-11D0-A520-000000000000}", netshowtypes[1]);
		Util::ToCLSID(L"{D51BD5AE-7548-11CF-A520-0080C77EF58A}", netshowtypes[2]);

		Foreach(DirectShow::GetFirstPin(pBF, PINDIR_OUTPUT), [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
		{
			if(std::find(netshowtypes.begin(), netshowtypes.end(), pmt->subtype) != netshowtypes.end())
			{
				RemoveFilter(pBF);

				pBF = NULL;

				f = new CFGFilterRegistry(CLSID_NetShowSource);

				hr = AddSourceFilter(f, lpcwstrFileName, lpcwstrFilterName, &pBF);

				delete f;

				return S_OK;
			}

			return S_CONTINUE;
		});
	}

	*ppBF = pBF.Detach();

	m_unks = unks;

	return S_OK;
}

void CFGManager::SplitFilename(LPCWSTR str, std::wstring& protocol, std::wstring& filename, std::wstring& extension)
{
	filename = Util::TrimLeft(str);

	std::wstring::size_type i = filename.find_first_of(':');

	if(i != std::wstring::npos && i > 1)
	{
		protocol = Util::MakeLower(filename.substr(0, i));
	}
	else
	{
		protocol.clear();
	}

	extension = Util::MakeLower(PathFindExtension(filename.c_str()));
}

HRESULT CFGManager::Connect(IPin* pPinOut, IPin* pPinIn, bool recursive)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPinOut, E_POINTER);

	HRESULT hr;

	if(DirectShow::GetDirection(pPinOut) != PINDIR_OUTPUT)
	{
		return VFW_E_INVALID_DIRECTION;
	}

	if(pPinIn != NULL && DirectShow::GetDirection(pPinIn) != PINDIR_INPUT)
	{
		return VFW_E_INVALID_DIRECTION;
	}

	if(DirectShow::GetConnected(pPinOut) != NULL)
	{
		return VFW_E_ALREADY_CONNECTED;
	}

	if(pPinIn != NULL && DirectShow::GetConnected(pPinIn) != NULL)
	{
		return VFW_E_ALREADY_CONNECTED;
	}

	if(pPinIn)
	{
		// 1. Try a direct connection between the filters, with no intermediate filters

		hr = ConnectDirect(pPinOut, pPinIn, NULL);

		if(SUCCEEDED(hr))
		{
			return hr;
		}
	}
	else
	{
		// 1. Use IStreamBuilder

		if(CComQIPtr<IStreamBuilder> pSB = pPinOut)
		{
			hr = pSB->Render(pPinOut, this);

			if(SUCCEEDED(hr))
			{
				return hr;
			}

			pSB->Backout(pPinOut, this);
		}

		if(CComQIPtr<IStreamBuilder> pSB = DirectShow::GetFilter(pPinOut))
		{
			hr = pSB->Render(pPinOut, this);

			if(SUCCEEDED(hr))
			{
				return hr;
			}

			pSB->Backout(pPinOut, this);
		}
	}

	// 2. Try cached filters

	if(CComQIPtr<IGraphConfig> pGC = (IGraphBuilder2*)this)
	{
		hr = Foreach(pGC, [&] (IBaseFilter* pBF) -> HRESULT
		{
			HRESULT hr;

			if(pPinIn == NULL || DirectShow::GetFilter(pPinIn) != pBF)
			{
				hr = pGC->RemoveFilterFromCache(pBF);

				// does RemoveFilterFromCache call AddFilter like AddFilterToCache calls RemoveFilter ?

				hr = ConnectFilterDirect(pPinOut, pBF, NULL);

				if(SUCCEEDED(hr))
				{
					hr = ConnectFilter(pBF, pPinIn);

					if(SUCCEEDED(hr))
					{
						return S_OK;
					}
				}

				hr = pGC->AddFilterToCache(pBF);
			}

			return S_CONTINUE;
		});

		if(hr == S_OK)
		{
			return S_OK;
		}
	}

	// 3. Try filters in the graph

	{
		CLSID clsid;

		Util::ToCLSID(L"{04FE9017-F873-410E-871E-AB91661A4EF7}", clsid); // ffdshow

		std::list<IBaseFilter*> filters;

		hr = Foreach(this, [&] (IBaseFilter* pBF) -> HRESULT
		{
			if(pPinIn != NULL && DirectShow::GetFilter(pPinIn) == pBF 
			|| DirectShow::GetFilter(pPinOut) == pBF)
			{
				return S_CONTINUE;
			}

			if(DirectShow::GetCLSID(pPinOut) == clsid 
			&& DirectShow::GetCLSID(pBF) == CLSID_AudioRecord)
			{
				return S_CONTINUE;
			}

			filters.push_back(pBF);

			return S_CONTINUE;
		});

		for(auto i = filters.begin(); i != filters.end(); i++)
		{
			IBaseFilter* pBF = *i;

			hr = ConnectFilterDirect(pPinOut, pBF, NULL);

			if(SUCCEEDED(hr))
			{
				hr = ConnectFilter(pBF, pPinIn);

				if(SUCCEEDED(hr))
				{
					return S_OK;
				}
			}

			EXECUTE_ASSERT(Disconnect(pPinOut));
		};
	}

	// 4. Look up filters in the registry
	
	{
		CFGFilterList fl;

		std::list<CMediaType> mts;

		ForeachUnique(pPinOut, [&mts] (AM_MEDIA_TYPE* pmt) -> HRESULT 
		{
			mts.push_back(*pmt); 
			
			return S_CONTINUE;
		});

		for(auto i = m_transform.begin(); i != m_transform.end(); i++)
		{
			CFGFilter* f = *i;

			if(f->GetMerit() < MERIT64_DO_USE || f->Match(mts, false))
			{
				fl.Insert(f, 0, f->Match(mts, true), false);
			}
		}

		for(auto i = m_override.begin(); i != m_override.end(); i++)
		{
			CFGFilter* f = *i;

			if(f->GetMerit() < MERIT64_DO_USE || f->Match(mts, false))
			{
				fl.Insert(f, 0, f->Match(mts, true), false);
			}
		}

		if(!mts.empty())
		{
			std::vector<CLSID> types;

			types.reserve(mts.size() * 2);

			for(auto i = mts.begin(); i != mts.end(); i++)
			{
				types.push_back(i->majortype);
				types.push_back(i->subtype);
			}

			CComPtr<IEnumMoniker> pEM;

			hr = m_fm->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE + 1, TRUE, types.size() / 2, &types[0], NULL, NULL, FALSE, !!pPinIn, 0, NULL, NULL, NULL);

			if(SUCCEEDED(hr))
			{
				for(CComPtr<IMoniker> moniker; S_OK == pEM->Next(1, &moniker, NULL); moniker = NULL)
				{
					CFGFilterRegistry* f = new CFGFilterRegistry(moniker);

					fl.Insert(f, 0, f->Match(mts, true));
				}
			}
		}

		std::list<CFGFilter*> filters;

		fl.GetFilters(filters);

		for(auto i = filters.begin(); i != filters.end(); i++)
		{
			CFGFilter* f = *i;

			CComPtr<IBaseFilter> pBF;

			std::list<CAdapt<CComPtr<IUnknown>>> unks;

			hr = f->Create(&pBF, unks);

			if(FAILED(hr))
			{
				continue;
			}

			std::wstring name = f->GetName();

			hr = AddFilter(pBF, name.c_str());

			wprintf(L"%s (%08x)\n", name.c_str(), hr);

			if(FAILED(hr))
			{
				continue;
			}

			hr = ConnectFilterDirect(pPinOut, pBF, NULL);
/*
			if(FAILED(hr) && !mts.empty())
			{
				const CMediaType& mt = mts.front();

				if(mt.majortype == MEDIATYPE_Stream && mt.subtype != GUID_NULL)
				{
					CMediaType mt2 = mt;
					
					mt2.formattype = FORMAT_None;

					hr = ConnectFilterDirect(pPinOut, pBF, &mt2);

					if(FAILED(hr)) 
					{
						mt2.formattype = GUID_NULL;

						hr = ConnectFilterDirect(pPinOut, pBF, &mt2);
					}
				}
			}
*/
			if(SUCCEEDED(hr))
			{
				if(recursive)
				{
					hr = ConnectFilter(pBF, pPinIn);
				}

				if(SUCCEEDED(hr))
				{
					m_unks.insert(m_unks.end(), unks.begin(), unks.end());

					// maybe the application should do these...

					for(auto i = unks.begin(); i != unks.end(); i++)
					{
						CComPtr<IUnknown> unk = *i;

						if(CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> pMPC = unk)
						{
							pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED);
						}
					}

					if(CComQIPtr<IVMRAspectRatioControl> pARC = pBF)
					{
						pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
					}
					
					if(CComQIPtr<IVMRAspectRatioControl9> pARC = pBF)
					{
						pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
					}

					return hr;
				}
			}

			EXECUTE_ASSERT(SUCCEEDED(RemoveFilter(pBF)));
		}
	}

	return pPinIn ? VFW_E_CANNOT_CONNECT : VFW_E_CANNOT_RENDER;
}

// IFilterGraph

STDMETHODIMP CFGManager::AddFilter(IBaseFilter* pFilter, LPCWSTR pName)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	HRESULT hr;

	hr = CComQIPtr<IFilterGraph2>(m_inner)->AddFilter(pFilter, pName);

	if(FAILED(hr))
	{
		return hr;
	}

	// TODO

	hr = pFilter->JoinFilterGraph(NULL, NULL);
	hr = pFilter->JoinFilterGraph(this, pName);

	return hr;
}

STDMETHODIMP CFGManager::RemoveFilter(IBaseFilter* pFilter)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->RemoveFilter(pFilter);
}

STDMETHODIMP CFGManager::EnumFilters(IEnumFilters** ppEnum)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->EnumFilters(ppEnum);
}

STDMETHODIMP CFGManager::FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->FindFilterByName(pName, ppFilter);
}

STDMETHODIMP CFGManager::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	CComPtr<IBaseFilter> pBF = DirectShow::GetFilter(pPinIn);

	CLSID clsid = DirectShow::GetCLSID(pBF);

	// TODO: GetUpStreamFilter goes up on the first input pin only

	CComPtr<IBaseFilter> pBFUS = DirectShow::GetFilter(pPinOut);

	while(pBFUS != NULL)
	{
		if(pBFUS == pBF)
		{
			return VFW_E_CIRCULAR_GRAPH;
		}

        if(clsid == DirectShow::GetCLSID(pBFUS) && clsid != CLSID_Proxy)
		{
			return VFW_E_CANNOT_CONNECT;
		}

		pBFUS = DirectShow::GetUpStreamFilter(pBFUS);
	}

	return CComQIPtr<IFilterGraph2>(m_inner)->ConnectDirect(pPinOut, pPinIn, pmt);
}

STDMETHODIMP CFGManager::Reconnect(IPin* ppin)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->Reconnect(ppin);
}

STDMETHODIMP CFGManager::Disconnect(IPin* ppin)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->Disconnect(ppin);
}

STDMETHODIMP CFGManager::SetDefaultSyncSource()
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->SetDefaultSyncSource();
}

// IGraphBuilder

STDMETHODIMP CFGManager::Connect(IPin* pPinOut, IPin* pPinIn)
{
	return Connect(pPinOut, pPinIn, true);
}

STDMETHODIMP CFGManager::Render(IPin* pPinOut)
{
	CAutoLock cAutoLock(this);

	return RenderEx(pPinOut, 0, NULL);
}

STDMETHODIMP CFGManager::RenderFile(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrPlayList)
{
	return RenderFileMime(lpcwstrFileName, NULL);
}

STDMETHODIMP CFGManager::AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
	return AddSourceFilterMime(lpcwstrFileName, lpcwstrFilterName, NULL, ppFilter);
}

STDMETHODIMP CFGManager::SetLogFile(DWORD_PTR hFile)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->SetLogFile(hFile);
}

STDMETHODIMP CFGManager::Abort()
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->Abort();
}

STDMETHODIMP CFGManager::ShouldOperationContinue()
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->ShouldOperationContinue();
}

// IFilterGraph2

STDMETHODIMP CFGManager::AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter);
}

STDMETHODIMP CFGManager::ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt)
{
	if(!m_inner) return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_inner)->ReconnectEx(ppin, pmt);
}

STDMETHODIMP CFGManager::RenderEx(IPin* pPinOut, DWORD dwFlags, DWORD* pvContext)
{
	CAutoLock cAutoLock(this);

	if(pPinOut == NULL || dwFlags > AM_RENDEREX_RENDERTOEXISTINGRENDERERS || pvContext != NULL)
	{
		return E_INVALIDARG;
	}

	HRESULT hr;

	if(dwFlags & AM_RENDEREX_RENDERTOEXISTINGRENDERERS)
	{
		hr = Foreach(this, [&] (IBaseFilter* pBF) -> HRESULT
		{
			bool renderer = false;

			CComQIPtr<IAMFilterMiscFlags> pAMMF = pBF;

			if(pAMMF != NULL)
			{
				if(pAMMF->GetMiscFlags() & AM_FILTER_MISC_FLAGS_IS_RENDERER)
				{
					renderer = true;
				}
			}
			else
			{
				Foreach(pBF, [&renderer] (IPin* pin) -> HRESULT
				{
					HRESULT hr;

					CComPtr<IPin> pPinIn;
					
					DWORD n = 1;

					hr = pin->QueryInternalConnections(&pPinIn, &n);

					if(SUCCEEDED(hr) && n == 0)
					{
						renderer = true;

						return S_OK;
					}

					return S_CONTINUE;
				});
			}

			if(renderer)
			{
				HRESULT hr;

				hr = ConnectFilter(pPinOut, pBF);

				if(SUCCEEDED(hr))
				{
					return S_OK;
				}
			}

			return S_FALSE;
		});

		return hr == S_OK ? S_OK : VFW_E_CANNOT_RENDER;
	}

	return Connect(pPinOut, NULL);
}

// IGraphBuilder2

STDMETHODIMP CFGManager::RenderFileMime(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrMimeType)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

/*
	CComPtr<IBaseFilter> pBF;

	hr = AddSourceFilter(lpcwstrFile, lpcwstrFile, &pBF);

	if(FAILED(hr))
	{
		return hr;
	}

	return ConnectFilter(pBF, NULL);
*/

	CFGFilterList fl;

	hr = EnumSourceFilters(lpcwstrFileName, lpcwstrMimeType, fl);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = VFW_E_CANNOT_RENDER;

	std::list<CFGFilter*> filters;

	fl.GetFilters(filters);

	for(auto i = filters.begin(); i != filters.end(); i++)
	{
		CComPtr<IBaseFilter> pBF;

		hr = AddSourceFilter(*i, lpcwstrFileName, lpcwstrFileName, &pBF);
		
		if(SUCCEEDED(hr))
		{
			hr = ConnectFilter(pBF, NULL);

			if(SUCCEEDED(hr))
			{
				return hr;
			}

			NukeDownstream(pBF);

			RemoveFilter(pBF);
		}
	}

	return hr;	
}

STDMETHODIMP CFGManager::AddSourceFilterMime(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, LPCWSTR lpcwstrMimeType, IBaseFilter** ppFilter)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	CFGFilterList fl;

	hr = EnumSourceFilters(lpcwstrFileName, lpcwstrMimeType, fl);

	if(FAILED(hr))
	{
		return hr;
	}

	std::list<CFGFilter*> filters;

	fl.GetFilters(filters);

	for(auto i = filters.begin(); i != filters.end(); i++)
	{
		hr = AddSourceFilter(*i, lpcwstrFileName, lpcwstrFilterName, ppFilter);

		if(SUCCEEDED(hr))
		{
			return hr;
		}
	}

	return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
}

STDMETHODIMP CFGManager::HasRegisteredSourceFilter(LPCWSTR fn)
{
	CFGFilterList fl;

	return SUCCEEDED(EnumSourceFilters(fn, NULL, fl)) && fl.GetCount() > 1 ? S_OK : S_FALSE;
}

STDMETHODIMP CFGManager::AddFilterForDisplayName(LPCWSTR dispname, IBaseFilter** ppBF)
{
	CheckPointer(ppBF, E_POINTER);

	*ppBF = NULL;

	HRESULT hr;

	CComPtr<IBindCtx> ctx;

	CreateBindCtx(0, &ctx);

	CComPtr<IMoniker> moniker;

	ULONG eaten;

	hr = MkParseDisplayName(ctx, dispname, &eaten, &moniker);

	if(S_OK != hr)
	{
		return E_FAIL;
	}

	hr = moniker->BindToObject(ctx, 0, IID_IBaseFilter, (void**)ppBF);

	if(FAILED(hr) || *ppBF == NULL)
	{
		return E_FAIL;
	}

	std::wstring name = dispname;

	CComPtr<IPropertyBag> pPB;
	
	hr = moniker->BindToStorage(ctx, 0, IID_IPropertyBag, (void**)&pPB);

	if(SUCCEEDED(hr))
	{
		CComVariant var;

		hr = pPB->Read(CComBSTR(L"FriendlyName"), &var, NULL);

		if(SUCCEEDED(hr))
		{
			name = var.bstrVal;
		}
	}

	return AddFilter(*ppBF, name.c_str());
}

STDMETHODIMP CFGManager::NukeDownstream(IUnknown* pUnk)
{
	CAutoLock cAutoLock(this);

	if(CComQIPtr<IBaseFilter> pBF = pUnk)
	{
		Foreach(pBF, PINDIR_OUTPUT, [this] (IPin* pin) -> HRESULT
		{
			NukeDownstream(pin);

			return S_CONTINUE;
		});
	}
	else if(CComQIPtr<IPin> pPin = pUnk)
	{
		CComPtr<IPin> pPinTo;

		if(DirectShow::GetDirection(pPin) == PINDIR_OUTPUT && SUCCEEDED(pPin->ConnectedTo(&pPinTo)))
		{
			if(CComPtr<IBaseFilter> pBF = DirectShow::GetFilter(pPinTo))
			{
				NukeDownstream(pBF);

				Disconnect(pPinTo);
				Disconnect(pPin);

				pPinTo = NULL; // release the pin before the filter gets destroyed, so we don't call IUnknown::Release on the deleted pin!

				RemoveFilter(pBF);
			}
		}
	}
	else
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP CFGManager::ConnectFilter(IPin* pPinOut)
{
	return Connect(pPinOut, NULL, false);
}

STDMETHODIMP CFGManager::ConnectFilter(IBaseFilter* pBF, IPin* pPinIn)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pBF, E_POINTER);

	if(pPinIn && DirectShow::GetDirection(pPinIn) != PINDIR_INPUT)
	{
		return VFW_E_INVALID_DIRECTION;
	}

	int total = 0;
	int rendered = 0;

	Foreach(pBF, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		std::wstring name = DirectShow::GetName(pin);

		if(name.empty() || name[0] != '~')
		{
			if(DirectShow::GetConnected(pin) == NULL)
			{
				HRESULT hr = Connect(pin, pPinIn);

				if(SUCCEEDED(hr))
				{
					rendered++;

					if(pPinIn != NULL)
					{
						total = 1;

						return S_OK;
					}
				}

				total++;
			}
		}

		return S_CONTINUE;
	});

	return 
		rendered == total ? (rendered > 0 ? S_OK : S_FALSE) :
		rendered > 0 ? VFW_S_PARTIAL_RENDER :
		VFW_E_CANNOT_RENDER;
}

STDMETHODIMP CFGManager::ConnectFilter(IPin* pPinOut, IBaseFilter* pBF)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPinOut, E_POINTER);
	CheckPointer(pBF, E_POINTER);

	if(DirectShow::GetDirection(pPinOut) != PINDIR_OUTPUT)
	{
		return VFW_E_INVALID_DIRECTION;
	}

	HRESULT hr;

	hr = Foreach(pBF, PINDIR_INPUT, [&] (IPin* pin) -> HRESULT
	{
		std::wstring name = DirectShow::GetName(pin);

		if(name.empty() || name[0] != '~')
		{
			if(DirectShow::GetConnected(pin) == NULL)
			{
				HRESULT hr = Connect(pPinOut, pin);

				if(SUCCEEDED(hr))
				{
					return hr;
				}
			}
		}

		return S_CONTINUE;
	});

	return SUCCEEDED(hr) && hr != S_CONTINUE ? hr : VFW_E_CANNOT_CONNECT;
}

STDMETHODIMP CFGManager::ConnectFilterDirect(IBaseFilter* pBFOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pBFOut, E_POINTER);
	CheckPointer(pBF, E_POINTER);

	HRESULT hr;

	int total = 0;
	int connected = 0;

	hr = Foreach(pBFOut, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		hr = ConnectFilterDirect(pin, pBF, pmt);

		if(SUCCEEDED(hr))
		{
			connected++;
		}

		total++;

		return S_CONTINUE;
	});

	return total > 0 && connected == 0 ? VFW_E_CANNOT_CONNECT : connected == total ? S_OK : VFW_S_PARTIAL_RENDER;
}

STDMETHODIMP CFGManager::ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPinOut, E_POINTER);
	CheckPointer(pBF, E_POINTER);

	if(DirectShow::GetDirection(pPinOut) != PINDIR_OUTPUT)
	{
		return VFW_E_INVALID_DIRECTION;
	}

	HRESULT hr;

	hr = Foreach(pBF, PINDIR_INPUT, [&] (IPin* pin) -> HRESULT
	{
		std::wstring name = DirectShow::GetName(pin);

		if(name.empty() || name[0] != '~')
		{
			if(DirectShow::GetConnected(pin) == NULL)
			{
				HRESULT hr = ConnectDirect(pPinOut, pin, pmt);

				if(SUCCEEDED(hr))
				{
					return hr;
				}
			}
		}

		return S_CONTINUE;
	});

	return SUCCEEDED(hr) && hr != S_CONTINUE ? hr : VFW_E_CANNOT_CONNECT;
}

STDMETHODIMP CFGManager::FindInterface(REFIID iid, void** ppv, bool remove)
{
	CAutoLock cAutoLock(this);

	CheckPointer(ppv, E_POINTER);

	for(auto i = m_unks.begin(); i != m_unks.end(); i++)
	{
		CComPtr<IUnknown> unk = *i;

		if(SUCCEEDED(unk->QueryInterface(iid, ppv)))
		{
			if(remove) 
			{
				m_unks.erase(i);
			}

			return S_OK;
		}
	}

	return E_NOINTERFACE;
}

STDMETHODIMP CFGManager::AddToROT()
{
	CAutoLock cAutoLock(this);

    HRESULT hr;

	if(m_cookie != 0)
	{
		return S_FALSE;
	}

    CComPtr<IRunningObjectTable> rot;

	hr = GetRunningObjectTable(0, &rot);

	if(FAILED(hr))
	{
		return hr;
	}

	CComPtr<IMoniker> moniker;

	std::wstring name = Util::Format(L"FilterGraph %08p pid %08x", this, GetCurrentProcessId());

	hr = CreateItemMoniker(L"!", name.c_str(), &moniker);

    if(FAILED(hr))
	{
		return hr;
	}

	return rot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, (IGraphBuilder2*)this, moniker, &m_cookie);
}

STDMETHODIMP CFGManager::RemoveFromROT()
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	if(m_cookie == 0)
	{
		return S_FALSE;
	}

	CComPtr<IRunningObjectTable> rot;

	hr = GetRunningObjectTable(0, &rot);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = rot->Revoke(m_cookie);

	if(FAILED(hr))
	{
		return hr;
	}

	m_cookie = 0;

	return S_OK;
}

STDMETHODIMP CFGManager::Reset()
{
	if(!m_inner) return E_UNEXPECTED;

	CComQIPtr<IMediaControl>(m_inner)->Stop();

	std::list<IBaseFilter*> filters;

	Foreach(this, [&filters] (IBaseFilter* pBF) -> HRESULT
	{
		filters.push_back(pBF);

		return S_CONTINUE;
	});

	for(auto i = filters.begin(); i != filters.end(); i++)
	{
		RemoveFilter(*i);
	}

	RemoveFromROT();

	return TRUE;
}

STDMETHODIMP CFGManager::Clean()
{
	std::list<IBaseFilter*> filters;

	Foreach(this, [&filters] (IBaseFilter* pBF) -> HRESULT
	{
		PIN_COUNT count;

		DirectShow::GetPinCount(pBF, count);

		if(count.connected.in == 0 && count.connected.out == 0)
		{
			filters.push_back(pBF);
		}

		return S_CONTINUE;
	});

	for(auto i = filters.begin(); i != filters.end(); i++)
	{
		std::wstring name = DirectShow::GetName(*i);

		wprintf(L"Removing %s\n", name.c_str());

		RemoveFilter(*i);
	}

	return S_OK;
}

STDMETHODIMP CFGManager::Dump()
{
	wprintf(L"Graph:\n");

	Foreach(this, [] (IBaseFilter* pBF) -> HRESULT
	{
		wprintf(L"Filter: %s (%p)\n", DirectShow::GetName(pBF).c_str(), pBF);

		Foreach(pBF, [] (IPin* pin) -> HRESULT
		{
			CComPtr<IPin> pPinTo = DirectShow::GetConnected(pin);

			CMediaTypeEx mt;

			if(pPinTo != NULL)
			{
				pin->ConnectionMediaType(&mt);
			}

			wprintf(L"Pin [%s, %s]: %s\n", 
				DirectShow::GetDirection(pin) == PINDIR_INPUT ? L"Input" : L"Output", 
				pPinTo ? (L"Connected to " + DirectShow::GetName(DirectShow::GetFilter(pPinTo)) + L", " + mt.ToString()).c_str() : L"Not Connected", 
				DirectShow::GetName(pin).c_str());

			return S_CONTINUE;
		});

		return S_CONTINUE;
	});

	return S_OK;
}

STDMETHODIMP CFGManager::GetVolume(float& volume)
{
	if(!m_inner) return E_UNEXPECTED;

	HRESULT hr;
	long lVolume = 0;
	hr = CComQIPtr<IBasicAudio>(m_inner)->get_Volume(&lVolume);
	if(FAILED(hr)) return E_FAIL;
	volume = pow(10, (float)std::min<float>(lVolume, 0.0f) / 5000);
	return S_OK;
}

STDMETHODIMP CFGManager::SetVolume(float volume)
{
	if(!m_inner) return E_UNEXPECTED;

	volume = std::min<float>(std::max<float>(volume, 0.0f), 1.0f);
	long lVolume = volume <= 0 ? -10000 : (int)(log10(std::max<float>(volume, 0.01f))*5000);
	return CComQIPtr<IBasicAudio>(m_inner)->put_Volume(lVolume);
}

STDMETHODIMP CFGManager::GetBalance(float& balance)
{
	if(!m_inner) return E_UNEXPECTED;

	HRESULT hr;
	long lBalance = 0;
	hr = CComQIPtr<IBasicAudio>(m_inner)->get_Balance(&lBalance);
	if(FAILED(hr)) return E_FAIL;
	int sign = lBalance < 0 ? -1 : +1;
	balance = sign * (1.0f - pow(10, (float)min(-abs(lBalance), 0) / 5000));
	return S_OK;
}

STDMETHODIMP CFGManager::SetBalance(float balance)
{
	if(!m_inner) return E_UNEXPECTED;

	balance = std::min<float>(std::max<float>(balance, -1.0f), 1.0f);
	int sign = balance < 0 ? -1 : +1;
	long lBalance = balance == 0 ? 0 : (int)(log10(std::max<float>(1.0f - std::min<float>(abs(balance), 1.0f), 0.01f)) * 5000) * sign;
	return CComQIPtr<IBasicAudio>(m_inner)->put_Balance(lBalance);
}

// 	CFGManagerCustom

CFGManagerCustom::CFGManagerCustom(LPCTSTR pName, LPUNKNOWN pUnk, UINT src, UINT tra, bool mt)
	: CFGManager(pName, pUnk, mt)
{
	InitSourceFilters(src);
	
	InitTransformFilters(tra);
/*
	// Overrides

	WORD merit_low = 1;

	POSITION pos = s.filters.GetTailPosition();

	while(pos)
	{
		FilterOverride* fo = s.filters.GetPrev(pos);

		if(fo->fDisabled || fo->type == FilterOverride::EXTERNAL && !CPath(MakeFullPath(fo->path)).FileExists()) 
			continue;

		ULONGLONG merit = 
			fo->iLoadType == FilterOverride::PREFERRED ? MERIT64_ABOVE_DSHOW : 
			fo->iLoadType == FilterOverride::MERIT ? MERIT64(fo->dwMerit) : 
			MERIT64_DO_NOT_USE; // fo->iLoadType == FilterOverride::BLOCKED

		merit += merit_low++;

		CFGFilter* f = NULL;

		if(fo->type == FilterOverride::REGISTERED)
		{
			f = new CFGFilterRegistry(fo->dispname, merit);
		}
		else if(fo->type == FilterOverride::EXTERNAL)
		{
			f = new CFGFilterFile(fo->clsid, fo->path, CStringW(fo->name), merit);
		}

		if(f)
		{
			f->SetTypes(fo->guids);
			m_override.AddTail(f);
		}
	}
*/
}

void CFGManagerCustom::InitSourceFilters(UINT src)
{
	// TODO: add mime types

	CFGFilter* f;

	{
		f = new CFGFilterRegistry(CLSID_WMAsfReader);
		f->m_mimes.push_back(L"video/x-ms-asf");
		f->m_mimes.push_back(L"video/x-ms-wmv");
		f->m_mimes.push_back(L"audio/x-ms-wma");
		f->m_mimes.push_back(L"video/x-ms-wvx");
		f->m_mimes.push_back(L"audio/x-ms-wax");
		f->m_mimes.push_back(L"video/x-ms-wm");
		f->m_mimes.push_back(L"video/x-ms-wmx");
		f->m_mimes.push_back(L"application/x-ms-wmz");
		f->m_mimes.push_back(L"application/x-ms-wmd");
		f->m_protocols.push_back(L"mms");
		m_source.push_back(f);
	}

	{
		f = new CFGFilterInternal<CRTMPSourceFilter>();
		f->m_protocols.push_back(L"rtmp");
		f->m_protocols.push_back(L"bbc");
		m_source.push_back(f);
	}

	{
		f = new CFGFilterInternal<CSilverlightSourceFilter>();
		f->m_protocols.push_back(L"http");
		f->m_protocols.push_back(L"silverlight");
		m_source.push_back(f);
	}

	{
		f = new CFGFilterInternal<CStrobeSourceFilter>();
		f->m_mimes.push_back(L"text/xml");
		f->m_mimes.push_back(L"application/xml");
		f->m_mimes.push_back(L"application/f4m+xml");
		f->m_protocols.push_back(L"http");
		f->m_protocols.push_back(L"strobe");
		f->m_extensions.push_back(L"f4m");
		m_source.push_back(f);
	}

	if(src & SRC_SHOUTCAST)
	{
		f = new CFGFilterInternal<CM3USourceFilter>();
		f->m_protocols.push_back(L"http");
		m_source.push_back(f);
	}

	if(src & SRC_HTTP)
	{
		f = new CFGFilterInternal<CHttpReader>();
		f->m_protocols.push_back(L"http");
		m_source.push_back(f);
	}
/*
	__if_exists(CUDPReader)
	{
	// if(src & SRC_UDP)
	{
		f = new CFGFilterInternal<CUDPReader>();
		f->m_protocols.push_back(L"udp");
		m_source.push_back(f);
	}
	}
*/
	if(src & SRC_AVI)
	{
		f = new CFGFilterInternal<CAVISourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,52494646,8,4,,41564920");
		f->m_chkbytes.push_back(L"0,4,,52494646,8,4,,41564958");
		m_source.push_back(f);
	}

	if(src & SRC_MP4)
	{
		f = new CFGFilterInternal<CMP4SourceFilter>();
		f->m_chkbytes.push_back(L"4,4,,66747970"); // ftyp
		f->m_chkbytes.push_back(L"4,4,,6d6f6f76"); // moov
		f->m_chkbytes.push_back(L"4,4,,6d646174"); // mdat
		f->m_chkbytes.push_back(L"4,4,,736b6970"); // skip
		f->m_chkbytes.push_back(L"4,12,ffffffff00000000ffffffff,77696465027fe3706d646174"); // wide ? mdat
		f->m_chkbytes.push_back(L"3,3,,000001"); // raw mpeg4 video
		f->m_extensions.push_back(L".mov");
		m_source.push_back(f);
	}

	if(src & SRC_FLV)
	{
		f = new CFGFilterInternal<CFLVSourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,464C5601"); // FLV (v1)
		m_source.push_back(f);
	}

	if(src & SRC_MATROSKA)
	{
		f = new CFGFilterInternal<CMatroskaSourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,1A45DFA3");
		m_source.push_back(f);
	}

	if(src & SRC_REALMEDIA)
	{
		f = new CFGFilterInternal<CRealMediaSourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,2E524D46");
		m_source.push_back(f);
	}

	if(src & SRC_DSM)
	{
		f = new CFGFilterInternal<CDSMSourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,44534D53");
		m_source.push_back(f);
		f = new CFGFilterInternal<CDSMMultiSourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,ACB2ACBB");
		m_source.push_back(f);
	}

	__if_exists(CFLICSource)
	{
	if(src & SRC_FLIC)
	{
		f = new CFGFilterInternal<CFLICSource>();
		f->m_chkbytes.push_back(L"4,2,,11AF");
		f->m_chkbytes.push_back(L"4,2,,12AF");
		f->m_extensions.push_back(L".fli");
		f->m_extensions.push_back(L".flc");
		m_source.push_back(f);
	}
	}

	if(src & SRC_CDDA)
	{
		f = new CFGFilterInternal<CCDDAReader>();
		f->m_extensions.push_back(L".cda");
		m_source.push_back(f);
	}

	if(src & SRC_CDXA)
	{
		f = new CFGFilterInternal<CCDXAReader>();
		f->m_chkbytes.push_back(L"0,4,,52494646,8,4,,43445841");
		m_source.push_back(f);
	}

	__if_exists(CVTSReader)
	{
	if(src & SRC_VTS)
	{
		f = new CFGFilterInternal<CVTSReader>();
		f->m_chkbytes.push_back(L"0,12,,445644564944454F2D565453");
		m_source.push_back(f);
	}
	}

	__if_exists(CD2VSource)
	{
	if(src & SRC_D2V)
	{
		f = new CFGFilterInternal<CD2VSource>();
		f->m_chkbytes.push_back(L"0,18,,4456443241564950726F6A65637446696C65");
		f->m_extensions.push_back(L".d2v"));
		m_source.push_back(f);
	}
	}

	__if_exists(CRadGtSourceFilter)
	{
	if(src & SRC_RADGT)
	{
		f = new CFGFilterInternal<CRadGtSourceFilter>();
		f->m_chkbytes.push_back(L"0,3,,534D4B");
		f->m_chkbytes.push_back(L"0,3,,42494B");
		f->m_extensions.push_back(L".smk");
		f->m_extensions.push_back(L".bik");
		m_source.push_back(f);
	}
	}

	__if_exists(CRoQSourceFilter)
	{
	if(src & SRC_ROQ)
	{
		f = new CFGFilterInternal<CRoQSourceFilter>();
		f->m_chkbytes.push_back(L"0,8,,8410FFFFFFFF1E00");
		m_source.push_back(f);
	}
	}

	if(src & SRC_OGG)
	{
		f = new CFGFilterInternal<COggSourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,4F676753");
		m_source.push_back(f);
	}

	__if_exists(CNutSourceFilter)
	{
	if(src & SRC_NUT)
	{
		f = new CFGFilterInternal<CNutSourceFilter>();
		f->m_chkbytes.push_back(L"0,8,,F9526A624E55544D");
		m_source.push_back(f);
	}
	}

	__if_exists(CDiracSourceFilter)
	{
	if(src & SRC_DIRAC)
	{
		f = new CFGFilterInternal<CDiracSourceFilter>();
		f->m_chkbytes.push_back(L"0,8,,4B572D4449524143");
		m_source.push_back(f);
	}
	}

	if(src & SRC_MPEG)
	{
		f = new CFGFilterInternal<CMPEGSourceFilter>();
		f->m_chkbytes.push_back(L"0,16,FFFFFFFFF100010001800001FFFFFFFF,000001BA2100010001800001000001BB");
		f->m_chkbytes.push_back(L"0,5,FFFFFFFFC0,000001BA40");
//		f->m_chkbytes.push_back(L"0,1,,47,188,1,,47,376,1,,47");
//		f->m_chkbytes.push_back(L"4,1,,47,196,1,,47,388,1,,47");
//		f->m_chkbytes.push_back(L"0,4,,54467263,1660,1,,47");
		f->m_chkbytes.push_back(L"0,8,fffffc00ffe00000,4156000055000000");
		m_source.push_back(f);

		f = new CFGFilterRegistry(CLSID_AsyncReader);
		f->m_chkbytes.push_back(L"0,1,,47,188,1,,47,376,1,,47");
		f->m_chkbytes.push_back(L"4,1,,47,196,1,,47,388,1,,47");
		f->m_chkbytes.push_back(L"0,4,,54467263,1660,1,,47");
		m_source.push_back(f);
	}

	if(src & SRC_DTSAC3)
	{
		f = new CFGFilterInternal<CDTSAC3Source>();
		f->m_chkbytes.push_back(L"0,4,,7FFE8001");
		f->m_chkbytes.push_back(L"0,8,,4454534844484452"); // DTSHDHDR
		f->m_chkbytes.push_back(L"0,2,,0B77");
		f->m_chkbytes.push_back(L"0,2,,770B");
		f->m_extensions.push_back(L".ac3");
		f->m_extensions.push_back(L".dts");
		f->m_extensions.push_back(L".dtshd");
		m_source.push_back(f);
	}

	if(src & SRC_MPA)
	{
		f = new CFGFilterInternal<CMPASourceFilter>();
		f->m_chkbytes.push_back(L"0,2,FFE0,FFE0");
		f->m_chkbytes.push_back(L"0,10,FFFFFF00000080808080,49443300000000000000");
		m_source.push_back(f);
	}

	if(src & SRC_FLAC)
	{
		f = new CFGFilterInternal<CFLACSourceFilter>();
		f->m_chkbytes.push_back(L"0,4,,664C6143"); // fLaC
		f->m_extensions.push_back(L".flac");
		m_source.push_back(f);
	}

/*
	if(AfxGetAppSettings().fUseWMASFReader)
	{
		f = new CFGFilterRegistry(CLSID_WMAsfReader);
		f->m_chkbytes.push_back(L"0,4,,3026B275");
		f->m_chkbytes.push_back(L"0,4,,D129E2D6");		
		m_source.push_back(f);
	}
*/
	if(src & SRC_AVI)
	{
		f = new CFGFilterInternal<CAVISplitterFilter>(L"AVI Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Avi);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	if(src & SRC_MP4)
	{
		f = new CFGFilterInternal<CMP4SplitterFilter>(L"MP4 Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MP4);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	if(src & SRC_FLV)
	{
		f = new CFGFilterInternal<CFLVSplitterFilter>(L"FLV Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_FLV);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	__if_exists(CAVI2AC3Filter)
	{
		f = new CFGFilterInternal<CAVI2AC3Filter>(L"AVI<->AC3/DTS", MERIT64(0x00680000) + 1);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WAVE_DOLBY_AC3);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WAVE_DTS);
		m_transform.push_back(f);
	}

	if(src & SRC_MATROSKA)
	{
		f = new CFGFilterInternal<CMatroskaSplitterFilter>(L"Matroska Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Matroska);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	if(src & SRC_REALMEDIA)
	{
		f = new CFGFilterInternal<CRealMediaSplitterFilter>(L"RealMedia Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_RealMedia);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	__if_exists(CRadGtSplitterFilter)
	{
	if(src & SRC_RADGT)
	{
		f = new CFGFilterInternal<CRadGtSplitterFilter>(L"RadGt Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Bink);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Smacker);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}
	}

	__if_exists(CRoQSplitterFilter)
	{
	if(src & SRC_ROQ)
	{
		f = new CFGFilterInternal<CRoQSplitterFilter>(L"RoQ Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_RoQ);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}
	}

	if(src & SRC_OGG)
	{
		f = new CFGFilterInternal<COggSplitterFilter>(L"Ogg Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Ogg);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	__if_exists(CNutSplitterFilter)
	{
	if(src & SRC_NUT)
	{
		f = new CFGFilterInternal<CNutSplitterFilter>(L"Nut Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Nut);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}
	}

	if(src & SRC_MPEG)
	{
		f = new CFGFilterInternal<CMPEGSplitterFilter>(L"MPEG Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG1System);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG2_PROGRAM);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG2_PVA);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	__if_exists(CDiracSplitterFilter)
	{
	if(src & SRC_DIRAC)
	{
		f = new CFGFilterInternal<CDiracSplitterFilter>(L"Dirac Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Dirac);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}
	}

	if(src & SRC_MPA)
	{
		f = new CFGFilterInternal<CMPASplitterFilter>(L"MPA Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG1Audio);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	if(src & SRC_DSM)
	{
		f = new CFGFilterInternal<CDSMSplitterFilter>(L"DSM Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_DirectShowMedia);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	if(src & SRC_TS)
	{
		f = new CFGFilterInternal<CTSDemuxFilter>(L"TS Demux", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG2_TRANSPORT);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}

	if(src & SRC_FLAC)
	{
		f = new CFGFilterInternal<CFLACSplitterFilter>(L"FLAC Splitter", MERIT64_ABOVE_DSHOW);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_FLAC_FRAMED);
		f->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_FLAC);
		f->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.push_back(f);
	}
}

void CFGManagerCustom::InitTransformFilters(UINT tra)
{
	CFGFilter* f;

	if(0)
	{
		CLSID clsid;

		Util::ToCLSID(L"{008BAC12-FBAF-497b-9670-BC6F6FBAE2C4}", clsid);
		
		// f = new CFGFilterFile(clsid, L"mpcvideodec.ax", L"MPC Video Decoder", MERIT64_DO_USE);
		f = new CFGFilterFile(clsid, L"C:\\Users\\Gabest\\Projects\\mpc-hc\\trunk\\src\\filters\\transform\\mpcvideodec\\Debug Unicode\\MPCVideoDec.ax", L"MPC Video Decoder", 
			MERIT64_ABOVE_DSHOW + 3);

		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG2_VIDEO);

		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_h264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_X264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_x264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_VSSH);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_vssh);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_DAVC);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_davc);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_PAVC);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_pavc);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVC1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_avc1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264_TRANSPORT);

		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_WVC1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_wvc1);

		m_transform.push_back(f);
	}

	// broadcom video decoder

	{
		CLSID clsid;

		Util::ToCLSID(L"{2DE1D17E-46B1-42A8-9AEC-E20E80D9B1A9}", clsid);

		f = new CFGFilterRegistry(clsid, MERIT64_ABOVE_DSHOW + 3);

		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG1Packet);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG1Payload);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVC1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_avc1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_h264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_X264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_x264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264_TRANSPORT);		

		m_transform.push_back(f);
	}

	// cyberlink h264

	if(0)
	{
		CLSID clsid;

		Util::ToCLSID(L"{45DE6BED-8A85-43ED-B331-56EE8C9B1B3F}", clsid);

		f = new CFGFilterRegistry(clsid, MERIT64_ABOVE_DSHOW + 2);

		f->RemoveTypes();

		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVC1);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_avc1);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_h264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_X264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_x264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264_TRANSPORT);		

		m_transform.push_back(f);
	}

	// cyberlink h264 (AMT)

	if(0)
	{
		CLSID clsid;

		Util::ToCLSID(L"{7B0B1CBD-A6D9-4E19-A49A-BFB3DB6DC1F2}", clsid);

		f = new CFGFilterRegistry(clsid, MERIT64_ABOVE_DSHOW + 2);

		f->RemoveTypes();

		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVC1);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_avc1);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_h264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_X264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_x264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264_TRANSPORT);

		m_transform.push_back(f);
	}

	// ms

	if(0)
	{
		CLSID clsid;

		Util::ToCLSID(L"{212690FB-83E5-4526-8FD7-74478B7939CD}", clsid);

		f = new CFGFilterRegistry(clsid, MERIT64_ABOVE_DSHOW + 2);

		f->RemoveTypes();

		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVC1);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_avc1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_h264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_X264);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_x264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264_TRANSPORT);

		m_transform.push_back(f);
	}

	{
		f = new CFGFilterInternal<CDXVADecoderFilter>(
			(tra & TRA_DXVA) ? L"DXVA Video Decoder" : L"DXVA Video Decoder (low merit)", 
			(tra & TRA_DXVA) ? MERIT64_ABOVE_DSHOW + 1 : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_h264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_X264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_x264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_VSSH);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_vssh);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_DAVC);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_davc);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_PAVC);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_pavc);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVC1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_avc1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264_TRANSPORT);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_WVC1);
		// f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_wvc1);
		m_transform.push_back(f);
	}

	{
		f = new CFGFilterInternal<CMPEG2DecoderFilter>(
			(tra & TRA_MPEG1) ? L"MPEG-1 Video Decoder" : L"MPEG-1 Video Decoder (low merit)", 
			(tra & TRA_MPEG1) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG1Packet);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG1Payload);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPEG2DecoderFilter>(
			(tra & TRA_MPEG2) ? L"MPEG-2 Video Decoder" : L"MPEG-2 Video Decoder (low merit)", 
			(tra & TRA_MPEG2) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MPEG2_VIDEO);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_MPEG2_VIDEO);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CH264DecoderFilter>(
			(tra & TRA_H264) ? L"H264 Video Decoder" : L"H264 Video Decoder (low merit)", 
			(tra & TRA_H264) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_h264);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVC1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_avc1);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_H264_TRANSPORT);
		m_transform.push_back(f);
	}

	{
		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_MPA) ? L"MPEG-1 Audio Decoder" : L"MPEG-1 Audio Decoder (low merit)",
			(tra & TRA_MPA) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MP3);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1AudioPayload);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1Payload);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1Packet);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_MPA) ? L"MPEG-2 Audio Decoder" : L"MPEG-2 Audio Decoder (low merit)",
			(tra & TRA_MPA) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MPEG2_AUDIO);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_MPEG2_AUDIO);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MPEG2_AUDIO);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG2_AUDIO);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_LPCM) ? L"LPCM Audio Decoder" : L"LPCM Audio Decoder (low merit)",
			(tra & TRA_LPCM) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_AC3) ? L"AC3 Audio Decoder" : L"AC3 Audio Decoder (low merit)",
			(tra & TRA_AC3) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_DOLBY_AC3);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_DOLBY_AC3);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_DOLBY_AC3);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DOLBY_AC3);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WAVE_DOLBY_AC3);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_DTS) ? L"DTS Decoder" : L"DTS Decoder (low merit)",
			(tra & TRA_DTS) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_DTS);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_DTS);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_DTS);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DTS);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WAVE_DTS);
		m_transform.push_back(f);
		
		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_AAC) ? L"AAC Decoder" : L"AAC Decoder (low merit)",
			(tra & TRA_AAC) ? MERIT64_ABOVE_DSHOW + 1 : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_AAC);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_AAC);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_AAC);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_AAC);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_LATM);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_LATM);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_LATM);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_LATM);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MP4A);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_MP4A);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MP4A);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MP4A);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_mp4a);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_mp4a);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_mp4a);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_mp4a);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_PS2AUD) ? L"PS2 Audio Decoder" : L"PS2 Audio Decoder (low merit)",
			(tra & TRA_PS2AUD) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_PS2_PCM);
		f->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_PS2_PCM);
		f->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_PS2_PCM);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PS2_PCM);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_VORBIS) ? L"Vorbis Audio Decoder" : L"Vorbis Audio Decoder (low merit)",
			(tra & TRA_VORBIS) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_Vorbis2);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CMPADecoderFilter>(
			(tra & TRA_FLAC) ? L"FLAC Decoder" : L"FLAC Decoder (low merit)",
			(tra & TRA_FLAC) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_FLAC_FRAMED);
		m_transform.push_back(f);
	}

	{
		f = new CFGFilterInternal<CRealVideoDecoder>(
			(tra & TRA_RV) ? L"RealVideo Decoder" : L"RealVideo Decoder (low merit)",
			(tra & TRA_RV) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_RV10);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_RV20);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_RV30);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_RV40);
		m_transform.push_back(f);
	}

	{
		f = new CFGFilterInternal<CRealAudioDecoder>(
			(tra & TRA_RA) ? L"RealAudio Decoder" : L"RealAudio Decoder (low merit)",
			(tra & TRA_RA) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_14_4);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_28_8);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ATRC);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_COOK);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DNET);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_SIPR);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_RAAC);
		m_transform.push_back(f);
	}

	__if_exists(CRoQVideoDecoder)
	{
		f = new CFGFilterInternal<CRoQVideoDecoder>(
			(tra & TRA_RV) ? L"RoQ Video Decoder" : L"RoQ Video Decoder (low merit)",
			(tra & TRA_RV) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_RoQV);
		m_transform.push_back(f);
	}

	__if_exists(CRoQAudioDecoder)
	{
		f = new CFGFilterInternal<CRoQAudioDecoder>(
			(tra & TRA_RA) ? L"RoQ Audio Decoder" : L"RoQ Audio Decoder (low merit)",
			(tra & TRA_RA) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_RoQA);
		m_transform.push_back(f);
	}

	__if_exists(CDiracVideoDecoder)
	{
		f = new CFGFilterInternal<CDiracVideoDecoder>(
			(tra & TRA_DIRAC) ? L"Dirac Video Decoder" : L"Dirac Video Decoder (low merit)",
			(tra & TRA_DIRAC) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_DiracVideo);
		m_transform.push_back(f);
	}

	{
		f = new CFGFilterInternal<CNullTextRenderer>(L"NullTextRenderer", MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Text, MEDIASUBTYPE_NULL);
		f->AddType(MEDIATYPE_ScriptCommand, MEDIASUBTYPE_NULL);
		f->AddType(MEDIATYPE_Subtitle, MEDIASUBTYPE_NULL);
		f->AddType(MEDIATYPE_Text, MEDIASUBTYPE_NULL);
		f->AddType(MEDIATYPE_NULL, MEDIASUBTYPE_DVD_SUBPICTURE);
		f->AddType(MEDIATYPE_NULL, MEDIASUBTYPE_CVD_SUBPICTURE);
		f->AddType(MEDIATYPE_NULL, MEDIASUBTYPE_SVCD_SUBPICTURE);
		m_transform.push_back(f);
	}

	__if_exists(CFLVVideoDecoder)
	{
		f = new CFGFilterInternal<CFLVVideoDecoder>(
			(tra & TRA_FLV4) ? L"FLV Video Decoder" : L"FLV Video Decoder (low merit)",
			(tra & TRA_FLV4) ? MERIT64_PREFERRED : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_FLV4);
		m_transform.push_back(f);
		
		f = new CFGFilterInternal<CFLVVideoDecoder>(
			(tra & TRA_VP62) ? L"VP62 Video Decoder" : L"VP62 Video Decoder (low merit)",
			(tra & TRA_VP62) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_VP62);
		m_transform.push_back(f);
	}

	{
		f = new CFGFilterInternal<CTeletextFilter>(L"Teletext Filter", MERIT64_DO_USE);
		f->AddType(MEDIATYPE_TeletextPacket, GUID_NULL);
		m_transform.push_back(f);

		f = new CFGFilterInternal<CTeletextRenderer>(L"Teletext Renderer", MERIT64_NORMAL + 1);
		f->AddType(MEDIATYPE_VBI, MEDIASUBTYPE_TELETEXT);
		m_transform.push_back(f);
/*
		f = new CFGFilterFile(CLSID_WSTDecoder, L"wstdecod.dll", L"WST Decoder", MERIT64_DO_USE);
		m_transform.push_back(f);
*/
	}

	// blocked filters

	{
		// "Subtitle Mixer" makes an access violation around the 
		// 11-12th media type when enumerating them on its output.

		CLSID clsid;

		Util::ToCLSID(L"{00A95963-3BE5-48C0-AD9F-3356D67EA09D}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// ISCR suxx
	
		CLSID clsid;

		Util::ToCLSID(L"{48025243-2D39-11CE-875D-00608CB78066}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// Samsung's "mpeg-4 demultiplexor" can even open matroska files, amazing...
		
		CLSID clsid;

		Util::ToCLSID(L"{99EC0C72-4D1B-411B-AB1F-D561EE049D94}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// LG Video Renderer (lgvid.ax) just crashes when trying to connect it
	
		CLSID clsid;

		Util::ToCLSID(L"{9F711C60-0668-11D0-94D4-0000C02BA972}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// palm demuxer crashes (even crashes graphedit when dropping an .ac3 onto it)
		
		CLSID clsid;

		Util::ToCLSID(L"{BE2CF8A7-08CE-4A2C-9A25-FD726A999196}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	// DCDSPFilter (early versions crash mpc)

	{
		CRegKey key;

		WCHAR buff[256] = {0};
		ULONG len = sizeof(buff);

		std::wstring str = L"{B38C58A0-1809-11D6-A458-EDAE78F1DF12}";

		CLSID clsid;

		Util::ToCLSID(str.c_str(), clsid);

		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, (L"CLSID\\" + str + L"\\InprocServer32").c_str(), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len)
		&& DirectShow::GetFileVersion(buff) < 0x0001000000030000ui64)
		{
			m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
		}
	}
/*
	{
		// NVIDIA Transport Demux crashed for someone, I could not reproduce it

		CLSID clsid;

		Util::ToCLSID(L"{735823C1-ACC4-11D3-85AC-006008376FB8}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));	
	}
*/
	{
		// mainconcept color converter

		CLSID clsid;

		Util::ToCLSID(L"{272D77A0-A852-4851-ADA4-9091FEAD4C86}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// cyberlink h264 decoder (blank output)
		
		CLSID clsid;

		Util::ToCLSID(L"{D12E285B-3B29-4416-BA8E-79BD81D193CC}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// nero video decoder (debugger protection)
		
		CLSID clsid;

		Util::ToCLSID(L"{C0BA9CF8-96E0-4C34-B5DE-E92C3FC05ED6}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));	
	}

	{
		// nero dvd decoder
		
		CLSID clsid;

		Util::ToCLSID(L"{DCD6EADC-EE69-47DD-B934-95573296039C}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// moonlight mpeg2 demux (reads the whole file thru even if not an mpeg)
		
		CLSID clsid;

		Util::ToCLSID(L"{731B8592-4001-46D4-B1A5-33EC792B4501}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	{
		// monogram encoders default merit is unlikely, change it to do not use

		m_transform.push_back(new CFGFilterRegistry(CLSID_MonogramX264, MERIT64_DO_NOT_USE));
		m_transform.push_back(new CFGFilterRegistry(CLSID_MonogramAACEncoder, MERIT64_DO_NOT_USE));		
	}
}

// IFilterGraph

STDMETHODIMP CFGManagerCustom::AddFilter(IBaseFilter* pBF, LPCWSTR pName)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	hr = __super::AddFilter(pBF, pName);

	if(FAILED(hr))
	{
		return hr;
	}

	if(DirectShow::GetCLSID(pBF) == CLSID_DMOWrapperFilter)
	{
		if(CComQIPtr<IPropertyBag> pPB = pBF)
		{
			CComVariant var(true);

			pPB->Write(CComBSTR(L"_HIRESOUTPUT"), &var);
		}
	}
/*
	if(CComQIPtr<IAudioSwitcherFilter> pASF = pBF)
	{
		AppSettings& s = AfxGetAppSettings();
		pASF->EnableDownSamplingTo441(s.fDownSampleTo441);
		pASF->SetSpeakerConfig(s.fCustomChannelMapping, s.pSpeakerToChannelMap);
		pASF->SetAudioTimeShift(s.fAudioTimeShift ? 10000i64*s.tAudioTimeShift : 0);
		pASF->SetNormalizeBoost(s.fAudioNormalize, s.fAudioNormalizeRecover, s.AudioBoost);
	}

	if(CComQIPtr<IMPEG2DecoderFilter> pM2DF = pBF)
	{
		pM2DF->SetBrightness(-16);
		pM2DF->SetContrast(255.0f / (235 - 16));
	}
*/
	return hr;
}

// 	CFGManagerPlayer

CFGManagerPlayer::CFGManagerPlayer(LPCTSTR pName, LPUNKNOWN pUnk, UINT src, UINT tra, bool mt)
	: CFGManagerCustom(pName, pUnk, src, tra, mt)
	, m_vrmerit(MERIT64(MERIT_PREFERRED))
	, m_armerit(MERIT64(MERIT_PREFERRED))
{
	CFGFilter* f;

	if(m_fm)
	{
		CComPtr<IEnumMoniker> pEM;

		GUID types[] = {MEDIATYPE_Video, MEDIASUBTYPE_NULL};

		HRESULT hr = m_fm->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE + 1, TRUE, 1, types, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL);

		if(SUCCEEDED(hr))
		{
			for(CComPtr<IMoniker> moniker; S_OK == pEM->Next(1, &moniker, NULL); moniker = NULL)
			{
				m_vrmerit = max(m_vrmerit, CFGFilterRegistry(moniker).GetMerit());
			}
		}

		m_vrmerit += 0x100;
	}

	if(m_fm)
	{
		CComPtr<IEnumMoniker> pEM;

		GUID types[] = {MEDIATYPE_Audio, MEDIASUBTYPE_NULL};

		HRESULT hr = m_fm->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE + 1, TRUE, 1, types, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL);

		if(SUCCEEDED(hr))
		{
			for(CComPtr<IMoniker> moniker; S_OK == pEM->Next(1, &moniker, NULL); moniker = NULL)
			{
				m_armerit = max(m_armerit, CFGFilterRegistry(moniker).GetMerit());
			}
		}

		Foreach(CLSID_AudioRendererCategory, [this] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
		{
			UINT64 merit = CFGFilterRegistry(m).GetMerit();

			m_armerit = max(m_armerit, merit);

			return S_CONTINUE;
		});

		m_armerit += 0x100;
	}

	// switchers

	// if(s.fEnableAudioSwitcher)
	{
		f = new CFGFilterInternal<CAudioSwitcherFilter>(L"Audio Switcher", m_armerit + 0x100);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
		m_transform.push_back(f);

		// morgan stream switcher

		CLSID clsid;

		Util::ToCLSID(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}

	/*

	TODO: set vmr9 renderless

	// Renderers

	if(s.iDSVideoRendererType == VIDRNDT_DS_OLDRENDERER)
		m_transform.AddTail(new CFGFilterRegistry(CLSID_VideoRenderer, m_vrmerit));
	else if(s.iDSVideoRendererType == VIDRNDT_DS_OVERLAYMIXER)
		m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_OverlayMixer, L"Overlay Mixer", m_vrmerit));
	else if(s.iDSVideoRendererType == VIDRNDT_DS_VMR7WINDOWED)
		m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_VideoMixingRenderer, L"Video Mixing Render 7 (Windowed)", m_vrmerit));
	else if(s.iDSVideoRendererType == VIDRNDT_DS_VMR9WINDOWED)
		m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_VideoMixingRenderer9, L"Video Mixing Render 9 (Windowed)", m_vrmerit));
	else if(s.iDSVideoRendererType == VIDRNDT_DS_VMR7RENDERLESS)
		m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_VMR7AllocatorPresenter, L"Video Mixing Render 7 (Renderless)", m_vrmerit));
	else if(s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS)
		m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_VMR9AllocatorPresenter, L"Video Mixing Render 9 (Renderless)", m_vrmerit));
	else if(s.iDSVideoRendererType == VIDRNDT_DS_DXR)
		m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_DXRAllocatorPresenter, L"Haali's Video Renderer", m_vrmerit));
	else if(s.iDSVideoRendererType == VIDRNDT_DS_NULL_COMP)
	{
		f = new CFGFilterInternal<CNullVideoRenderer>(L"Null Video Renderer (Any)", MERIT64_ABOVE_DSHOW+2);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
		m_transform.push_back(f)
	}
	else if(s.iDSVideoRendererType == VIDRNDT_DS_NULL_UNCOMP)
	{
		f = new CFGFilterInternal<CNullUVideoRenderer>(L"Null Video Renderer (Uncompressed)", MERIT64_ABOVE_DSHOW+2);
		f->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
		m_transform.push_back(f)
	}

	if(s.AudioRendererDisplayName == AUDRNDT_NULL_COMP)
	{
		f = new CFGFilterInternal<CNullAudioRenderer>(AUDRNDT_NULL_COMP, MERIT64_ABOVE_DSHOW+2);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
		m_transform.push_back(f)
	}
	else if(s.AudioRendererDisplayName == AUDRNDT_NULL_UNCOMP)
	{
		f = new CFGFilterInternal<CNullUAudioRenderer>(AUDRNDT_NULL_UNCOMP, MERIT64_ABOVE_DSHOW+2);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
		m_transform.push_back(f)
	}
	else if(!s.AudioRendererDisplayName.IsEmpty())
	{
		f = new CFGFilterRegistry(s.AudioRendererDisplayName, m_armerit);
		f->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
		m_transform.push_back(f)
	}
	*/
}

STDMETHODIMP CFGManagerPlayer::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);
/*
	if(GetCLSID(pPinOut) == CLSID_MPEG2Demultiplexer)
	{
		CComQIPtr<IMediaSeeking> pMS = pPinOut;
		REFERENCE_TIME rtDur = 0;
		if(!pMS || FAILED(pMS->GetDuration(&rtDur)) || rtDur <= 0)
			 return E_FAIL;
	}
*/
	return __super::ConnectDirect(pPinOut, pPinIn, pmt);
}

// CFGManagerDVD

CFGManagerDVD::CFGManagerDVD(LPCTSTR pName, LPUNKNOWN pUnk, UINT src, UINT tra, bool mt)
	: CFGManagerPlayer(pName, pUnk, src, tra, mt)
{
/*
	AppSettings& s = AfxGetAppSettings();

	// have to avoid the old video renderer
	if(!s.fXpOrBetter && s.iDSVideoRendererType != VIDRNDT_DS_OVERLAYMIXER || s.iDSVideoRendererType == VIDRNDT_DS_OLDRENDERER)
		m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_OverlayMixer, L"Overlay Mixer", m_vrmerit-1));
*/
	{
		// elecard's decoder isn't suited for dvd playback (atm)

		CLSID clsid;

		Util::ToCLSID(L"{F50B3F13-19C4-11CF-AA9A-02608C9BABA2}", clsid);

		m_transform.push_back(new CFGFilterRegistry(clsid, MERIT64_DO_NOT_USE));
	}
}
/*
#include "..\..\decss\VobFile.h"

class CResetDVD : public CDVDSession
{
public:
	CResetDVD(LPCTSTR path)
	{
		if(Open(path))
		{
			if(BeginSession()) {Authenticate(); EndSession();}
			Close();
		}
	}
};
*/
STDMETHODIMP CFGManagerDVD::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	CComPtr<IBaseFilter> pBF;

	hr = AddSourceFilter(lpcwstrFile, lpcwstrFile, &pBF);

	if(FAILED(hr))
	{
		return hr;
	}

	return ConnectFilter(pBF, NULL);
}

[uuid("482d10b6-376e-4411-8a17-833800A065DB")]
class RatDVDNavigator {};

STDMETHODIMP CFGManagerDVD::AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
	CAutoLock cAutoLock(this);

	CheckPointer(lpcwstrFileName, E_POINTER);
	CheckPointer(ppFilter, E_POINTER);

	HRESULT hr;

	std::wstring protocol;
	std::wstring filename;
	std::wstring extension;

	SplitFilename(lpcwstrFileName, protocol, filename, extension);

	CComPtr<IBaseFilter> pBF;

	hr = pBF.CoCreateInstance(extension == L".ratdvd" ? __uuidof(RatDVDNavigator) : CLSID_DVDNavigator);

	if(FAILED(hr))
	{
		return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
	}

	hr = AddFilter(pBF, L"DVD Navigator");

	if(FAILED(hr))
	{
		return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
	}

	CComQIPtr<IDvdControl2> control = pBF;
	CComQIPtr<IDvdInfo2> info = pBF;

	if(control == NULL || info == NULL)
	{
		return E_NOINTERFACE;
	}

	if(!filename.empty())
	{
		if(FAILED(hr = control->SetDVDDirectory(filename.c_str()))
		&& FAILED(hr = control->SetDVDDirectory((filename + L"VIDEO_TS").c_str()))
		&& FAILED(hr = control->SetDVDDirectory((filename + L"\\VIDEO_TS").c_str())))
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		WCHAR buff[MAX_PATH];
		ULONG len;

		hr = info->GetDVDDirectory(buff, countof(buff), &len);

		if(FAILED(hr) || len == 0)
		{
			return hr;
		}
	}

	control->SetOption(DVD_ResetOnStop, FALSE);
	control->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);
/*
	if(clsid == CLSID_DVDNavigator)
	{
		CResetDVD(buff);
	}
*/
	*ppFilter = pBF.Detach();

	return S_OK;
}

// CFGAggregator

CFGAggregator::CFGAggregator(const CLSID& clsid, LPCTSTR pName, LPUNKNOWN pUnk, HRESULT& hr)
	: CUnknown(pName, pUnk)
{
	hr = m_inner.CoCreateInstance(clsid, GetOwner());
}

CFGAggregator::~CFGAggregator()
{
	m_inner.Release();
}

STDMETHODIMP CFGAggregator::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return
		m_inner && (riid != IID_IUnknown && SUCCEEDED(m_inner->QueryInterface(riid, ppv))) ? S_OK :
		__super::NonDelegatingQueryInterface(riid, ppv);
}
