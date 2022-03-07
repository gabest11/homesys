#include "StdAfx.h"
#include "DSMPropertyBag.h"
#include "DirectShow.h"

// IDSMPropertyBagImpl

IDSMPropertyBagImpl::IDSMPropertyBagImpl()
{
}

IDSMPropertyBagImpl::~IDSMPropertyBagImpl()
{
}

// IPropertyBag

STDMETHODIMP IDSMPropertyBagImpl::Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog)
{
	CheckPointer(pVar, E_POINTER);

	if(pVar->vt != VT_EMPTY)
	{
		return E_INVALIDARG;
	}

	auto pair = m_properties.find(pszPropName);

	if(pair == m_properties.end())
	{
		return E_FAIL;
	}

	CComVariant(pair->second.c_str()).Detach(pVar);

	return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::Write(LPCOLESTR pszPropName, VARIANT* pVar)
{
	return SetProperty(pszPropName, pVar);
}

// IPropertyBag2

STDMETHODIMP IDSMPropertyBagImpl::Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pvarValue, E_POINTER);
	CheckPointer(phrError, E_POINTER);

	for(ULONG i = 0; i < cProperties; phrError[i] = S_OK, i++)
	{
		std::wstring s;

		auto pair = m_properties.find(pPropBag[i].pstrName);

		if(pair != m_properties.end())
		{
			s = pair->second;
		}

		CComVariant(s.c_str()).Detach(pvarValue);
	}

	return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pvarValue, E_POINTER);

	for(ULONG i = 0; i < cProperties; i++)
	{
		SetProperty(pPropBag[i].pstrName, &pvarValue[i]);
	}

	return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::CountProperties(ULONG* pcProperties)
{
	CheckPointer(pcProperties, E_POINTER);

	*pcProperties = m_properties.size();

	return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pcProperties, E_POINTER);

	int j = 0;

	for(auto pair = m_properties.begin(); pair != m_properties.end(); pair++, j++)
	{
		if(j >= iProperty)
		{
			int i = j - iProperty;

			if(i >= cProperties)
			{
				break;
			}

			std::wstring s = pair->first;

			pPropBag[i].pstrName = (BSTR)CoTaskMemAlloc((s.size() + 1) * sizeof(WCHAR));

			if(!pPropBag[i].pstrName) return E_FAIL;

			wcscpy(pPropBag[i].pstrName, s.c_str());
		}
	}

	return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog)
{
	return E_NOTIMPL;
}

// IDSMProperyBag

HRESULT IDSMPropertyBagImpl::SetProperty(LPCWSTR key, LPCWSTR value)
{
	CheckPointer(key, E_POINTER);
	CheckPointer(value, E_POINTER);

	m_properties[key] = value;

	return S_OK;
}

HRESULT IDSMPropertyBagImpl::SetProperty(LPCWSTR key, VARIANT* var)
{
	CheckPointer(key, E_POINTER);
	CheckPointer(var, E_POINTER);

	if((var->vt & (VT_BSTR | VT_BYREF)) != VT_BSTR) 
	{
		return E_INVALIDARG;
	}

	return SetProperty(key, var->bstrVal);
}

HRESULT IDSMPropertyBagImpl::GetProperty(LPCWSTR key, BSTR* value)
{
	CheckPointer(key, E_POINTER);
	CheckPointer(value, E_POINTER);

	auto pair = m_properties.find(key);

	if(pair == m_properties.end())
	{
		return E_FAIL;
	}

	*value = SysAllocString(pair->second.c_str());

	return S_OK;
}

HRESULT IDSMPropertyBagImpl::RemoveAll()
{
	m_properties.clear();

	return S_OK;
}

HRESULT IDSMPropertyBagImpl::RemoveAt(LPCWSTR key)
{
	return m_properties.erase(key) != 0 ? S_OK : S_FALSE;
}

// CDSMResource

CCritSec CDSMResource::m_csResources;
std::map<DWORD_PTR, CDSMResource*> CDSMResource::m_resources;

CDSMResource::CDSMResource() 
	: m_mime(L"application/octet-stream")
	, m_tag(0)
{
	CAutoLock cAutoLock(&m_csResources);

	m_resources[(DWORD_PTR)this] = this;
}

CDSMResource::CDSMResource(const CDSMResource& r)
{
	*this = r;

	CAutoLock cAutoLock(&m_csResources);

	m_resources[(DWORD_PTR)this] = this;
}

CDSMResource::CDSMResource(LPCWSTR name, LPCWSTR desc, LPCWSTR mime, BYTE* data, int len, DWORD_PTR tag)
{
	m_name = name;
	m_desc = desc;
	m_mime = mime;
	m_data.resize(len);
	memcpy(&m_data[0], data, len);
	m_tag = tag;

	CAutoLock cAutoLock(&m_csResources);

	m_resources[(DWORD_PTR)this] = this;
}

CDSMResource::~CDSMResource()
{
	CAutoLock cAutoLock(&m_csResources);

	m_resources.erase((DWORD_PTR)this);
}

// IDSMResourceBagImpl

IDSMResourceBagImpl::IDSMResourceBagImpl()
{
}

// IDSMResourceBag

STDMETHODIMP_(DWORD) IDSMResourceBagImpl::ResGetCount()
{
	return m_resources.size();
}

STDMETHODIMP IDSMResourceBagImpl::ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag)
{
	if(ppData) CheckPointer(pDataLen, E_POINTER);

	if(iIndex >= m_resources.size())
	{
		return E_INVALIDARG;
	}

	CDSMResource& r = m_resources[iIndex];

	if(ppName) *ppName = SysAllocString(r.m_name.c_str());
	if(ppDesc) *ppDesc = SysAllocString(r.m_desc.c_str());
	if(ppMime) *ppMime = SysAllocString(r.m_mime.c_str());
	if(ppData) {*pDataLen = r.m_data.size(); memcpy(*ppData = (BYTE*)CoTaskMemAlloc(*pDataLen), &r.m_data[0], *pDataLen);}
	if(pTag) *pTag = r.m_tag;

	return S_OK;
}

STDMETHODIMP IDSMResourceBagImpl::ResSet(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag)
{
	if(iIndex >= m_resources.size())
	{
		return E_INVALIDARG;
	}

	CDSMResource& r = m_resources[iIndex];

	if(pName) r.m_name = pName;
	if(pDesc) r.m_desc = pDesc;
	if(pMime) r.m_mime = pMime;
	if(pData || len == 0) {r.m_data.resize(len); if(pData) memcpy(&r.m_data[0], pData, len);}
	r.m_tag = tag;

	return S_OK;
}

STDMETHODIMP IDSMResourceBagImpl::ResAppend(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag)
{
	int index = m_resources.size();

	m_resources.push_back(CDSMResource());

	return ResSet(index, pName, pDesc, pMime, pData, len, tag);
}

STDMETHODIMP IDSMResourceBagImpl::ResRemoveAt(DWORD iIndex)
{
	if((INT_PTR)iIndex >= m_resources.size())
	{
		return E_INVALIDARG;
	}

	m_resources.erase(m_resources.begin() + iIndex);

	return S_OK;
}

STDMETHODIMP IDSMResourceBagImpl::ResRemoveAll(DWORD_PTR tag)
{
	if(tag)
	{
		for(auto i = m_resources.begin(); i != m_resources.end(); )
		{
			auto j = i++;

			if(j->m_tag == tag)
			{
				m_resources.erase(j);
			}
		}
	}
	else
	{
		m_resources.clear();
	}

	return S_OK;
}

// CDSMChapter

int CDSMChapter::m_counter = 0;

CDSMChapter::CDSMChapter()
{
	m_order = m_counter++;

	m_rt = 0;
}

CDSMChapter::CDSMChapter(REFERENCE_TIME rt, LPCWSTR name)
{
	m_order = m_counter++;

	m_rt = rt;
	m_name = name;
}

void CDSMChapter::operator = (const CDSMChapter& c)
{
	m_order = c.m_counter;
	m_rt = c.m_rt;
	m_name = c.m_name;
}

// IDSMChapterBagImpl

IDSMChapterBagImpl::IDSMChapterBagImpl()
{
	m_sorted = false;
}

// IDSMRChapterBag

STDMETHODIMP_(DWORD) IDSMChapterBagImpl::ChapGetCount()
{
	return m_chapters.size();
}

STDMETHODIMP IDSMChapterBagImpl::ChapGet(DWORD iIndex, REFERENCE_TIME* prt, BSTR* ppName)
{
	if(iIndex >= m_chapters.size())
	{
		return E_INVALIDARG;
	}

	CDSMChapter& c = m_chapters[iIndex];

	if(prt) *prt = c.m_rt;
	if(ppName) *ppName = SysAllocString(c.m_name.c_str());

	return S_OK;
}

STDMETHODIMP IDSMChapterBagImpl::ChapSet(DWORD iIndex, REFERENCE_TIME rt, LPCWSTR pName)
{
	if(iIndex >= m_chapters.size())
	{
		return E_INVALIDARG;
	}

	CDSMChapter& c = m_chapters[iIndex];

	c.m_rt = rt;
	if(pName) c.m_name = pName;

	m_sorted = false;

	return S_OK;
}

STDMETHODIMP IDSMChapterBagImpl::ChapAppend(REFERENCE_TIME rt, LPCWSTR pName)
{
	int index = m_chapters.size();

	m_chapters.push_back(CDSMChapter());

	return ChapSet(index, rt, pName);
}

STDMETHODIMP IDSMChapterBagImpl::ChapRemoveAt(DWORD iIndex)
{
	if(iIndex >= m_chapters.size())
	{
		return E_INVALIDARG;
	}

	m_chapters.erase(m_chapters.begin() + iIndex);

	return S_OK;
}

STDMETHODIMP IDSMChapterBagImpl::ChapRemoveAll()
{
	m_chapters.clear();

	m_sorted = false;

	return S_OK;
}

STDMETHODIMP_(long) IDSMChapterBagImpl::ChapLookup(REFERENCE_TIME* prt, BSTR* ppName)
{
	CheckPointer(prt, -1);

	ChapSort();

	int i = range_bsearch(m_chapters, *prt);

	if(i >= 0)
	{
		*prt = m_chapters[i].m_rt;

		if(ppName) *ppName = SysAllocString(m_chapters[i].m_name.c_str());
	}

	return i;
}

STDMETHODIMP IDSMChapterBagImpl::ChapSort()
{
	if(!m_sorted)
	{
		std::sort(m_chapters.begin(), m_chapters.end(), [] (const CDSMChapter& a, const CDSMChapter& b) -> bool
		{
			if(a.m_rt < b.m_rt) return true;
			else if(a.m_rt > b.m_rt) return false;
			else return a.m_order < b.m_order;
		});
		
		m_sorted = true;
	}

	return S_OK;
}

//
// CDSMChapterBag
//

CDSMChapterBag::CDSMChapterBag(LPUNKNOWN pUnk, HRESULT* phr) 
	: CUnknown(_T("CDSMChapterBag"), NULL)
{
}

STDMETHODIMP CDSMChapterBag::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return
		QI(IDSMChapterBag)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}
