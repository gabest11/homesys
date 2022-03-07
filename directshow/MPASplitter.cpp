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

#include "StdAfx.h"
#include "MPASplitter.h"
#include <initguid.h>
#include "moreuuids.h"

// CMPASplitterFilter

CMPASplitterFilter::CMPASplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMPASplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
{
}

CMPASplitterFilter::~CMPASplitterFilter()
{
	delete m_file;
}

STDMETHODIMP CMPASplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMPASplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	delete m_file;

	m_file = new CMpaSplitterFile(pAsyncReader, hr);

	if(FAILED(hr)) 
	{
		delete m_file; 
		
		m_file = NULL;

		return hr;
	}

	std::vector<CMediaType> mts;

	mts.push_back(m_file->GetMediaType());

	CBaseSplitterOutputPin* pin = new CBaseSplitterOutputPin(mts, L"Audio", this, &hr);

	AddOutputPin(0, pin);

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_file->GetDuration();

	std::wstring s, title;

	if(m_file->GetTag('TIT2', s)) title = s;
	if(m_file->GetTag('TYER', s) && !title.empty() && !s.empty()) title += L" (" + s + L")";
	if(!title.empty()) SetProperty(L"TITL", title.c_str());
	if(m_file->GetTag('TPE1', s)) SetProperty(L"AUTH", s.c_str());
	if(m_file->GetTag('TCOP', s)) SetProperty(L"CPYR", s.c_str());
	if(m_file->GetTag('COMM', s)) SetProperty(L"DESC", s.c_str());

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

STDMETHODIMP CMPASplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_file, VFW_E_NOT_CONNECTED);

	*pDuration = m_file->GetDuration();

	return S_OK;
}

bool CMPASplitterFilter::DemuxInit()
{
	if(!m_file) return false;

	// TODO

	return true ;
}

void CMPASplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	__int64 start = m_file->GetStartPos();
	__int64 end = m_file->GetEndPos();

	if(rt <= 0 || m_file->GetDuration() <= 0)
	{
		m_file->Seek(start);
	}
	else
	{
		m_file->Seek(start + (__int64)((1.0 * rt / m_file->GetDuration()) * (end - start)));
	}

	m_start = rt;
}

bool CMPASplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	int size;
	REFERENCE_TIME dur;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_file->GetPos() < m_file->GetEndPos() - 9)
	{
		if(!m_file->Sync(size, dur)) {Sleep(1); continue;}

		CPacket* p = new CPacket(size);

		m_file->ByteRead(&p->buff[0], size);

		p->id = 0;
		p->start = m_start;
		p->stop = m_start + dur;
		p->flags |= CPacket::SyncPoint | CPacket::TimeValid;

		hr = DeliverPacket(p);

		m_start += dur;
	}

	return true;
}

// CMPASourceFilter

CMPASourceFilter::CMPASourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMPASplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}
