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
#include <atltime.h> // TODO
#include "DSMMuxer.h"
#include "DirectShow.h"
#include <initguid.h>
#include <qnetwork.h>
#include "moreuuids.h"

static int GetByteLength(UINT64 data, int min = 0)
{
	int i = 7;
	while(i >= min && ((BYTE*)&data)[i] == 0) i--;
	return ++i;
}

//

CDSMMuxerFilter::CDSMMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, bool fAutoChap, bool fAutoRes)
	: CBaseMuxerFilter(pUnk, phr, __uuidof(this))
	, m_fAutoChap(fAutoChap)
	, m_fAutoRes(fAutoRes)
	, m_key(NULL)
	, m_keypos(0)
	, m_keylen(0)
{
	if(phr) *phr = S_OK;
}

CDSMMuxerFilter::~CDSMMuxerFilter()
{
	delete m_key;
}

STDMETHODIMP CDSMMuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		QI(IDSMMuxerFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CDSMMuxerFilter::MuxPacketHeader(IBitStream* pBS, dsmp_t type, UINT64 len)
{
	ASSERT(type < 32);

	int i = GetByteLength(len, 1);

	pBS->BitWrite(DSMSW, DSMSW_SIZE<<3);
	pBS->BitWrite(type, 5);
	pBS->BitWrite(i-1, 3);
	pBS->BitWrite(len, i<<3);
}

void CDSMMuxerFilter::MuxFileInfo(IBitStream* pBS)
{
	int len = 1;

	std::map<std::string, std::string> si;

	for(auto i = m_properties.begin(); i != m_properties.end(); i++)
	{
		std::string k = Util::UTF16To8(i->first.c_str());
		std::string v = Util::UTF16To8(i->second.c_str());
		
		if(k.size() == 4)
		{
			si[k] = v;
			
			len += 4 + v.size() + 1;
		}
	}

	MuxPacketHeader(pBS, DSMP_FILEINFO, len);

	pBS->BitWrite(DSMF_VERSION, 8);

	for(auto i = si.begin(); i != si.end(); i++)
	{
		pBS->ByteWrite(i->first.c_str(), 4);
		pBS->ByteWrite(i->second.c_str(), i->second.size() + 1);
	}
}

void CDSMMuxerFilter::MuxStreamInfo(IBitStream* pBS, CBaseMuxerInputPin* pin)
{
	int len = 1;

	std::map<std::string, std::string> si;

	for(auto i = pin->m_properties.begin(); i != pin->m_properties.end(); i++)
	{
		std::string k = Util::UTF16To8(i->first.c_str());
		std::string v = Util::UTF16To8(i->second.c_str());
		
		if(k.size() == 4)
		{
			si[k] = v;
			
			len += 4 + v.size() + 1;
		}
	}

	if(len > 1)
	{
		MuxPacketHeader(pBS, DSMP_STREAMINFO, len);

		pBS->BitWrite(pin->GetID(), 8);

		for(auto i = si.begin(); i != si.end(); i++)
		{
			pBS->ByteWrite(i->first.c_str(), 4);
			pBS->ByteWrite(i->second.c_str(), i->second.size() + 1);
		}
	}
}

void CDSMMuxerFilter::MuxInit()
{
	m_sps.clear();
	m_isps.clear();
	m_rtPrevSyncPoint = _I64_MIN;
}

void CDSMMuxerFilter::MuxHeader(IBitStream* pBS)
{
	std::string muxer = Util::Format("DSM Muxer (%s)", __TIMESTAMP__);
	std::wstring now = CTime::GetCurrentTime().FormatGmt(L"%Y-%m-%d %H:%M:%S");

	SetProperty(L"MUXR", std::wstring(muxer.begin(), muxer.end()).c_str());
	SetProperty(L"DATE", now.c_str());

	MuxFileInfo(pBS);

	if(m_key)
	{
		MuxPacketHeader(pBS, DSMP_KEY, 1 + 4 + m_key->len);
		m_keypos = pBS->GetPos();
		m_keylen = m_key->len;
		pBS->BitWrite(m_key->type, 8);
		pBS->BitWrite(m_key->id, 32);
		pBS->ByteWrite(m_key->data, m_key->len);
	}

	for(auto i = m_pins.begin(); i != m_pins.end(); i++)
	{
		CBaseMuxerInputPin* pin = *i;

		const CMediaType& mt = pin->CurrentMediaType();

		ASSERT((mt.lSampleSize >> 30) == 0); // you don't need >1GB samples, do you?

		MuxPacketHeader(pBS, DSMP_MEDIATYPE, 5 + sizeof(GUID) * 3 + mt.FormatLength());
		pBS->BitWrite(pin->GetID(), 8);
		pBS->ByteWrite(&mt.majortype, sizeof(mt.majortype));
		pBS->ByteWrite(&mt.subtype, sizeof(mt.subtype));
		pBS->BitWrite(mt.bFixedSizeSamples, 1);
		pBS->BitWrite(mt.bTemporalCompression, 1);
		pBS->BitWrite(mt.lSampleSize, 30);
		pBS->ByteWrite(&mt.formattype, sizeof(mt.formattype));
		pBS->ByteWrite(mt.Format(), mt.FormatLength());
		
		MuxStreamInfo(pBS, pin);
	}

	// resources & chapters

	std::list<IDSMResourceBag*> pRBs;

	pRBs.push_back(this);

	CComQIPtr<IDSMChapterBag> pCB = (IUnknown*)(INonDelegatingUnknown*)this;
	
	for(auto i = m_pins.begin(); i != m_pins.end(); i++)
	{
		CComPtr<IPin> pin = (*i)->GetConnected();

		while(pin != NULL)
		{
			if(m_fAutoRes)
			{
				CComQIPtr<IDSMResourceBag> pPB = DirectShow::GetFilter(pin);

				if(pPB != NULL && std::find(pRBs.begin(), pRBs.end(), pPB) == pRBs.end())
				{
					pRBs.push_back(pPB);
				}
			}

			if(m_fAutoChap)
			{
				if(pCB == NULL || pCB->ChapGetCount() == 0) 
				{
					pCB = DirectShow::GetFilter(pin);
				}
			}

			pin = DirectShow::GetUpStreamPin(DirectShow::GetFilter(pin));
		}
	}

	// resources

	for(auto i = pRBs.begin(); i != pRBs.end(); i++)
	{
		IDSMResourceBag* pRB = *i;

		for(DWORD i = 0, j = pRB->ResGetCount(); i < j; i++)
		{
			CComBSTR name, desc, mime;
			BYTE* pData = NULL;
			DWORD len = 0;

			if(SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, NULL)))
			{
				std::string utf8_name = Util::UTF16To8(name);
				std::string utf8_desc = Util::UTF16To8(desc);
				std::string utf8_mime = Util::UTF16To8(mime);

				MuxPacketHeader(pBS, DSMP_RESOURCE, 
					1 + 
					utf8_name.size() + 1 + 
					utf8_desc.size() + 1 + 
					utf8_mime.size() + 1 + 
					len);

				pBS->BitWrite(0, 2);
				pBS->BitWrite(0, 6); // reserved
				pBS->ByteWrite(utf8_name.c_str(), utf8_name.size() + 1);
				pBS->ByteWrite(utf8_desc.c_str(), utf8_desc.size() + 1);
				pBS->ByteWrite(utf8_mime.c_str(), utf8_mime.size() + 1);
				pBS->ByteWrite(pData, len);

				CoTaskMemFree(pData);
			}
		}
	}

	// chapters

	if(pCB != NULL)
	{
		std::list<CDSMChapter> chapters;

		REFERENCE_TIME rtPrev = 0;
		int len = 0;

		pCB->ChapSort();

		for(DWORD i = 0; i < pCB->ChapGetCount(); i++)
		{
			CDSMChapter c;
			CComBSTR name;

			if(SUCCEEDED(pCB->ChapGet(i, &c.m_rt, &name)))
			{
				REFERENCE_TIME rtDiff = c.m_rt - rtPrev; rtPrev = c.m_rt; c.m_rt = rtDiff;
				c.m_name = name;
				len += 1 + GetByteLength(std::abs(c.m_rt)) + Util::UTF16To8(c.m_name.c_str()).size() + 1;
				chapters.push_back(c);
			}
		}

		if(!chapters.empty())
		{
			MuxPacketHeader(pBS, DSMP_CHAPTERS, len);

			for(auto i = chapters.begin(); i != chapters.end(); i++)
			{
				const CDSMChapter& c = *i;
				std::string name = Util::UTF16To8(c.m_name.c_str());
				int irt = GetByteLength(std::abs(c.m_rt));
				pBS->BitWrite(c.m_rt < 0, 1);
				pBS->BitWrite(irt, 3);
				pBS->BitWrite(0, 4);
				pBS->BitWrite(std::abs(c.m_rt), irt << 3);
				pBS->ByteWrite(name.c_str(), name.size() + 1);
			}
		}
	}
}

void CDSMMuxerFilter::MuxPacket(IBitStream* pBS, const CMuxerPacket* pPacket)
{
	if(pPacket->IsEOS())
	{
		return;
	}

	if(pPacket->pin->CurrentMediaType().majortype == MEDIATYPE_Text)
	{
		std::string s((char*)pPacket->buff.data(), pPacket->buff.size());
		
		Util::Replace(s, "\xff", " ");
		Util::Replace(s, "&nbsp;", " ");
		Util::Replace(s, "&nbsp", " ");
		
		s = Util::Trim(s);

		if(s.empty())
		{
			return;
		}
	}

	ASSERT(!pPacket->IsSyncPoint() || pPacket->IsTimeValid());

	REFERENCE_TIME rtTimeStamp = _I64_MIN, rtDuration = 0;
	int iTimeStamp = 0, iDuration = 0;

	if(pPacket->IsTimeValid())
	{
		rtTimeStamp = pPacket->start;
		rtDuration = std::max<REFERENCE_TIME>(pPacket->stop - pPacket->start, 0);

		iTimeStamp = GetByteLength(std::abs(rtTimeStamp));

		ASSERT(iTimeStamp <= 7);

		iDuration = GetByteLength(rtDuration);

		ASSERT(iDuration <= 7);

		IndexSyncPoint(pPacket, pBS->GetPos());
	}

	int len = 2 + iTimeStamp + iDuration + pPacket->buff.size(); // id + flags + data 

	MuxPacketHeader(pBS, DSMP_SAMPLE, len);
	pBS->BitWrite(pPacket->pin->GetID(), 8);
	pBS->BitWrite(pPacket->IsSyncPoint(), 1);
	pBS->BitWrite(rtTimeStamp < 0, 1);
	pBS->BitWrite(iTimeStamp, 3);
	pBS->BitWrite(iDuration, 3);
	pBS->BitWrite(std::abs(rtTimeStamp), iTimeStamp << 3);
	pBS->BitWrite(rtDuration, iDuration << 3);

	size_t count = pPacket->buff.size();

	if(m_data.size() < count)
	{
		m_data.resize(count);
	}

	BYTE* data = m_data.data();

	memcpy(data, (BYTE*)pPacket->buff.data(), count);

	if(m_key)
	{
		m_key->Encode(data, count);
	}

	pBS->ByteWrite(data, count);
}

void CDSMMuxerFilter::MuxFooter(IBitStream* pBS)
{
	// syncpoints

	int len = 0;
	std::list<IndexedSyncPoint> isps;
	REFERENCE_TIME rtPrev = 0, rt;
	UINT64 fpPrev = 0, fp;

	for(auto i = m_isps.begin(); i != m_isps.end(); i++)
	{
		const IndexedSyncPoint& isp = *i;

		// printf("sp[%d]: %I64d %I64x\n", isp.id, isp.rt, isp.fp);

		rt = isp.rt - rtPrev; rtPrev = isp.rt;
		fp = isp.fp - fpPrev; fpPrev = isp.fp;

		IndexedSyncPoint isp2;

		isp2.fp = fp;
		isp2.rt = rt;

		isps.push_back(isp2);

		len += 1 + GetByteLength(std::abs(rt)) + GetByteLength(fp); // flags + rt + fp
	}

	MuxPacketHeader(pBS, DSMP_SYNCPOINTS, len);

	for(auto i = m_isps.begin(); i != m_isps.end(); i++)
	{
		const IndexedSyncPoint& isp = *i;

		int irt = GetByteLength(std::abs(isp.rt));
		int ifp = GetByteLength(isp.fp);

		pBS->BitWrite(isp.rt < 0, 1);
		pBS->BitWrite(irt, 3);
		pBS->BitWrite(ifp, 3);
		pBS->BitWrite(0, 1); // reserved
		pBS->BitWrite(std::abs(isp.rt), irt << 3);
		pBS->BitWrite(isp.fp, ifp << 3);
	}
}

void CDSMMuxerFilter::IndexSyncPoint(const CMuxerPacket* p, __int64 fp)
{
	// Yes, this is as complicated as it looks.
	// Rule #1: don't write this packet if you can't do it reliably. 
	// (think about overlapped subtitles, line1: 0->10, line2: 1->9)

	// FIXME: the very last syncpoints won't get moved to m_isps because there are no more syncpoints to trigger it!

	if(fp < 0 || p == NULL || !p->IsTimeValid() || !p->IsSyncPoint()) 
	{
		return;
	}

	ASSERT(p->start >= m_rtPrevSyncPoint);

	m_rtPrevSyncPoint = p->start;

	SyncPoint sp;

	sp.id = p->pin->GetID();
	sp.rtStart = p->start;
	sp.rtStop = p->pin->IsSubtitleStream() ? p->stop : _I64_MAX;
	sp.fp = fp;

	{
		SyncPoint& head = !m_sps.empty() ? m_sps.front() : sp;
		SyncPoint& tail = !m_sps.empty() ? m_sps.back() : sp;

		REFERENCE_TIME rtfp = !m_isps.empty() ? m_isps.back().rtfp : _I64_MIN;

		if(head.rtStart > rtfp + 1000000) // 100ms limit, just in case every stream had only keyframes, then sycnpoints would be too frequent
		{
			IndexedSyncPoint isp;

			isp.id = head.id;
			isp.rt = tail.rtStart;
			isp.rtfp = head.rtStart;
			isp.fp = head.fp;

			m_isps.push_back(isp);
		}
	}

	for(auto i = m_sps.begin(); i != m_sps.end(); )
	{
		auto j = i++;

		SyncPoint& sp2 = *j;

		if(sp2.id == sp.id && sp2.rtStop <= sp.rtStop || sp2.rtStop <= sp.rtStart)
		{
			m_sps.erase(j);
		}
	}

	m_sps.push_back(sp);
}

// IDSMMuxerFilter

STDMETHODIMP CDSMMuxerFilter::SetKey(const DSMKey* key)
{
	delete m_key;

	if(key)
	{
		m_key = new DSMKey();
		m_key->type = key->type;
		m_key->id = key->id;
		m_key->len = key->len;
		m_key->data = new BYTE[key->len];
		memcpy(m_key->data, key->data, key->len);
	}

	return S_OK;
}

STDMETHODIMP CDSMMuxerFilter::GetKeyPos(__int64* pos, int* len)
{
	*pos = m_keypos;
	*len = m_keylen;

	return S_OK;
}