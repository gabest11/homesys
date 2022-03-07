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
#include "SubPassThruFilter.h"

CSubPassThruFilter::CInputPin::CInputPin(CSubPassThruFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CSubInputPin(pFilter, pLock, phr)
{
}

void CSubPassThruFilter::CInputPin::AddSubStream(ISubStream* ss)
{
	if(m_ssold != NULL)
	{
		if(ss != NULL)
		{
			((CSubPassThruFilter*)m_pFilter)->m_cb->ReplaceSubtitle(m_ssold, ss);
		}

		m_ssold = NULL;
	}
}

void CSubPassThruFilter::CInputPin::RemoveSubStream(ISubStream* ss)
{
	m_ssold = ss;
}

void CSubPassThruFilter::CInputPin::InvalidateSubtitle(REFERENCE_TIME start, ISubStream* ss)
{
	((CSubPassThruFilter*)m_pFilter)->m_cb->InvalidateSubtitle(start, ss);
}

STDMETHODIMP CSubPassThruFilter::CInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	__super::NewSegment(tStart, tStop, dRate);

	return ((CSubPassThruFilter*)m_pFilter)->m_output->DeliverNewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CSubPassThruFilter::CInputPin::Receive(IMediaSample* pSample)
{
	__super::Receive(pSample);

	return ((CSubPassThruFilter*)m_pFilter)->m_output->Deliver(pSample);
}

STDMETHODIMP CSubPassThruFilter::CInputPin::EndOfStream()
{
	__super::EndOfStream();

	return ((CSubPassThruFilter*)m_pFilter)->m_output->DeliverEndOfStream();
}

STDMETHODIMP CSubPassThruFilter::CInputPin::BeginFlush()
{
	__super::BeginFlush();

	return ((CSubPassThruFilter*)m_pFilter)->m_output->DeliverBeginFlush();
}

STDMETHODIMP CSubPassThruFilter::CInputPin::EndFlush()
{
	__super::EndFlush();

	return ((CSubPassThruFilter*)m_pFilter)->m_output->DeliverEndFlush();
}

//

CSubPassThruFilter::COutputPin::COutputPin(CSubPassThruFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(NAME("CSubPassThruOutputPin"), pFilter, pLock, phr, L"Out")
{
}

HRESULT CSubPassThruFilter::COutputPin::CheckMediaType(const CMediaType* mtOut)
{
	CMediaType mt;

	return S_OK == ((CSubPassThruFilter*)m_pFilter)->m_input->ConnectionMediaType(&mt) && mt == *mtOut ? S_OK : E_FAIL;
}

HRESULT CSubPassThruFilter::COutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(!((CSubPassThruFilter*)m_pFilter)->m_input->IsConnected())
	{
		return E_UNEXPECTED;
	}

	CComPtr<IMemAllocator> allocator;

	((CSubPassThruFilter*)m_pFilter)->m_input->GetAllocator(&allocator);

	return allocator != NULL ? allocator->GetProperties(pProperties) : E_FAIL;
}

HRESULT CSubPassThruFilter::COutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(((CSubPassThruFilter*)m_pFilter)->m_input->IsConnected() == FALSE)
	{
		return E_UNEXPECTED;
	}

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	((CSubPassThruFilter*)m_pFilter)->m_input->ConnectionMediaType(pmt);

	return S_OK;
}

CSubPassThruFilter::CSubPassThruFilter(ICallback* cb) 
	: CBaseFilter(NAME("CSubPassThruFilter"), NULL, this, __uuidof(this))
	, m_cb(cb)
{
	HRESULT hr;

	m_input = new CInputPin(this, this, &hr);
	m_output = new COutputPin(this, this, &hr);
}

CSubPassThruFilter::~CSubPassThruFilter()
{
	delete m_input;
	delete m_output;
}

STDMETHODIMP CSubPassThruFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	if(m_input != NULL && riid == __uuidof(ISubStream))
	{
		CComPtr<ISubStream> ss = m_input->GetSubStream();
		
		if(ss != NULL)
		{
			*ppv = ss.Detach();

			return S_OK;
		}
	}

	return __super::NonDelegatingQueryInterface(riid, ppv);
}

int CSubPassThruFilter::GetPinCount()
{
	return 2;
}

CBasePin* CSubPassThruFilter::GetPin(int n)
{
	switch(n)
	{
	case 0: return m_input;
	case 1: return m_output;
	}
	
	return NULL;
}

/*
void CSubtitlePassThruFilter::SetName()
{
	CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pRTS;

	CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);

	if(CComQIPtr<IPropertyBag> pPB = m_pTPTInput->GetConnected())
	{
		CComVariant var;

		if(SUCCEEDED(pPB->Read(L"LANGUAGE", &var, NULL)))
		{
			pRTS->m_name = CString(var.bstrVal) + _T(" (embeded)");
		}
	}

	if(pRTS->m_name.IsEmpty())
	{
		CPinInfo pi;
		m_pTPTInput->GetConnected()->QueryPinInfo(&pi);
		pRTS->m_name = CString(CStringW(pi.achName)) + _T(" (embeded)");

	}
}
*/
