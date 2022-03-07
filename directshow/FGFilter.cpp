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

#include "stdafx.h"
#include "DirectShow.h"
#include "FGFilter.h"
#include "moreuuids.h"

// CFGFilter

CFGFilter::CFGFilter(const CLSID& clsid, LPCWSTR name, UINT64 merit)
	: m_clsid(clsid)
	, m_name(name)
{
	m_merit.val = merit;
}

void CFGFilter::RemoveTypes()
{
	m_types.clear();
}

void CFGFilter::SetTypes(const std::list<GUID>& types)
{
	m_types = types;
}

void CFGFilter::AddType(const GUID& majortype, const GUID& subtype)
{
	m_types.push_back(majortype);
	m_types.push_back(subtype);
}

bool CFGFilter::Match(const std::list<CMediaType>& mts, bool exact)
{
	for(auto i = m_types.begin(); i != m_types.end(); )
	{
		const GUID& majortype = *i++;

		if(i == m_types.end()) {ASSERT(0); break;}

		const GUID& subtype = *i++;

		for(auto j = mts.begin(); j != mts.end(); j++)
		{
			if(exact)
			{
				if(majortype == j->majortype && majortype != GUID_NULL 
				&& subtype == j->subtype && subtype != GUID_NULL)
				{
            		return true;
				}
			}
			else
			{
				if((majortype == GUID_NULL || j->majortype == GUID_NULL || j->majortype == majortype)
				&& (subtype == GUID_NULL || j->subtype == GUID_NULL || j->subtype == subtype))
				{
					return true;
				}
			}
		}
	}

	return false;
}

// CFGFilterRegistry

CFGFilterRegistry::CFGFilterRegistry(IMoniker* moniker, UINT64 merit) 
	: CFGFilter(GUID_NULL, L"", merit)
	, m_moniker(moniker)
{
	LPOLESTR str = NULL;
	
	if(m_moniker == NULL || FAILED(m_moniker->GetDisplayName(0, 0, &str))) 
	{
		return;
	}

	m_dispname = m_name = str;

	CoTaskMemFree(str); str = NULL;

	CComPtr<IPropertyBag> pPB;

	if(SUCCEEDED(m_moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
	{
		CComVariant var;

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
		{
			m_name = var.bstrVal;

			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("CLSID")), &var, NULL)))
		{
			CLSIDFromString(var.bstrVal, &m_clsid);

			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
		{			
			BSTR* str = NULL;

			if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&str)))
			{
				ExtractFilterData((BYTE*)str, var.parray->cbElements * (var.parray->rgsabound[0].cElements));

				SafeArrayUnaccessData(var.parray);
			}

			var.Clear();
		}
	}

	if(merit != MERIT64_DO_USE) 
	{
		m_merit.val = merit;
	}
}

CFGFilterRegistry::CFGFilterRegistry(LPCWSTR dispname, UINT64 merit) 
	: CFGFilter(GUID_NULL, L"", merit)
	, m_dispname(dispname)
{
	CComPtr<IBindCtx> pBC;

	CreateBindCtx(0, &pBC);

	ULONG chEaten;

	if(S_OK != MkParseDisplayName(pBC, CComBSTR(m_dispname.c_str()), &chEaten, &m_moniker))
	{
		return;
	}

	CComPtr<IPropertyBag> pPB;

	if(SUCCEEDED(m_moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
	{
		CComVariant var;

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
		{
			m_name = var.bstrVal;

			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("CLSID")), &var, NULL)))
		{
			CLSIDFromString(var.bstrVal, &m_clsid);

			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
		{			
			BSTR* str = NULL;

			if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&str)))
			{
				ExtractFilterData((BYTE*)str, var.parray->cbElements * (var.parray->rgsabound[0].cElements));

				SafeArrayUnaccessData(var.parray);
			}

			var.Clear();
		}
	}

	if(merit != MERIT64_DO_USE) 
	{
		m_merit.val = merit;
	}
}

CFGFilterRegistry::CFGFilterRegistry(const CLSID& clsid, UINT64 merit) 
	: CFGFilter(clsid, L"", merit)
{
	std::wstring str;
	
	Util::ToString(m_clsid, str);

	CRegKey key;

	if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, (L"CLSID\\" + str).c_str(), KEY_READ))
	{
		ULONG chars = 0;

		if(ERROR_SUCCESS == key.QueryStringValue(NULL, NULL, &chars))
		{
			wchar_t* name = new wchar_t[chars + 1];

			if(ERROR_SUCCESS == key.QueryStringValue(NULL, name, &chars))
			{
				m_name = name;
			}

			delete [] name;
		}

		key.Close();
	}

	CRegKey catkey;

	if(ERROR_SUCCESS == catkey.Open(HKEY_CLASSES_ROOT, L"CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance", KEY_READ))
	{
		if(ERROR_SUCCESS != key.Open(catkey, str.c_str(), KEY_READ))
		{
			// illiminable pack uses the name of the filter and not the clsid, have to enum all keys to find it...

			FILETIME ft;

			TCHAR buff[256];
			DWORD len = countof(buff);

			for(DWORD i = 0; ERROR_SUCCESS == catkey.EnumKey(i, buff, &len, &ft); i++, len = countof(buff))
			{
				if(ERROR_SUCCESS == key.Open(catkey, buff, KEY_READ))
				{
					len = countof(buff);

					if(ERROR_SUCCESS == key.QueryStringValue(L"CLSID", buff, &len))
					{
						CLSID clsid = GUID_NULL;

						Util::ToCLSID(buff, clsid);

						if(clsid == m_clsid)
						{
							break;
						}
					}

					key.Close();
				}
			}
		}

		if(key)
		{
			ULONG chars = 0;

			if(ERROR_SUCCESS == key.QueryStringValue(L"FriendlyName", NULL, &chars))
			{
				wchar_t* name = new wchar_t[chars];

				if(ERROR_SUCCESS == key.QueryStringValue(L"FriendlyName", name, &chars))
				{
					m_name = name;
				}

				delete [] name;
			}

			ULONG bytes = 0;

			if(ERROR_SUCCESS == key.QueryBinaryValue(L"FilterData", NULL, &bytes))
			{
				BYTE* buff = new BYTE[bytes];

				if(ERROR_SUCCESS == key.QueryBinaryValue(L"FilterData", buff, &bytes))
				{
					ExtractFilterData(buff, bytes);
				}

				delete [] buff;
			}

			key.Close();
		}
	}

	if(merit != MERIT64_DO_USE) 
	{
		m_merit.val = merit;
	}
}

#include "..\3rdparty\detours\detours.h"

static bool s_bIsDebuggerPresentDetoured = false;
static bool s_bIsDebuggerPresent = false;

DETOUR_TRAMPOLINE(BOOL WINAPI Real_IsDebuggerPresent(), IsDebuggerPresent);

BOOL WINAPI Mine_IsDebuggerPresent()
{
	return s_bIsDebuggerPresent || Real_IsDebuggerPresent() ? TRUE : FALSE;
}

[uuid("FC772AB0-0C7F-11D3-8FF2-00A0C9224CF4")] struct BDA_MPEG2_TIF {};

HRESULT CFGFilterRegistry::Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks)
{
	CheckPointer(ppBF, E_POINTER);

	HRESULT hr = E_FAIL;
	
	if(m_moniker != NULL)
	{
		if(m_dispname == L"@device:sw:{A2E3074F-6C3D-11D3-B653-00C04F79498E}\\BDA MPEG2 Transport Information Filter")
		{
/*
			if(!s_bIsDebuggerPresentDetoured)
			{
				DetourFunctionWithTrampoline((PBYTE)Real_IsDebuggerPresent, (PBYTE)Mine_IsDebuggerPresent);

				s_bIsDebuggerPresentDetoured = true;
			}
*/
			s_bIsDebuggerPresent = true;
		}

		hr = m_moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)ppBF);

		s_bIsDebuggerPresent = false;

		if(SUCCEEDED(hr))
		{
			m_clsid = DirectShow::GetCLSID(*ppBF);
		}
	}
	else if(m_clsid != GUID_NULL)
	{
		CComQIPtr<IBaseFilter> pBF;

		if(FAILED(pBF.CoCreateInstance(m_clsid))) 
		{
			return E_FAIL;
		}

		*ppBF = pBF.Detach();
		
		hr = S_OK;
	}

	return hr;
};

[uuid("97f7c4d4-547b-4a5f-8332-536430ad2e4d")]
interface IAMFilterData : public IUnknown
{
	STDMETHOD(ParseFilterData)(BYTE* rgbFilterData, ULONG cb, BYTE** prgbRegFilter2) PURE;
	STDMETHOD(CreateFilterData)(REGFILTER2* prf2, BYTE** prgbFilterData, ULONG* pcb) PURE;
};

void CFGFilterRegistry::ExtractFilterData(BYTE* p, UINT len)
{
	CComPtr<IAMFilterData> pFD;
	BYTE* ptr = NULL;

	if(SUCCEEDED(pFD.CoCreateInstance(CLSID_FilterMapper2))
	&& SUCCEEDED(pFD->ParseFilterData(p, len, (BYTE**)&ptr)))
	{
		REGFILTER2* prf = (REGFILTER2*)*(DWORD*)ptr; // this is f*cked up

		m_merit.mid = prf->dwMerit;

		if(prf->dwVersion == 1)
		{
			for(int i = 0; i < prf->cPins; i++)
			{
				if(prf->rgPins[i].bOutput)
				{
					continue;
				}

				for(int j = 0; j < prf->rgPins[i].nMediaTypes; j++)
				{
					const REGPINTYPES& rpt = prf->rgPins[i].lpMediaType[j];

					if(rpt.clsMajorType != NULL && rpt.clsMinorType != NULL)
					{
						AddType(*rpt.clsMajorType, *rpt.clsMinorType);
					}
					else
					{
						break;
					}
				}
			}
		}
		else if(prf->dwVersion == 2)
		{
			for(UINT i = 0; i < prf->cPins2; i++)
			{
				if(prf->rgPins2[i].dwFlags & REG_PINFLAG_B_OUTPUT)
				{
					continue;
				}

				for(int j = 0; j < prf->rgPins2[i].nMediaTypes; j++)
				{
					const REGPINTYPES& rpt = prf->rgPins2[i].lpMediaType[j];

					if(rpt.clsMajorType != NULL && rpt.clsMinorType != NULL)
					{
						AddType(*rpt.clsMajorType, *rpt.clsMinorType);
					}
					else
					{
						break;
					}
				}
			}
		}

		CoTaskMemFree(prf);
	}
	else
	{
		BYTE* base = p;

		#define ChkLen(size) if(p - base + size > (int)len) return;

		ChkLen(4)
		if(*(DWORD*)p != 0x00000002) return; // only version 2 supported, no samples found for 1
		p += 4;

		ChkLen(4)
		m_merit.mid = *(DWORD*)p; p += 4;

		m_types.clear();

		ChkLen(8)
		DWORD nPins = *(DWORD*)p; p += 8;
		while(nPins-- > 0)
		{
			ChkLen(1)
			BYTE n = *p-0x30; p++;
			
			ChkLen(2)
			WORD pi = *(WORD*)p; p += 2;
			ASSERT(pi == 'ip');

			ChkLen(1)
			BYTE x33 = *p; p++;
			ASSERT(x33 == 0x33);

			ChkLen(8)
			bool fOutput = !!(*p&REG_PINFLAG_B_OUTPUT);
			p += 8;

			ChkLen(12)
			DWORD nTypes = *(DWORD*)p; p += 12;
			while(nTypes-- > 0)
			{
				ChkLen(1)
				BYTE n = *p-0x30; p++;

				ChkLen(2)
				WORD ty = *(WORD*)p; p += 2;
				ASSERT(ty == 'yt');

				ChkLen(5)
				BYTE x33 = *p; p++;
				ASSERT(x33 == 0x33);
				p += 4;

				ChkLen(8)
				if(*(DWORD*)p < (p-base+8) || *(DWORD*)p >= len 
				|| *(DWORD*)(p+4) < (p-base+8) || *(DWORD*)(p+4) >= len)
				{
					p += 8;
					continue;
				}

				GUID majortype, subtype;
				memcpy(&majortype, &base[*(DWORD*)p], sizeof(GUID)); p += 4;
				if(!fOutput) AddType(majortype, subtype); 
			}
		}

		#undef ChkLen
	}
}

// CFGFilterFile

CFGFilterFile::CFGFilterFile(const CLSID& clsid, LPCWSTR path, LPCWSTR name, UINT64 merit)
	: CFGFilter(clsid, name, merit)
	, m_path(path)
	, m_hInst(NULL)
{
}

HRESULT CFGFilterFile::Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks)
{
	CheckPointer(ppBF, E_POINTER);

	return DirectShow::LoadExternalObject(m_path.c_str(), m_clsid, __uuidof(IBaseFilter), (void**)ppBF);
}

// CFGFilterVideoRenderer

CFGFilterVideoRenderer::CFGFilterVideoRenderer(HWND hWnd, const CLSID& clsid, LPCWSTR name, UINT64 merit) 
	: CFGFilter(clsid, name, merit)
	, m_hWnd(hWnd)
{
	AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
}

HRESULT CFGFilterVideoRenderer::Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks)
{
	CheckPointer(ppBF, E_POINTER);

	HRESULT hr = S_OK;
/*
	CComPtr<ISubPicAllocatorPresenter> pCAP;

	if(m_clsid == CLSID_VMR7AllocatorPresenter 
	|| m_clsid == CLSID_VMR9AllocatorPresenter 
	|| m_clsid == CLSID_DXRAllocatorPresenter)
	{
		if(SUCCEEDED(CreateAP7(m_clsid, m_hWnd, &pCAP))
		|| SUCCEEDED(CreateAP9(m_clsid, m_hWnd, &pCAP)))
		{
			CComPtr<IUnknown> pRenderer;

			if(SUCCEEDED(hr = pCAP->CreateRenderer(&pRenderer)))
			{
				*ppBF = CComQIPtr<IBaseFilter>(pRenderer).Detach();

				unks.AddTail(pCAP);
			}
		}
	}
	else
*/	{
		CComPtr<IBaseFilter> pBF;

		if(SUCCEEDED(pBF.CoCreateInstance(m_clsid)))
		{
			Foreach(pBF, PINDIR_INPUT, [&unks] (IPin* pin) -> HRESULT
			{
				CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> pMPC = pin;

				if(pMPC != NULL)
				{
					CComPtr<IUnknown> unk = pMPC;

					unks.push_back(unk);

					return S_OK;
				}

				return S_CONTINUE;
			});

			*ppBF = pBF.Detach();
		}
	}

	if(*ppBF == NULL)
	{
		hr = E_FAIL;
	}

	return hr;
}

// CFGFilterList

CFGFilterList::CFGFilterList()
{
}

CFGFilterList::~CFGFilterList()
{
	RemoveAll();
}

void CFGFilterList::RemoveAll()
{
	for(auto i = m_filters.begin(); i != m_filters.end(); i++)
	{
		const Filter& f = *i;

		if(f.autodelete)
		{
			delete f.filter;
		}
	}

	m_filters.clear();

	m_sortedfilters.clear();
}

void CFGFilterList::Insert(CFGFilter* filter, int group, bool exactmatch, bool autodelete)
{
	// dup?

	if(CFGFilterRegistry* fr1 = dynamic_cast<CFGFilterRegistry*>(filter))
	{
		for(auto i = m_filters.begin(); i != m_filters.end(); i++)
		{
			if(group == i->group)
			{
				if(CFGFilterRegistry* fr2 = dynamic_cast<CFGFilterRegistry*>(i->filter))
				{
					bool moniker = fr1->GetMoniker() && fr2->GetMoniker() && S_OK == fr1->GetMoniker()->IsEqual(fr2->GetMoniker());
					bool clsid = fr1->GetCLSID() != GUID_NULL && fr1->GetCLSID() == fr2->GetCLSID();

					if(moniker || clsid)
					{
						if(autodelete)
						{
							delete filter;
						}

						return;
					}
				}
			}
		}
	}

	for(auto i = m_filters.begin(); i != m_filters.end(); i++)
	{
		if(i->filter == filter)
		{
			return;
		}
	}

	Filter f;
	
	f.index = m_filters.size();
	f.group = group;
	f.exactmatch = exactmatch;
	f.autodelete = autodelete;
	f.filter = filter;
	
	m_filters.push_back(f);

	m_sortedfilters.clear();
}

void CFGFilterList::GetFilters(std::list<CFGFilter*>& filters)
{
	std::vector<const Filter*> f;

	f.reserve(m_filters.size());

	for(auto i = m_filters.begin(); i != m_filters.end(); i++)
	{
		if(i->filter->GetMerit() >= MERIT64_DO_USE)
		{
			f.push_back(&*i);
		}
	}

	std::sort(f.begin(), f.end(), [] (const Filter* a, const Filter* b) -> bool
	{
		if(a->group < b->group) return true;
		if(a->group > b->group) return false;

		if(a->filter->GetCLSID() == b->filter->GetCLSID())
		{
			CFGFilterFile* af = dynamic_cast<CFGFilterFile*>(a->filter);
			CFGFilterFile* bf = dynamic_cast<CFGFilterFile*>(b->filter);

			if(af && !bf) return true;
			if(!af && bf) return false;
		}

		if(a->filter->GetMerit() > b->filter->GetMerit()) return true;
		if(a->filter->GetMerit() < b->filter->GetMerit()) return false;

		if(a->exactmatch && !b->exactmatch) return true;
		if(!a->exactmatch && b->exactmatch) return false;

		if(a->index < b->index) return true;
		if(a->index > b->index) return false;

		return false;
	});

	filters.clear();

	for(auto i = f.begin(); i != f.end(); i++)
	{
		filters.push_back((*i)->filter);
	}
}
