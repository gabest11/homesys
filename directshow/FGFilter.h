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
#include <dshow.h>

#define MERIT64(merit) (((UINT64)(merit)) << 16)
#define MERIT64_DO_NOT_USE MERIT64(MERIT_DO_NOT_USE)
#define MERIT64_DO_USE MERIT64(MERIT_DO_NOT_USE + 1)
#define MERIT64_UNLIKELY (MERIT64(MERIT_UNLIKELY))
#define MERIT64_NORMAL (MERIT64(MERIT_NORMAL))
#define MERIT64_PREFERRED (MERIT64(MERIT_PREFERRED))
#define MERIT64_ABOVE_DSHOW (MERIT64(1) << 32)

class CFGFilter
{
protected:
	CLSID m_clsid;
	std::wstring m_name;
	struct {union {UINT64 val; struct {UINT64 low:16, mid:32, high:16;};};} m_merit;
	std::list<GUID> m_types; // TODO: std::list<CMediaType> m_mts;

public:
	CFGFilter(const CLSID& clsid, LPCWSTR name = L"", UINT64 merit = MERIT64_DO_USE);
	virtual ~CFGFilter() {}

	CLSID GetCLSID() {return m_clsid;}
	std::wstring GetName() {return m_name;}
	UINT64 GetMerit() {return m_merit.val;}
	DWORD GetMeritForDirectShow() {return m_merit.mid;}

	const std::list<GUID>& GetTypes() const {return m_types;}
	void RemoveTypes();
	void SetTypes(const std::list<GUID>& types);
	void AddType(const GUID& majortype, const GUID& subtype);
	bool Match(const std::list<CMediaType>& mts, bool exact);

	std::list<std::wstring> m_mimes;
	std::list<std::wstring> m_protocols;
	std::list<std::wstring> m_extensions;
	std::list<std::wstring> m_chkbytes; 
	// TODO: subtype?

	virtual HRESULT Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks) = 0;
};

class CFGFilterRegistry : public CFGFilter
{
protected:
	std::wstring m_dispname;
	CComPtr<IMoniker> m_moniker;

	void ExtractFilterData(BYTE* p, UINT len);

public:
	CFGFilterRegistry(IMoniker* moniker, UINT64 merit = MERIT64_DO_USE);
	CFGFilterRegistry(LPCWSTR dispname, UINT64 merit = MERIT64_DO_USE);
	CFGFilterRegistry(const CLSID& clsid, UINT64 merit = MERIT64_DO_USE);

	std::wstring GetDisplayName() {return m_dispname;}
	IMoniker* GetMoniker() {return m_moniker;}

	HRESULT Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks);
};

template<class T>
class CFGFilterInternal : public CFGFilter
{
public:
	CFGFilterInternal(LPCWSTR name = L"", UINT64 merit = MERIT64_DO_USE) 
		: CFGFilter(__uuidof(T), name, merit)
	{
	}

	HRESULT Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks)
	{
		CheckPointer(ppBF, E_POINTER);

		HRESULT hr = S_OK;

		CComPtr<IBaseFilter> pBF = new T(NULL, &hr);

		if(FAILED(hr)) return hr;

		*ppBF = pBF.Detach();

		return hr;
	}
};

class CFGFilterFile : public CFGFilter
{
protected:
	std::wstring m_path;
	HINSTANCE m_hInst;

public:
	CFGFilterFile(const CLSID& clsid, LPCWSTR path, LPCWSTR name = L"", UINT64 merit = MERIT64_DO_USE);

	HRESULT Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks);
};

class CFGFilterVideoRenderer : public CFGFilter
{
protected:
	HWND m_hWnd;

public:
	CFGFilterVideoRenderer(HWND hWnd, const CLSID& clsid, LPCWSTR name = L"", UINT64 merit = MERIT64_DO_USE);

	HRESULT Create(IBaseFilter** ppBF, std::list<CAdapt<CComPtr<IUnknown>>>& unks);
};

class CFGFilterList
{
	struct Filter 
	{
		CFGFilter* filter; 
		int index; 
		int group; 
		bool exactmatch;
		bool autodelete;
	};

	std::list<Filter> m_filters;
	std::list<CFGFilter*> m_sortedfilters;

public:
	CFGFilterList();
	virtual ~CFGFilterList();

	bool IsEmpty() {return m_filters.empty();}
	int GetCount() {return m_filters.size();}
	void RemoveAll();

	void Insert(CFGFilter* filter, int group, bool exactmatch = false, bool autodelete = true);
	void GetFilters(std::list<CFGFilter*>& filters);
};
