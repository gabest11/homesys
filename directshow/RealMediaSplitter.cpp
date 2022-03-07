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
#include "RealMediaSplitter.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include "BitBlt.h"
#include <initguid.h>
#include "moreuuids.h"

using namespace RMFF;

// CRealMediaSplitterFilter

CRealMediaSplitterFilter::CRealMediaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CRealMediaSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
{
}

CRealMediaSplitterFilter::~CRealMediaSplitterFilter()
{
	delete m_file;
}

HRESULT CRealMediaSplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	delete m_file;

	m_file = new CRMFile(pAsyncReader, hr);

	if(FAILED(hr)) 
	{
		delete m_file;
		
		m_file = NULL;
		
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = 0;

	m_rtStop = 10000i64 * m_file->m_p.tDuration;

	for(auto i = m_file->m_mps.begin(); i != m_file->m_mps.end(); i++)
	{
		MediaProperies* pmp = *i;

		std::wstring name = Util::Format(L"Output %02d", pmp->stream);

		if(!pmp->name.empty()) 
		{
			name += L" (" + std::wstring(pmp->name.begin(), pmp->name.end()) + L")";
		}

		std::vector<CMediaType> mts;

		CMediaType mt;

		mt.lSampleSize = std::max<DWORD>(pmp->maxPacketSize * 16, 1);

		if(pmp->mime == L"video/x-pn-realvideo")
		{
			RealVideoDesc rv(pmp->buff.data());

			ASSERT(rv.dwSize >= FIELD_OFFSET(RealVideoDesc, morewh));
			ASSERT(rv.fcc1 == 'VIDO');

			DWORD fcc = (rv.fcc2 >> 24) | (rv.fcc2 << 24) | ((rv.fcc2 & 0xff0000) >> 8) | ((rv.fcc2 & 0xff00) << 8);

			mt.majortype = MEDIATYPE_Video;
			mt.subtype = FOURCCMap(fcc);
			mt.formattype = FORMAT_VideoInfo;

			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pmp->buff.size());

			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(vih + 1, pmp->buff.data(), pmp->buff.size());

			vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
			vih->bmiHeader.biWidth = rv.w;
			vih->bmiHeader.biHeight = rv.h;
			vih->bmiHeader.biPlanes = 3;
			vih->bmiHeader.biBitCount = rv.bpp;
			vih->bmiHeader.biCompression = fcc;
			vih->bmiHeader.biSizeImage = rv.w * rv.h * 3 / 2;

			vih->dwBitRate = pmp->avgBitRate; 

			if(rv.fps > 0x10000) 
			{
				vih->AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 / ((float)rv.fps / 0x10000)); 
			}

			mts.push_back(mt);

			if(pmp->width > 0 && pmp->height > 0)
			{
				BITMAPINFOHEADER bmi = vih->bmiHeader;

				mt.formattype = FORMAT_VideoInfo2;

				VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2) + pmp->buff.size());

				memset(mt.Format() + FIELD_OFFSET(VIDEOINFOHEADER2, dwInterlaceFlags), 0, mt.FormatLength() - FIELD_OFFSET(VIDEOINFOHEADER2, dwInterlaceFlags));
				memcpy(vih2 + 1, pmp->buff.data(), pmp->buff.size());

				vih2->bmiHeader = bmi;
				vih2->bmiHeader.biWidth = (DWORD)pmp->width;
				vih2->bmiHeader.biHeight = (DWORD)pmp->height;
				vih2->dwPictAspectRatioX = rv.w;
				vih2->dwPictAspectRatioY = rv.h;

				mts.insert(mts.begin(), mt);
			}
		}
		else if(pmp->mime == L"audio/x-pn-realaudio")
		{
			mt.majortype = MEDIATYPE_Audio;
			mt.formattype = FORMAT_WaveFormatEx;

			mt.bTemporalCompression = 1;

			WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmp->buff.size());

			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(wfe + 1, pmp->buff.data(), pmp->buff.size());

			const BYTE* fmt = pmp->buff.data();

			for(int i = 0; i < pmp->buff.size() - 4; i++, fmt++)
			{
				if(fmt[0] == '.' || fmt[1] == 'r' || fmt[2] == 'a')
				{
					break;
				}
			}

			RealAudioDesc ra(fmt);

			wfe->nBlockAlign = ra.frame_size;

			DWORD fcc = 0;

			const BYTE* extra = NULL;

			if(ra.version2 == 4)
			{
				RealAudioDesc4 ra4(fmt + sizeof(RealAudioDesc));

				wfe->nChannels = ra4.channels;
				wfe->wBitsPerSample = ra4.sample_size;
				wfe->nSamplesPerSec = ra4.sample_rate;

				const BYTE* p = fmt + sizeof(RealAudioDesc) + sizeof(RealAudioDesc4);

				p += *p++;

				int len = *p++;

				if(len == 4) 
				{
					fcc = MAKEFOURCC(p[0], p[1], p[2], p[3]);
				}
				else
				{
					ASSERT(0);
				}

				extra = p + len + 3;
			}
			else if(ra.version2 == 5)
			{
				RealAudioDesc5 ra5(fmt + sizeof(RealAudioDesc));

				wfe->nChannels = ra5.channels;
				wfe->wBitsPerSample = ra5.sample_size;
				wfe->nSamplesPerSec = ra5.sample_rate;

				fcc = ra5.fourcc3;

				extra = fmt + sizeof(RealAudioDesc) + sizeof(RealAudioDesc5) + 4;
			}
			else
			{
				continue;
			}

			DWORD rfcc = 0;

			for(int i = 0; i < 4; i++)
			{
				char* c = (char*)&fcc;

				c[i] = toupper(c[i]);

				rfcc = (rfcc << 8) | c[i];
			}

			mt.subtype = FOURCCMap(rfcc);

			switch(fcc)
			{
			case '14_4': wfe->wFormatTag = WAVE_FORMAT_14_4; break;
			case '28_8': wfe->wFormatTag = WAVE_FORMAT_28_8; break;
			case 'ATRC': wfe->wFormatTag = WAVE_FORMAT_ATRC; break;
			case 'COOK': wfe->wFormatTag = WAVE_FORMAT_COOK; break;
			case 'DNET': wfe->wFormatTag = WAVE_FORMAT_DNET; break;
			case 'SIPR': wfe->wFormatTag = WAVE_FORMAT_SIPR; break;
			case 'RAAC': wfe->wFormatTag = WAVE_FORMAT_RAAC; break;
			case 'RACP': wfe->wFormatTag = WAVE_FORMAT_RACP; break;
			}

			if(wfe->wFormatTag)
			{
				mts.push_back(mt);

				if(fcc == 'DNET')
				{
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3);

					mts.insert(mts.begin(), mt);
				}
				else if(fcc == 'RAAC' || fcc == 'RACP')
				{
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_AAC);

					DWORD len = ((extra[0] << 24) | (extra[1] << 16) | (extra[2] << 8) | extra[3]) - 1;

					ASSERT(extra[4] == 2); // always 2? why? what does it mean?

					if(extra[4] == 2)
					{
						WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + len);
						
						wfe->cbSize = (WORD)len;

						memcpy(wfe + 1, &extra[5], len);
					}
					else
					{
						WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 5);

						wfe->cbSize = DirectShow::AACInitData((BYTE*)(wfe + 1), 0, wfe->nSamplesPerSec, wfe->nChannels);
					}

					mts.insert(mts.begin(), mt);
				}
			}
		}
		else if(pmp->mime == L"logical-fileinfo")
		{
			std::map<std::wstring, std::wstring> lfi;

			CBaseSplitterFile f(pmp->buff.data(), pmp->buff.size());

			f.Seek(8);

			DWORD n = (DWORD)f.BitRead(32);

			if(n > 0xffff) // different format?
			{
				f.BitRead(16); // ?

				n = (DWORD)f.BitRead(32);
			}

			while(n-- > 0 && f.GetRemaining() > 4)
			{
				__int64 next = f.GetPos() + f.BitRead(32);

				std::wstring key, value;

				f.BitRead(8); // ?
				f.ReadAString(key, (int)f.BitRead(16));
				f.BitRead(32); // ?
				f.ReadAString(value, (int)f.BitRead(16));

				lfi[key] = value;

				ASSERT(f.GetPos() == next);

				f.Seek(next);
			}

			for(auto i = lfi.begin(); i != lfi.end(); i++)
			{
				if(i->first.find(L"CHAPTER") != 0 || i->first.find(L"TIME") != i->first.size() - 4)
				{
					continue;
				}

				int n = _wtoi(i->first.c_str() + 7);

				if(n > 0)
				{
					int h, m, s, ms;
					wchar_t c;

					if(7 == swscanf(i->second.c_str(), L"%d%c%d%c%d%c%d", &h, &c, &m, &c, &s, &c, &ms))
					{
						std::wstring name;

						auto j = lfi.find(Util::Format(L"CHAPTER%02dNAME", n));

						if(j != lfi.end())
						{
							name = j->second;
						}
						
						if(j->second.empty())
						{
							name = Util::Format(L"Chapter %d", n);
						}

						REFERENCE_TIME t = ((((REFERENCE_TIME)h * 60 + m) * 60 + s) * 1000 + ms) * 10000;

						ChapAppend(t, name.c_str());
					}
				}
			}
		}

		if(!mts.empty())
		{
			HRESULT hr;

			AddOutputPin((DWORD)pmp->stream, new CRealMediaOutputPin(mts, name.c_str(), this, &hr));

			if(m_rtStop == 0)
			{
				m_file->m_p.tDuration = std::max<DWORD>(m_file->m_p.tDuration, pmp->tDuration);
			}
		}
	}

	DWORD stream = 0;

	for(auto i = m_file->m_subs.begin(); i != m_file->m_subs.end(); i++)
	{
		CRMFile::Subtitle& s = *i;

		std::wstring name = Util::Format(L"Subtitle %02d", stream++);

		if(!s.name.empty()) 
		{
			name += L" (" + s.name + L")";
		}

		std::vector<CMediaType> mts;

		CMediaType mt;

		mt.majortype = MEDIATYPE_Text;
		mt.lSampleSize = 1;

		mts.push_back(mt);

		HRESULT hr;

		AddOutputPin((DWORD)~stream, new CRealMediaOutputPin(mts, name.c_str(), this, &hr));
	}

	m_rtDuration = m_rtNewStop = m_rtStop = 10000i64 * m_file->m_p.tDuration;

	SetProperty(L"TITL", m_file->m_cd.title.c_str());
	SetProperty(L"AUTH", m_file->m_cd.author.c_str());
	SetProperty(L"CPYR", m_file->m_cd.copyright.c_str());
	SetProperty(L"DESC", m_file->m_cd.comment.c_str());

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool CRealMediaSplitterFilter::DemuxInit()
{
	if(m_file->m_irs.empty())
	{
		// re-index

		m_nOpenProgress = 0;
		m_rtDuration = 0;

		int stream = m_file->GetMasterStream();

		DWORD tLastStart = -1;
		DWORD nPacket = 0;

		for(auto i = m_file->m_dcs.begin(); i != m_file->m_dcs.end() && !m_abort; i++)
		{
			DataChunk* pdc = *i;

			m_file->Seek(pdc->pos);

			for(DWORD i = 0; i < pdc->nPackets && !m_abort; i++, nPacket++)
			{
				UINT64 filepos = m_file->GetPos();

				MediaPacketHeader mph;

				if(!m_file->Read(mph, false))
				{
					break;
				}

				m_rtDuration = max((__int64)(10000i64 * mph.tStart), m_rtDuration);

				if(mph.stream == stream && (mph.flags & MediaPacketHeader::PN_KEYFRAME_FLAG) && tLastStart != mph.tStart)
				{
					IndexRecord* ir = new IndexRecord();

					ir->tStart = mph.tStart;
					ir->ptrFilePos = (DWORD)filepos;
					ir->packet = nPacket;

					m_file->m_irs.push_back(ir);

					tLastStart = mph.tStart;
				}

				m_nOpenProgress = m_file->GetPos() * 100 / m_file->GetLength();

				DWORD cmd;

				if(CheckRequest(&cmd))
				{
					if(cmd == CMD_EXIT) 
					{
						m_abort = true;
					}
					else 
					{
						Reply(S_OK);
					}
				}
			}
		}

		m_nOpenProgress = 100;

		if(m_abort)
		{
			for(auto i = m_file->m_irs.begin(); i != m_file->m_irs.end(); i++)
			{
				delete *i;
			}

			m_file->m_irs.clear();
		}

		m_abort = false;
	}

	m_seek.i = m_file->m_dcs.end();
	m_seek.packet = 0;
	m_seek.pos = 0;

	return true;
}

void CRealMediaSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if(rt > 0)
	{
		DWORD start = (DWORD)(rt / 10000);

		for(auto i = m_file->m_irs.rbegin(); i != m_file->m_irs.rend(); i++)
		{
			IndexRecord* ir = *i;

			if(ir->tStart > start)
			{
				continue;
			}

			m_seek.packet = ir->packet;
			m_seek.pos = ir->ptrFilePos;
			
			for(auto i = m_file->m_dcs.rbegin(); i != m_file->m_dcs.rend(); i++)
			{
				DataChunk* dc = *i;

				if(dc->pos <= ir->ptrFilePos)
				{
					auto j = m_file->m_dcs.begin();

					for(; *j != *i; j++)
					{
						m_seek.packet -= (*j)->nPackets;
					}

					m_seek.i = j;

					return;
				}
			}
		}
	}

	m_seek.i = m_file->m_dcs.begin(); 
	m_seek.packet = 0;
	m_seek.pos = m_file->m_dcs.front()->pos;
}

bool CRealMediaSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	DWORD stream = 0;

	for(auto i = m_file->m_subs.begin(); 
		i != m_file->m_subs.end() && SUCCEEDED(hr) && !CheckRequest(NULL); 
		i++, stream++)
	{
		const CRMFile::Subtitle& s = *i;

		int size = (4 + 1) + (2 + 4 + (s.name.size() + 1) * 2) + (2 + 4 + s.buff.size());

		CPacket* p = new CPacket(size);

		p->id = ~stream;

		p->start = 0;
		p->stop = 1;

		p->flags |= CPacket::SyncPoint | CPacket::TimeValid;

		BYTE* ptr = p->buff.data();

		strcpy((char*)ptr, "GAB2"); ptr += 4 + 1;
		*(WORD*)ptr = 2; ptr += 2;
		*(DWORD*)ptr = (s.name.size() + 1) * 2; ptr += 4;
		wcscpy((WCHAR*)ptr, s.name.c_str()); ptr += (s.name.size() + 1) * 2;
		*(WORD*)ptr = 4; ptr += 2;
		*(DWORD*)ptr = s.buff.size(); ptr += 4;
		memcpy(ptr, s.buff.data(), s.buff.size()); ptr += s.name.size();

		hr = DeliverPacket(p);
	}

	for(auto i = m_seek.i; i != m_file->m_dcs.end() && SUCCEEDED(hr) && !CheckRequest(NULL); i++)
	{
		DataChunk* dc = *i;

		m_file->Seek(m_seek.pos > 0 ? m_seek.pos : dc->pos);

		for(DWORD i = m_seek.packet; i < dc->nPackets && SUCCEEDED(hr) && !CheckRequest(NULL); i++)
		{
			MediaPacketHeader mph;

			if(!m_file->Read(mph))
			{
				break;
			}

			CPacket* p = new CPacket();

			p->id = mph.stream;

			p->start = 10000i64 * mph.tStart;
			p->stop = p->start + 1;

			p->flags = CPacket::TimeValid;

			if(mph.flags & MediaPacketHeader::PN_KEYFRAME_FLAG) 
			{
				p->flags |= CPacket::SyncPoint;
			}
			
			p->buff.swap(mph.buff);

			hr = DeliverPacket(p);
		}

		m_seek.packet = 0;
		m_seek.pos = 0;
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CRealMediaSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	if(m_file == NULL) 
	{
		return E_UNEXPECTED;
	}

	nKFs = m_file->m_irs.size();

	return S_OK;
}

STDMETHODIMP CRealMediaSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if(m_file == NULL) 
	{
		return E_UNEXPECTED;
	}

	if(*pFormat != TIME_FORMAT_MEDIA_TIME) 
	{
		return E_INVALIDARG;
	}

	UINT n = 0;

	for(auto i = m_file->m_irs.begin(); i != m_file->m_irs.end(); i++)
	{
		pKFs[n++] = 10000i64 * (*i)->tStart;
	}

	nKFs = n;

	return S_OK;
}

// CRealMediaOutputPin

CRealMediaSplitterFilter::CRealMediaOutputPin::CRealMediaOutputPin(const std::vector<CMediaType>& mts, LPCWSTR pName, CRealMediaSplitterFilter* pFilter, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, phr)
{
}

CRealMediaSplitterFilter::CRealMediaOutputPin::~CRealMediaOutputPin()
{
	m_segments.clear();
}

HRESULT CRealMediaSplitterFilter::CRealMediaOutputPin::DeliverEndFlush()
{
	m_segments.clear();

	return __super::DeliverEndFlush();
}

HRESULT CRealMediaSplitterFilter::CRealMediaOutputPin::DeliverSegments()
{
	if(m_segments.size() == 0)
	{
		m_segments.clear();

		return S_OK;
	}

	DWORD len = 0, total = 0;

	for(auto i = m_segments.begin(); i != m_segments.end(); i++)
	{
		Segment* s = *i;

		len = std::max<DWORD>(len, s->offset + s->buff.size());

		total += s->buff.size();
	}

	ASSERT(len == total);

	len += 1 + 2 * 4 * (!m_segments.merged ? m_segments.size() : 1);

	CPacket* p = new CPacket(len);

	p->id = -1;
	
	p->start = m_segments.start;
	p->stop = m_segments.start + 1;

	p->flags |= CPacket::TimeValid | m_segments.flags;

	BYTE* ptr = p->buff.data();

	*ptr++ = m_segments.merged ? 0 : m_segments.size() - 1;

	if(m_segments.merged)
	{
		*((DWORD*)ptr) = 1; ptr += 4;
		*((DWORD*)ptr) = 0; ptr += 4;
	}
	else
	{
		for(auto i = m_segments.begin(); i != m_segments.end(); i++)
		{
			*((DWORD*)ptr) = 1; ptr += 4;
			*((DWORD*)ptr) = (*i)->offset; ptr += 4;
		}
	}

	for(auto i = m_segments.begin(); i != m_segments.end(); i++)
	{
		memcpy(ptr + (*i)->offset, (*i)->buff.data(), (*i)->buff.size());
	}

	m_segments.clear();

	return __super::DeliverPacket(p);
}

HRESULT CRealMediaSplitterFilter::CRealMediaOutputPin::DeliverPacket(CPacket* p)
{
	HRESULT hr = S_OK;

	ASSERT(p->start < p->stop);

	if(m_mt.subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
	{
		// be => le

		WORD* ptr = (WORD*)p->buff.data();

		for(int i = p->buff.size() >> 1; i > 0; i--, ptr++)
		{
			*ptr = (*ptr >> 8) | (*ptr << 8);
		}
	}

	if(m_mt.subtype == MEDIASUBTYPE_RV10 || m_mt.subtype == MEDIASUBTYPE_RV20
	|| m_mt.subtype == MEDIASUBTYPE_RV30 || m_mt.subtype == MEDIASUBTYPE_RV40
	|| m_mt.subtype == MEDIASUBTYPE_RV41)
	{
		hr = DeliverRealVideoPacket(p);
	}
	else if(m_mt.subtype == MEDIASUBTYPE_RAAC || m_mt.subtype == MEDIASUBTYPE_RACP
		 || m_mt.subtype == MEDIASUBTYPE_AAC)
	{
		hr = DeliverRealAudioAACPacket(p);
	}
	else
	{
		hr = __super::DeliverPacket(p);
	}

	return hr;
}

HRESULT CRealMediaSplitterFilter::CRealMediaOutputPin::DeliverRealVideoPacket(CPacket* p)
{
	CAutoLock cAutoLock(&m_segments);

	HRESULT hr = S_OK;

	if(m_segments.start != p->start)
	{
		hr = DeliverSegments();
	}

	m_segments.start = p->start;
	m_segments.flags |= p->flags & (CPacket::Discontinuity | CPacket::SyncPoint);

	CBaseSplitterFile f(p->buff.data(), p->buff.size());

	while(f.GetRemaining() > 0 && hr == S_OK)
	{
		BYTE subseq = 0;
		BYTE seqnum = 0;
		int packetlen = 0;
		int packetoffset = 0;

		BYTE hdr = (BYTE)f.BitRead(8);

		if((hdr & 0xc0) == 0x40)
		{
			f.BitRead(8); // ?

			packetlen = (int)f.GetRemaining();
		}
		else
		{
			if((hdr & 0x40) == 0)
			{
				subseq = (BYTE)f.BitRead(8) & 0x7f;
			}

			if(f.BitRead(1, true)) 
			{
				m_segments.merged = true;
			}
			
			packetlen = (int)f.BitRead((f.BitRead(2) & 1) == 0 ? 30 : 14);
			packetoffset = (int)f.BitRead((f.BitRead(2) & 1) == 0 ? 30 : 14);

			if((hdr & 0xc0) == 0xc0)
			{
				m_segments.start = 10000i64 * packetoffset - m_start;
				
				packetoffset = 0;
			}
			else if((hdr & 0xc0) == 0x80)
			{
				packetoffset = packetlen - packetoffset;
			}

			seqnum = (BYTE)f.BitRead(8);
		}

		int len = std::min<int>((int)f.GetRemaining(), packetlen - packetoffset);

		Segment* s = new Segment();

		s->offset = packetoffset;
		s->buff.resize(len);
		f.ByteRead(s->buff.data(), len);

		m_segments.push_back(s);

		if((hdr & 0x80) || packetoffset + len >= packetlen)
		{
			hr = DeliverSegments();
		}
	}

	delete p;

	return hr;
}

HRESULT CRealMediaSplitterFilter::CRealMediaOutputPin::DeliverRealAudioAACPacket(CPacket* p)
{
	HRESULT hr = S_OK;

	BYTE* ptr = p->buff.data() + 2;

	std::list<WORD> sizes;

	int total = 0;
	int remaining = p->buff.size() - 2;
	int expected = ptr[-1] >> 4;

	while(total < remaining)
	{
		int size = (ptr[0] << 8) | ptr[1];
		sizes.push_back(size);
		total += size;
		ptr += 2;
		remaining -= 2;
		expected--;
	}

	ASSERT(total == remaining);
	ASSERT(expected == 0);

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.pbFormat;

	REFERENCE_TIME dur = 10240000000i64 / wfe->nSamplesPerSec * (wfe->cbSize > 2 ? 2 : 1);
	REFERENCE_TIME start = p->start;

	bool discontinuity = (p->flags & CPacket::Discontinuity) != 0;

	for(auto i = sizes.begin(); i != sizes.end(); i++)
	{
		WORD size = *i;

		CPacket* p = new CPacket();

		p->start = start;
		p->stop = start += dur;

		p->SetData(ptr, size);
		
		ptr += size;

		if(discontinuity)
		{
			p->flags |= CPacket::Discontinuity;
		}

		p->flags |= CPacket::SyncPoint | CPacket::TimeValid;

		discontinuity = false;

		if(S_OK != (hr = __super::DeliverPacket(p)))
		{
			break;
		}
	}

	delete p;

	return hr;
}

// CRealMediaSourceFilter

CRealMediaSourceFilter::CRealMediaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CRealMediaSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}

// CRMFile

CRMFile::CRMFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr)
{
	if(FAILED(hr)) return;

	hr = Init();
}

CRMFile::~CRMFile()
{
	for(auto i = m_mps.begin(); i != m_mps.end(); i++) delete *i;
	for(auto i = m_dcs.begin(); i != m_dcs.end(); i++) delete *i;
	for(auto i = m_irs.begin(); i != m_irs.end(); i++) delete *i;
}

bool CRMFile::Read(ChunkHdr& hdr)
{
	memset(&hdr, 0, sizeof(hdr));

	hdr.object_id = (DWORD)BitRead(32);
	hdr.size = (DWORD)BitRead(32);
	hdr.object_version = (WORD)BitRead(16);

	return true;
}

bool CRMFile::Read(MediaPacketHeader& mph, bool full)
{
	memset(&mph, 0, FIELD_OFFSET(MediaPacketHeader, buff));

	mph.stream = -1;

	HRESULT hr;

	WORD object_version = (WORD)BitRead(16);

	if(object_version != 0 && object_version != 1)
	{
		return true;
	}

	mph.len = (WORD)BitRead(16);
	mph.stream = (WORD)BitRead(16);
	mph.tStart = (DWORD)BitRead(32);
	mph.reserved = (BYTE)BitRead(8);
	mph.flags = (BYTE)BitRead(8);

	int len = mph.len;

	len -= sizeof(object_version);
	len -= FIELD_OFFSET(MediaPacketHeader, buff);
	ASSERT(len >= 0);
	len = max(len, 0);

	if(full)
	{
		mph.buff.resize(len);

		if(mph.len > 0 && S_OK != (hr = ByteRead(mph.buff.data(), len)))
		{
			return false;
		}
	}
	else
	{
		Seek(GetPos() + len);
	}

	return true;
}

HRESULT CRMFile::Init()
{
	if(BitRead(32, true) != '.RMF')
	{
		return E_FAIL;
	}

	ChunkHdr hdr;

	while(GetRemaining() > 0 && Read(hdr))
	{
		__int64 pos = GetPos() - sizeof(hdr);

		if(pos + hdr.size > GetLength() && hdr.object_id != 'DATA') // truncated?
		{
			break;
		}

		if(hdr.object_id == 0x2E7261FD) // '.ra+0xFD'
		{
			return E_FAIL;
		}

		MediaProperies* mp = NULL;
		DataChunk* dc = NULL;
		IndexChunkHeader ich;

		if(hdr.object_version == 0)
		{
			switch(hdr.object_id)
			{
			case '.RMF':

				m_fh.version = (DWORD)BitRead(32);
				m_fh.nHeaders = (DWORD)BitRead(hdr.size == 0x10 ? 16 : 32);
				break;

			case 'CONT':

				ReadAString(m_cd.title, (int)BitRead(16));
				ReadAString(m_cd.author, (int)BitRead(16));
				ReadAString(m_cd.copyright, (int)BitRead(16));
				ReadAString(m_cd.comment, (int)BitRead(16));
				break;

			case 'PROP':

				m_p.maxBitRate = (DWORD)BitRead(32);
				m_p.avgBitRate = (DWORD)BitRead(32);
				m_p.maxPacketSize = (DWORD)BitRead(32);
				m_p.avgPacketSize = (DWORD)BitRead(32);
				m_p.nPackets = (DWORD)BitRead(32);
				m_p.tDuration = (DWORD)BitRead(32);
				m_p.tPreroll = (DWORD)BitRead(32);
				m_p.ptrIndex = (DWORD)BitRead(32);
				m_p.ptrData = (DWORD)BitRead(32);
				m_p.nStreams = (WORD)BitRead(16);
				m_p.flags = (WORD)BitRead(16);
				break;

			case 'MDPR':

				mp = new MediaProperies();
				mp->stream = (WORD)BitRead(16);
				mp->maxBitRate = (DWORD)BitRead(32);
				mp->avgBitRate = (DWORD)BitRead(32);
				mp->maxPacketSize = (DWORD)BitRead(32);
				mp->avgPacketSize = (DWORD)BitRead(32);
				mp->tStart = (DWORD)BitRead(32);
				mp->tPreroll = (DWORD)BitRead(32);
				mp->tDuration = (DWORD)BitRead(32);
				ReadAString(mp->name, (int)BitRead(8));
				ReadAString(mp->mime, (int)BitRead(8));
				mp->buff.resize((DWORD)BitRead(32));
				ByteRead(mp->buff.data(), mp->buff.size());
				mp->width = mp->height = 0;
				mp->interlaced = mp->top_field_first = false;
				m_mps.push_back(mp);
				break;

			case 'DATA':

				dc = new DataChunk();
				dc->nPackets = (DWORD)BitRead(32);
				dc->ptrNext = (DWORD)BitRead(32);
				dc->pos = GetPos();
				m_dcs.push_back(dc);
                GetDimensions();
				break;

			case 'INDX':

				ich.nIndices = (DWORD)BitRead(32);
				ich.stream = (WORD)BitRead(16);
				ich.ptrNext = (DWORD)BitRead(32);

				for(DWORD i = 0, stream = GetMasterStream(); i < ich.nIndices; i++)
				{
					if(BitRead(16) == 0)
					{
						IndexRecord* ir = new IndexRecord();
						
						ir->tStart = (DWORD)BitRead(32);
						ir->ptrFilePos = (DWORD)BitRead(32);
						ir->packet = (DWORD)BitRead(32);

						if(ich.stream == stream) 
						{
							m_irs.push_back(ir);
						}
					}
				}

				break;

			case '.SUB':

				if(hdr.size > sizeof(hdr))
				{
					int size = hdr.size - sizeof(hdr);

					__int64 end = GetPos() + size;

					while(GetPos() < end)
					{
						Subtitle s;

						ReadAString(s.name, -1);
						ReadString(s.length, -1);
						ReadString(s.buff, atoi(s.length.c_str()));

						m_subs.push_back(s);
					}
				}

				break;
			}
		}

		ASSERT(hdr.object_id == 'DATA' 
			|| GetPos() == pos + hdr.size 
			|| GetPos() == pos + sizeof(hdr));

		pos += hdr.size;

		if(pos > GetPos()) 
		{
			Seek(pos);
		}
	}

	return S_OK;
}

void CRMFile::GetDimensions()
{
	for(auto i = m_mps.begin(); i != m_mps.end(); i++)
	{
		__int64 pos = GetPos();

		MediaProperies* pmp = *i;

		if(pmp->mime == L"video/x-pn-realvideo")
		{
			pmp->width = pmp->height = 0;

			RealVideoDesc rv(pmp->buff.data());

			if(rv.fcc2 != 'RV40' && rv.fcc2 != 'RV41')
			{
				continue;
			}

			MediaPacketHeader mph;

			while(Read(mph))
			{
				if(mph.stream != pmp->stream || mph.len == 0 || (mph.flags & MediaPacketHeader::PN_KEYFRAME_FLAG) == 0)
				{
					continue;
				}

				CBaseSplitterFile f(mph.buff.data(), mph.buff.size());

				BYTE hdr = (BYTE)f.BitRead(8);

				int packetlen = 0;
				int packetoffset = 0;

				if((hdr & 0xc0) == 0x40)
				{
					f.BitRead(8); // ?

					packetlen = (int)f.GetRemaining();
				}
				else
				{
					if((hdr & 0x40) == 0)
					{
						f.BitRead(8); // subseq
					}

					packetlen = (int)f.BitRead((f.BitRead(2) & 1) == 0 ? 30 : 14);
					packetoffset = (int)f.BitRead((f.BitRead(2) & 1) == 0 ? 30 : 14);

					if((hdr & 0xc0) == 0xc0) 
					{
						packetoffset = 0;
					}
					else if((hdr & 0xc0) == 0x80) 
					{
						packetoffset = packetlen - packetoffset;
					}

					f.BitRead(8); // seqnum
				}

				if(min(f.GetRemaining(), packetlen - packetoffset) > 0)
				{
					bool repeat_field = false;

					if(rv.fcc2 == 'RV41') 
					{
						pmp->interlaced = false;
						pmp->top_field_first = false;

						f.BitRead(9); // ?

						if(f.BitRead(1))
						{
							if(f.BitRead(1)) pmp->interlaced = true;
							if(f.BitRead(1)) pmp->top_field_first = true;
							if(f.BitRead(1)) repeat_field = true;
							f.BitRead(1); // ?
							if(f.BitRead(1)) f.BitRead(2); // ?
						}

						f.BitRead(16); // ?
					}
					else 
					{
						f.BitRead(13); // ?
						f.BitRead(13); // ?
					}

					static const int cw[8] = {160, 176, 240, 320, 352, 640, 704, 0};
					static const int ch1[8] = {120, 132, 144, 240, 288, 480, 0, 0};
					static const int ch2[4] = {180, 360, 576, 0};

					int c = 0;

					pmp->width = cw[(int)f.BitRead(3)];

					if(pmp->width == 0)
					{
						do
						{
							c = (int)f.BitRead(8);

							pmp->width += c << 2;
						}
						while(c == 255);
					}

					c = (int)f.BitRead(3);

					pmp->height = ch1[c];

					if(pmp->height == 0)
					{
						c = ((c << 1) | f.BitRead(1)) & 3;

						pmp->height = ch2[c];

						if(pmp->height == 0)
						{
							do
							{
								c = (int)f.BitRead(8);

								pmp->height += c << 2;
							}
							while(c == 255);
						}
					}

					if(rv.w == pmp->width && rv.h == pmp->height)
					{
						pmp->width = pmp->height = 0;
					}

					break;
				}
			}
		}
		
		Seek(pos);
	}
}

int CRMFile::GetMasterStream()
{
	int v = -1, a = -1;

	for(auto i = m_mps.begin(); i != m_mps.end(); i++)
	{
		MediaProperies* pmp = *i;

		if(pmp->mime == L"video/x-pn-realvideo") 
		{
			v = pmp->stream;

			break;
		}
		else if(pmp->mime == L"audio/x-pn-realaudio" && a == -1) 
		{
			a = pmp->stream;
		}
	}

	if(v == -1)
	{
		v = a;
	}

	return v;
}

//

RealVideoDesc::RealVideoDesc(const BYTE* buff)
{
	CBaseSplitterFile f(buff, sizeof(*this));

	dwSize = (DWORD)f.BitRead(32);
	fcc1 = (DWORD)f.BitRead(32);
	fcc2 = (DWORD)f.BitRead(32);
	w = (WORD)f.BitRead(16);
	h = (WORD)f.BitRead(16);
	bpp = (WORD)f.BitRead(16);
	unk1 = (DWORD)f.BitRead(32);
	fps = (DWORD)f.BitRead(32);
	type1 = (DWORD)f.BitRead(32);
	type2 = (DWORD)f.BitRead(32);
	f.ByteRead(morewh, sizeof(morewh));
}

RealAudioDesc::RealAudioDesc(const BYTE* buff)
{
	CBaseSplitterFile f(buff, sizeof(*this));

	fourcc1 = (DWORD)f.BitRead(32);
	version1 = (WORD)f.BitRead(16);
	unknown1 = (WORD)f.BitRead(16);
	fourcc2 = (DWORD)f.BitRead(32);
	unknown2 = (DWORD)f.BitRead(32);
	version2 = (WORD)f.BitRead(16);
	header_size = (DWORD)f.BitRead(32);
	flavor = (WORD)f.BitRead(16);
	coded_frame_size = (DWORD)f.BitRead(32);
	unknown3 = (DWORD)f.BitRead(32);
	unknown4 = (DWORD)f.BitRead(32);
	unknown5 = (DWORD)f.BitRead(32);
	sub_packet_h = (WORD)f.BitRead(16);
	frame_size = (WORD)f.BitRead(16);
	sub_packet_size = (WORD)f.BitRead(16);
	unknown6 = (WORD)f.BitRead(16);
}

RealAudioDesc4::RealAudioDesc4(const BYTE* buff)
{
	CBaseSplitterFile f(buff, sizeof(*this));

	sample_rate = (WORD)f.BitRead(16);
	unknown8 = (WORD)f.BitRead(16);
	sample_size = (WORD)f.BitRead(16);
	channels = (WORD)f.BitRead(16);
}

RealAudioDesc5::RealAudioDesc5(const BYTE* buff)
{
	CBaseSplitterFile f(buff, sizeof(*this));

	f.ByteRead(unknown7, sizeof(unknown7));
	sample_rate = (WORD)f.BitRead(16);
	unknown8 = (WORD)f.BitRead(16);
	sample_size = (WORD)f.BitRead(16);
	channels = (WORD)f.BitRead(16);
	genr = (DWORD)f.BitRead(32);
	fourcc3 = (DWORD)f.BitRead(32);
}

// CRealVideoDecoder

CRealVideoDecoder::CRealVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseVideoFilter(NAME("CRealVideoDecoder"), lpunk, phr, __uuidof(this))
	, m_dll(NULL)
	, m_cookie(0)
{
}

CRealVideoDecoder::~CRealVideoDecoder()
{
	if(m_dll != NULL) FreeLibrary(m_dll);
}

HRESULT CRealVideoDecoder::InitRV(const CMediaType* pmt)
{
	FreeRV();

	HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

	RealVideoDesc rv(pmt->Format() + (pmt->formattype == FORMAT_VideoInfo ? sizeof(VIDEOINFOHEADER) : sizeof(VIDEOINFOHEADER2)));

	#pragma pack(push, 1)
	struct {WORD unk1, w, h, unk3; DWORD unk2, subformat, unk5, format;} init = {11, rv.w, rv.h, 0, 0, rv.type1, 1, rv.type2};
	#pragma pack(pop)

	hr = RVInit(&init, &m_cookie);

	if(FAILED(hr))
	{
		return hr;
	}

	if(rv.fcc2 <= 'RV30' && rv.type2 >= 0x20200002)
	{
		int n = 1 + ((rv.type1 >> 16) & 7);

		DWORD* wh = new DWORD[n * 2];

		wh[0] = rv.w; 
		wh[1] = rv.h;

		for(int i = 2; i < n * 2; i++)
		{
			wh[i] = rv.morewh[i - 2] * 4;
		}

		#pragma pack(push, 1)
		struct {DWORD data1; DWORD data2; DWORD* dimensions;} cmsg_data = {0x24, n, wh};
		#pragma pack(pop)

		hr = RVCustomMessage(&cmsg_data, m_cookie);

		delete [] wh;
	}

	return hr;
}

void CRealVideoDecoder::FreeRV()
{
	if(m_cookie)
	{
		RVFree(m_cookie);

		m_cookie = 0;
	}
}

HRESULT CRealVideoDecoder::Transform(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

	BYTE* src = NULL;

	hr = pIn->GetPointer(&src);

	if(FAILED(hr))
	{
		return hr;
	}

	long len = pIn->GetActualDataLength();

	if(len <= 0) 
	{
		return S_OK; // nothing to do
	}

	REFERENCE_TIME start, stop;

	pIn->GetTime(&start, &stop);

	start += m_start;

	int offset = 1 + (*src + 1) * 8;

	#pragma pack(push, 1)
	struct {DWORD len, unk1, chunks; void* extra; DWORD unk2, ts;} transform_in = {len - offset, 0, *src, src + 1, 0, (DWORD)(start / 10000)};
	struct {DWORD unk1, unk2, timestamp, w, h;} transform_out = {0, 0, 0, 0, 0};
	#pragma pack(pop)

	src += offset;

	if(m_late > 0 && (m_timestamp + 1) == transform_in.ts)
	{
		m_timestamp = transform_in.ts;

		return S_OK;
	}

	hr = RVTransform(src, m_i420.data(), &transform_in, &transform_out, m_cookie);

	bool interlaced = ((src[1] >> 5) & 3) == 3;

	m_timestamp = transform_in.ts;

	if(FAILED(hr))
	{
		ASSERT((transform_out.unk1 & 1) == 0); // error allowed when the "render" flag is not set
//		return hr;
	}

	if(pIn->IsPreroll() == S_OK || start < 0 || (transform_out.unk1 & 1) == 0)
	{
		return S_OK;
	}

	int w = m_size.x;
	int h = m_size.y;

	CComPtr<IMediaSample> pOut;

	hr = GetDeliveryBuffer(w, h, &pOut);
	
	if(FAILED(hr))
	{
		return hr;
	}

	BYTE* dst = NULL;

	hr = pOut->GetPointer(&dst);

	if(FAILED(hr))
	{
		return hr;
	}

	BYTE* i420[3] = {m_i420.data(), m_i420tmp.data(), NULL};

	if(interlaced)
	{
		int size = w * h;
		
		Image::DeinterlaceBlend(i420[1], i420[0], w, h, w, w);
		Image::DeinterlaceBlend(i420[1] + size, i420[0] + size, w / 2, h / 2, w / 2, w / 2);
		Image::DeinterlaceBlend(i420[1] + size * 5 / 4, i420[0] + size * 5 / 4, w / 2, h / 2, w / 2, w / 2);

		i420[2] = i420[1];
		i420[1] = i420[0];
		i420[0] = i420[2];
	}

	if(transform_out.w != w || transform_out.h != h)
	{
		Resize(i420[0], transform_out.w, transform_out.h, i420[1], w, h);

		// only one of these can be true, and when it happens the result image must be in the tmp buffer

		if(transform_out.w == w || transform_out.h == h)
		{
			i420[2] = i420[1];
			i420[1] = i420[0];
			i420[0] = i420[2];
		}
	}

	start = 10000i64 * transform_out.timestamp - m_start;
	stop = start + 1;

	pOut->SetTime(&start, &stop); // NULL

	pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);

	CopyBuffer(dst, i420[0], w, h, w, MEDIASUBTYPE_I420);

	return m_pOutput->Deliver(pOut);
}

void CRealVideoDecoder::Resize(BYTE* src, DWORD wi, DWORD hi, BYTE* dst, DWORD wo, DWORD ho)
{
	int si = wi * hi;
	int so = wo * ho;

	ASSERT(((si * so) & 3) == 0);

	if(wi < wo)
	{
		ResizeWidth(src, wi, hi, dst, wo, ho);
		ResizeWidth(src + si, wi / 2, hi / 2, dst + so, wo / 2, ho / 2);
		ResizeWidth(src + si + si / 4, wi / 2, hi / 2, dst + so + so / 4, wo / 2, ho / 2);
		if(hi == ho) return; 
		ResizeHeight(dst, wo, hi, src, wo, ho);
		ResizeHeight(dst + so, wo / 2, hi / 2, src + so, wo / 2, ho / 2);
		ResizeHeight(dst + so + so / 4, wo / 2, hi / 2, src + so + so / 4, wo / 2, ho / 2);
	}
	else if(hi < ho)
	{
		ResizeHeight(src, wi, hi, dst, wo, ho);
		ResizeHeight(src + si, wi / 2, hi / 2, dst + so, wo / 2, ho / 2);
		ResizeHeight(src + si + si / 4, wi / 2, hi / 2, dst + so + so / 4, wo / 2, ho / 2);
		if(wi == wo) return;
		ASSERT(0); // this is uncreachable code, but anyway... looks nice being so symetric
		ResizeWidth(dst, wi, ho, src, wo, ho);
		ResizeWidth(dst + so, wi / 2, ho / 2, src + so, wo / 2, ho / 2);
		ResizeWidth(dst + so + so / 4, wi / 2, ho / 2, src + so + so / 4, wo / 2, ho / 2);
	}
}

void CRealVideoDecoder::ResizeWidth(BYTE* src, DWORD wi, DWORD hi, BYTE* dst, DWORD wo, DWORD ho)
{
	if(wi == wo)
	{
		for(DWORD y = 0; y < hi; y++, src += wi, dst += wo)
		{
			memcpy(dst, src, wo);
		}
	}
	else
	{
		for(DWORD y = 0; y < hi; y++, src += wi, dst += wo)
		{
			ResizeRow(src, wi, 1, dst, wo, 1);
		}
	}
}

void CRealVideoDecoder::ResizeHeight(BYTE* src, DWORD wi, DWORD hi, BYTE* dst, DWORD wo, DWORD ho)
{
	if(hi == ho) 
	{
		memcpy(dst, src, wo * ho);
	}
	else
	{
		for(DWORD x = 0; x < wo; x++, src++, dst++)
		{
			ResizeRow(src, hi, wo, dst, ho, wo);
		}
	}
}

void CRealVideoDecoder::ResizeRow(BYTE* src, DWORD wi, DWORD dpi, BYTE* dst, DWORD wo, DWORD dpo)
{
	ASSERT(wi < wo);

	// TODO: sse

    if(dpo == 1)
	{
		for(DWORD i = 0, j = 0, dj = (wi << 16) / wo; i < wo - 1; i++, dst++, j += dj)
//			dst[i] = src[j >> 16];
		{
			BYTE* p = &src[j >> 16];
			DWORD jf = j & 0xffff;
			*dst = ((p[0] * (0xffff - jf) + p[1] * jf) + 0x7fff) >> 16;
		}

		*dst = src[wi - 1];
	}
	else
	{
		for(DWORD i = 0, j = 0, dj = (wi << 16) / wo; i < wo - 1; i++, dst += dpo, j += dj)
//			*dst = src[dpi * (j >> 16)];
		{
			BYTE* p = &src[dpi * (j >> 16)];
			DWORD jf = j & 0xffff;
			*dst = ((p[0] * (0xffff - jf) + p[dpi] * jf) + 0x7fff) >> 16;
		}

		*dst = src[dpi * (wi - 1)];
	}
}

HRESULT CRealVideoDecoder::CheckInputType(const CMediaType* mtIn)
{
	if(mtIn->majortype != MEDIATYPE_Video 
	|| mtIn->subtype != MEDIASUBTYPE_RV20
	&& mtIn->subtype != MEDIASUBTYPE_RV30 
	&& mtIn->subtype != MEDIASUBTYPE_RV40
	&& mtIn->subtype != MEDIASUBTYPE_RV41)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if(mtIn->formattype == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mtIn->Format();

		if(vih->dwPictAspectRatioX < vih->bmiHeader.biWidth || vih->dwPictAspectRatioY < vih->bmiHeader.biHeight)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	if(!m_pInput->IsConnected())
	{
		if(m_dll) 
		{
			FreeLibrary(m_dll); 

			m_dll = NULL;
		}

		std::list<std::wstring> paths;
		std::wstring olddll, newdll, oldpath, newpath;

		olddll = Util::Format(L"drv%c3260.dll", (mtIn->subtype.Data1 >> 16) & 0xff);

		newdll = 
			mtIn->subtype == FOURCCMap('14VR') ? L"drvi.dll" :
			mtIn->subtype == FOURCCMap('02VR') ? L"drv2.dll" :
			L"drvc.dll";

		paths.push_back(newdll);
		paths.push_back(olddll);

		CRegKey key;

		wchar_t buff[MAX_PATH];
		ULONG len = sizeof(buff);

		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"Software\\RealNetworks\\Preferences\\DT_Codecs", KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && wcslen(buff) > 0)
		{
			oldpath = buff;
			wchar_t c = oldpath.back();
			if(c != '\\' && c != '/') oldpath += '\\';
			key.Close();
		}

		len = sizeof(buff);

		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"Helix\\HelixSDK\\10.0\\Preferences\\DT_Codecs", KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && wcslen(buff) > 0)
		{
			newpath = buff;
			wchar_t c = newpath.back();
			if(c != '\\' && c != '/') newpath += '\\';
			key.Close();
		}

		if(!newpath.empty()) paths.push_back(newpath + newdll);
		if(!oldpath.empty()) paths.push_back(oldpath + newdll);

		paths.push_back(newdll); // default dll paths

		if(!newpath.empty()) paths.push_back(newpath + olddll);
		if(!oldpath.empty()) paths.push_back(oldpath + olddll);

		paths.push_back(olddll); // default dll paths

		for(auto i = paths.begin(); i != paths.end(); i++)
		{
			m_dll = LoadLibrary((*i).c_str());

			if(m_dll != NULL)
			{
				RVCustomMessage = (PRVCustomMessage)GetProcAddress(m_dll, "RV20toYUV420CustomMessage");
				RVFree = (PRVFree)GetProcAddress(m_dll, "RV20toYUV420Free");
				RVHiveMessage = (PRVHiveMessage)GetProcAddress(m_dll, "RV20toYUV420HiveMessage");
				RVInit = (PRVInit)GetProcAddress(m_dll, "RV20toYUV420Init");
				RVTransform = (PRVTransform)GetProcAddress(m_dll, "RV20toYUV420Transform");

				if(RVCustomMessage == NULL) RVCustomMessage = (PRVCustomMessage)GetProcAddress(m_dll, "RV40toYUV420CustomMessage");
				if(RVFree == NULL) RVFree = (PRVFree)GetProcAddress(m_dll, "RV40toYUV420Free");
				if(RVHiveMessage == NULL) RVHiveMessage = (PRVHiveMessage)GetProcAddress(m_dll, "RV40toYUV420HiveMessage");
				if(RVInit == NULL) RVInit = (PRVInit)GetProcAddress(m_dll, "RV40toYUV420Init");
				if(RVTransform == NULL) RVTransform = (PRVTransform)GetProcAddress(m_dll, "RV40toYUV420Transform");

				break;
			}
		}

		if(m_dll == NULL 
		|| RVCustomMessage == NULL
		|| RVFree == NULL
		|| RVHiveMessage == NULL
		|| RVInit == NULL
		|| RVTransform == NULL)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		if(FAILED(InitRV(mtIn)))
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return S_OK;
}

HRESULT CRealVideoDecoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	BITMAPINFOHEADER bihIn, bihOut;

	if(!CMediaTypeEx(*mtIn).ExtractBIH(&bihIn)
	|| !CMediaTypeEx(*mtOut).ExtractBIH(&bihOut)
	|| std::abs(bihIn.biHeight) != std::abs(bihOut.biHeight))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return __super::CheckTransform(mtIn, mtOut);
}

HRESULT CRealVideoDecoder::StartStreaming()
{
	const CMediaType& mt = m_pInput->CurrentMediaType();

	if(FAILED(InitRV(&mt)))
	{
		return E_FAIL;
	}

	int size = m_size.x * m_size.y;

	m_i420.resize(size * 3 / 2);
	memset(&m_i420[0], 0, size);
	memset(&m_i420[size], 0x80, size / 2);
	m_i420tmp.resize(size * 3 / 2);
	memset(&m_i420tmp[0], 0, size);
	memset(&m_i420tmp[size], 0x80, size / 2);

	return __super::StartStreaming();
}

HRESULT CRealVideoDecoder::StopStreaming()
{
	m_i420.clear();
	m_i420tmp.clear();

	FreeRV();

	return __super::StopStreaming();
}

HRESULT CRealVideoDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_timestamp = ~0;

	DWORD tmp[2] = {20, 0};

	RVHiveMessage(tmp, m_cookie);

	m_start = tStart;

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CRealVideoDecoder::AlterQuality(Quality q)
{
	if(q.Late > 500 * 10000i64) m_late = 500;
	if(q.Late <= 0) m_late = 0;

	return S_OK;
}

// CRealAudioDecoder

CRealAudioDecoder::CRealAudioDecoder(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(NAME("CRealAudioDecoder"), lpunk, __uuidof(this))
	, m_dll(NULL)
	, m_cookie(0)
	, m_ra(NULL)
{
	if(phr) *phr = S_OK;
}

CRealAudioDecoder::~CRealAudioDecoder()
{
//	FreeRA();

	delete m_ra;

	if(m_dll != NULL)
	{
		FreeLibrary(m_dll);
	}
}

HRESULT CRealAudioDecoder::InitRA(const CMediaType* pmt)
{
	FreeRA();

	HRESULT hr;

	if(RAOpenCodec2 != NULL && FAILED(RAOpenCodec2(&m_cookie, m_dllpath.c_str()))
	|| RAOpenCodec != NULL && FAILED(RAOpenCodec(&m_cookie)))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->Format();

	// someone might be doing cbSize = sizeof(WAVEFORMATEX), chances of 
	// cbSize being really sizeof(WAVEFORMATEX) is less than this, 
	// especially with our rm splitter ;)

	DWORD cbSize = wfe->cbSize;

	if(cbSize == sizeof(WAVEFORMATEX)) 
	{
		ASSERT(0); 

		cbSize = 0;
	}

	WORD wBitsPerSample = wfe->wBitsPerSample;

	if(wBitsPerSample == 0)
	{
		wBitsPerSample = 16;
	}

	#pragma pack(push, 1)
	struct 
	{
		DWORD freq; 
		WORD bpsample, channels, quality; 
		DWORD bpframe, packetsize, extralen; 
		const void* extra;
	} initdata = {wfe->nSamplesPerSec, wBitsPerSample, wfe->nChannels, 100, 0, 0, 0, NULL};
	#pragma pack(pop)

	std::vector<BYTE> buff;

	if(pmt->subtype == MEDIASUBTYPE_AAC)
	{
		buff.resize(cbSize + 1);
		buff[0] = 0x02;
		memcpy(&buff[1], wfe + 1, cbSize);
		initdata.extralen = cbSize + 1;
		initdata.extra = buff.data();
	}
	else
	{
		if(pmt->FormatLength() <= sizeof(WAVEFORMATEX) + cbSize) // must have type_specific_data appended
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		BYTE* fmt = pmt->Format() + sizeof(WAVEFORMATEX) + cbSize;

		for(int i = 0, j = pmt->FormatLength() - (sizeof(WAVEFORMATEX) + cbSize) - 4; i < j; i++, fmt++)
		{
			if(fmt[0] == '.' || fmt[1] == 'r' || fmt[2] == 'a')
			{
				break;
			}
		}

		m_ra = new RealAudioDesc((BYTE*)fmt);

		const BYTE* p = fmt + sizeof(RealAudioDesc);

		if(m_ra->version2 == 4)
		{
			p += sizeof(RealAudioDesc4);
			p += *p++; ASSERT(*p == 4);
			p += *p++;
		}
		else if(m_ra->version2 == 5)
		{
			p += sizeof(RealAudioDesc5);
		}
		else
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		p += 3;

		if(m_ra->version2 == 5)
		{
			p++;
		}

		initdata.bpframe = m_ra->sub_packet_size;
		initdata.packetsize = m_ra->coded_frame_size;
		initdata.extralen = *(DWORD*)p;
		initdata.extra = p + 4;
	}

	hr = RAInitDecoder(m_cookie, &initdata);

	if(FAILED(hr))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if(RASetPwd != NULL)
	{
		RASetPwd(m_cookie, "Ardubancel Quazanga");
	}

	if(RASetFlavor != NULL)
	{
		hr = RASetFlavor(m_cookie, m_ra->flavor);
		
		if(FAILED(hr))
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return S_OK;
}

void CRealAudioDecoder::FreeRA()
{
	if(m_ra != NULL)
	{
		delete m_ra;

		m_ra = NULL;
	}

	if(m_cookie)
	{
		RAFreeDecoder(m_cookie);
		RACloseCodec(m_cookie);

		m_cookie = 0;
	}
}

HRESULT CRealAudioDecoder::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();

    if(pProps->dwStreamId != AM_STREAM_MEDIA)
	{
		return m_pOutput->Deliver(pIn);
	}

	BYTE* src = NULL;

	hr = pIn->GetPointer(&src);

	if(FAILED(hr))
	{
		return hr;
	}
	
	BYTE* base = src;

	long len = pIn->GetActualDataLength();

	if(len <= 0)
	{
		return S_OK;
	}

	REFERENCE_TIME start, stop;

	pIn->GetTime(&start, &stop);

	// if(pIn->IsPreroll() == S_OK || start < 0) return S_OK;

	if(S_OK == pIn->IsSyncPoint())
	{
		m_size = 0;
		m_buffstart = start;
		m_discontinuity = pIn->IsDiscontinuity() == S_OK;
	}

	BYTE* s = NULL;
	BYTE* d = NULL;

	int w = m_ra->coded_frame_size;
	int h = m_ra->sub_packet_h;
	int sps = m_ra->sub_packet_size;

	const CMediaType& mt = m_pInput->CurrentMediaType();

	if(mt.subtype == MEDIASUBTYPE_RAAC
	|| mt.subtype == MEDIASUBTYPE_RACP
	|| mt.subtype == MEDIASUBTYPE_AAC)
	{
		s = src;
		d = src + len;
		w = len;
	}
	else
	{
		memcpy(&m_buff[m_size], src, len);

		m_size += len;

		len = w * h;

		if(m_size >= len)
		{
			ASSERT(m_size == len);

			s = &m_buff[0];
			d = &m_buff[len];

			if(sps > 0 && (mt.subtype == MEDIASUBTYPE_COOK || mt.subtype == MEDIASUBTYPE_ATRC))
			{
				for(int y = 0; y < h; y++)
				{
					for(int x = 0, w2 = w / sps; x < w2; x++, s += sps)
					{
						memcpy(d + sps * (h * x + ((h + 1) / 2) * (y & 1) + (y >> 1)), s, sps);
					}
				}

				s = &m_buff[len];
				d = &m_buff[len * 2];
			}
			else if(mt.subtype == MEDIASUBTYPE_SIPR)
			{
				// http://mplayerhq.hu/pipermail/mplayer-dev-eng/2002-August/010569.html

				static BYTE sipr_swaps[38][2] = 
				{
					{ 0,63}, { 1,22}, { 2,44}, { 3,90}, { 5,81}, { 7,31}, { 8,86}, { 9,58}, 
					{10,36}, {12,68}, {13,39}, {14,73}, {15,53}, {16,69}, {17,57}, {19,88},
					{20,34}, {21,71}, {24,46}, {25,94}, {26,54}, {28,75}, {29,50}, {32,70}, 
					{33,92}, {35,74}, {38,85}, {40,56}, {42,87}, {43,65}, {45,59}, {48,79},
					{49,93}, {51,89}, {55,95}, {61,76}, {67,83}, {77,80}
				};

				int bs = h * w * 2 / 96; // nibbles per subpacket

				for(int n = 0; n < 38; n++)
				{
					int i = bs * sipr_swaps[n][0];
					int o = bs * sipr_swaps[n][1];
					
					// swap nibbles of block 'i' with 'o'      TODO: optimize
					
					for(int j = 0; j < bs; j++, i++, o++)
					{
						int x = (i & 1) ? (s[i >> 1] >> 4) : (s[i >> 1] & 0x0F);
						int y = (o & 1) ? (s[o >> 1] >> 4) : (s[o >> 1] & 0x0F);
						
						if(o & 1) s[o >> 1] = (s[o >> 1] & 0x0F) | (x << 4);
						else s[o >> 1] = (s[o >> 1] & 0xF0) | x;

						if(i & 1) s[i >> 1] = (s[(i >> 1)] & 0x0F) | (y << 4);
						else s[i >> 1] = (s[i >> 1] & 0xF0) | y;
					}
				}
			}

			m_size = 0;
		}
	}

	start = m_buffstart;

	for(; s < d; s += w)
	{
		CComPtr<IMediaSample> pOut;

		hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0);

		if(FAILED(hr))
		{
			return hr;
		}

		BYTE* dst = NULL;

		hr = pOut->GetPointer(&dst);

		if(FAILED(hr))
		{
			return hr;
		}

		AM_MEDIA_TYPE* pmt = NULL;

		if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt != NULL)
		{
			CMediaType mt(*pmt);
			m_pOutput->SetMediaType(&mt);
			DeleteMediaType(pmt);
		}

		hr = RADecode(m_cookie, s, w, dst, &len, -1);

		if(FAILED(hr))
		{
			continue;
		}

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

		stop = start + 10000000i64 * len / wfe->nAvgBytesPerSec;

		pOut->SetTime(&start, &stop);
		pOut->SetMediaTime(NULL, NULL);

		pOut->SetDiscontinuity(m_discontinuity);
		pOut->SetSyncPoint(TRUE);

		m_discontinuity = false;

		pOut->SetActualDataLength(len);

		if(start >= 0)
		{
			hr = m_pOutput->Deliver(pOut);

			if(S_OK != hr)
			{
				return hr;
			}
		}

		start = stop;
	}

	m_buffstart = start;

	return S_OK;
}

HRESULT CRealAudioDecoder::CheckInputType(const CMediaType* mtIn)
{
	if(mtIn->majortype != MEDIATYPE_Audio 
	|| mtIn->subtype != MEDIASUBTYPE_14_4
	&& mtIn->subtype != MEDIASUBTYPE_28_8
	&& mtIn->subtype != MEDIASUBTYPE_ATRC
	&& mtIn->subtype != MEDIASUBTYPE_COOK
	&& mtIn->subtype != MEDIASUBTYPE_DNET
	&& mtIn->subtype != MEDIASUBTYPE_SIPR
	&& mtIn->subtype != MEDIASUBTYPE_RAAC
	&& mtIn->subtype != MEDIASUBTYPE_RACP
	&& mtIn->subtype != MEDIASUBTYPE_AAC)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if(!m_pInput->IsConnected())
	{
		if(m_dll != NULL) 
		{
			FreeLibrary(m_dll);
			
			m_dll = NULL;
		}

		std::list<std::wstring> paths;
		std::wstring olddll, newdll, oldpath, newpath;

		DWORD fcc = mtIn->subtype.Data1;

		if(fcc == 'PCAR' || fcc == 0xff)
		{
			fcc = 'CAAR';
		}

		std::wstring fccstr = Util::Format(L"%c%c%c%c", fcc & 0xff, (fcc >> 8) & 0xff, (fcc >> 16) & 0xff, fcc >> 24);

		olddll = fccstr + L"3260.dll";
		newdll = fccstr + L".dll";

		paths.push_back(newdll);
		paths.push_back(olddll);

		CRegKey key;

		wchar_t buff[MAX_PATH];
		ULONG len = sizeof(buff);

		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"Software\\RealNetworks\\Preferences\\DT_Codecs", KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && wcslen(buff) > 0)
		{
			oldpath = buff;
			wchar_t c = oldpath.back();
			if(c != '\\' && c != '/') oldpath += '\\';
			key.Close();
		}

		len = sizeof(buff);
		
		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"Helix\\HelixSDK\\10.0\\Preferences\\DT_Codecs", KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && wcslen(buff) > 0)
		{
			newpath = buff;
			wchar_t c = newpath.back();
			if(c != '\\' && c != '/') newpath += '\\';
			key.Close();
		}

		if(!newpath.empty()) paths.push_back(newpath + newdll);
		if(!oldpath.empty()) paths.push_back(oldpath + newdll);

		paths.push_back(newdll); // default dll paths
		
		if(!newpath.empty()) paths.push_back(newpath + olddll);
		if(!oldpath.empty()) paths.push_back(oldpath + olddll);
		
		paths.push_back(olddll); // default dll paths

		for(auto i = paths.begin(); i != paths.end(); i++)
		{
			m_dll = LoadLibrary((*i).c_str());

			if(m_dll != NULL)
			{
				RACloseCodec = (PCloseCodec)GetProcAddress(m_dll, "RACloseCodec");
				RADecode = (PDecode)GetProcAddress(m_dll, "RADecode");
				RAFlush = (PFlush)GetProcAddress(m_dll, "RAFlush");
				RAFreeDecoder = (PFreeDecoder)GetProcAddress(m_dll, "RAFreeDecoder");
				RAGetFlavorProperty = (PGetFlavorProperty)GetProcAddress(m_dll, "RAGetFlavorProperty");
				RAInitDecoder = (PInitDecoder)GetProcAddress(m_dll, "RAInitDecoder");
				RAOpenCodec = (POpenCodec)GetProcAddress(m_dll, "RAOpenCodec");
				RAOpenCodec2 = (POpenCodec2)GetProcAddress(m_dll, "RAOpenCodec2");
				RASetFlavor = (PSetFlavor)GetProcAddress(m_dll, "RASetFlavor");
				RASetDLLAccessPath = (PSetDLLAccessPath)GetProcAddress(m_dll, "RASetDLLAccessPath");
				RASetPwd = (PSetPwd)GetProcAddress(m_dll, "RASetPwd");

				break;
			}
		}

		if(m_dll == NULL
		|| RACloseCodec == NULL
		|| RADecode == NULL 
		// || RAFlush == NULL
		|| RAFreeDecoder == NULL
		|| RAGetFlavorProperty == NULL
		// || RASetFlavor == NULL
		|| RAInitDecoder == NULL
		|| RAOpenCodec == NULL && RAOpenCodec2 == NULL)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		{
			char buff[MAX_PATH];
			GetModuleFileNameA(m_dll, buff, MAX_PATH);
			PathRemoveFileSpecA(buff);
			PathAddBackslashA(buff);
			m_dllpath = buff;
		}
		
		if(RASetDLLAccessPath != NULL)
		{
			RASetDLLAccessPath(("DT_Codecs=" + m_dllpath).c_str());
		}

		if(FAILED(InitRA(mtIn)))
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return S_OK;
}

HRESULT CRealAudioDecoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(mtIn->majortype == MEDIATYPE_Audio && mtOut->majortype == MEDIATYPE_Audio)
	{
		if(mtOut->subtype == MEDIASUBTYPE_PCM)
		{
			if(mtIn->subtype == MEDIASUBTYPE_14_4
			|| mtIn->subtype == MEDIASUBTYPE_28_8
			|| mtIn->subtype == MEDIASUBTYPE_ATRC
			|| mtIn->subtype == MEDIASUBTYPE_COOK
			|| mtIn->subtype == MEDIASUBTYPE_DNET
			|| mtIn->subtype == MEDIASUBTYPE_SIPR
			|| mtIn->subtype == MEDIASUBTYPE_RAAC
			|| mtIn->subtype == MEDIASUBTYPE_RACP
			|| mtIn->subtype == MEDIASUBTYPE_AAC)
			{
				return S_OK;
			}
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CRealAudioDecoder::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

	WORD wBitsPerSample = wfe->wBitsPerSample;
	
	if(wBitsPerSample == 0)
	{
		wBitsPerSample = 16;
	}

	// maybe this is too much...

	pProperties->cBuffers = 8;
	pProperties->cbBuffer = wfe->nChannels * wfe->nSamplesPerSec * wBitsPerSample >> 3; // nAvgBytesPerSec;

	return S_OK;
}

HRESULT CRealAudioDecoder::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	*pmt = m_pInput->CurrentMediaType();

	pmt->subtype = MEDIASUBTYPE_PCM;
	
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->ReallocFormatBuffer(sizeof(WAVEFORMATEX));

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > (wfe->nChannels > 2 && wfe->nChannels <= 6 ? 1 : 0)) return VFW_S_NO_MORE_ITEMS;

	if(wfe->wBitsPerSample == 0) 
	{
		wfe->wBitsPerSample = 16;
	}

	wfe->cbSize = 0;
	wfe->wFormatTag = WAVE_FORMAT_PCM;
	wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample >> 3;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec *  wfe->nBlockAlign;

	if(iPosition == 0 && wfe->nChannels > 2 && wfe->nChannels <= 6)
	{
		static DWORD chmask[] = 
		{
			KSAUDIO_SPEAKER_DIRECTOUT,
			KSAUDIO_SPEAKER_MONO,
			KSAUDIO_SPEAKER_STEREO,
			KSAUDIO_SPEAKER_STEREO | SPEAKER_FRONT_CENTER,
			KSAUDIO_SPEAKER_QUAD,
			KSAUDIO_SPEAKER_QUAD | SPEAKER_FRONT_CENTER,
			KSAUDIO_SPEAKER_5POINT1
		};
		
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)pmt->ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));

		wfex->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfex->dwChannelMask = chmask[wfex->Format.nChannels];
		wfex->Samples.wValidBitsPerSample = wfex->Format.wBitsPerSample;
		wfex->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	}

	return S_OK;
}

HRESULT CRealAudioDecoder::StartStreaming()
{
	int w = m_ra->coded_frame_size;
	int h = m_ra->sub_packet_h;
	int sps = m_ra->sub_packet_size;

	int len = w * h;

	m_buff.resize(len * 2 + 1); // + 1 for debug mode (&m_buff[m_buff.size()] would assert)
	m_size = 0;
	m_buffstart = 0;

	return __super::StartStreaming();
}

HRESULT CRealAudioDecoder::StopStreaming()
{
	m_buff.clear();
	m_size = 0;

	return __super::StopStreaming();
}

HRESULT CRealAudioDecoder::EndOfStream()
{
	return __super::EndOfStream();
}

HRESULT CRealAudioDecoder::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CRealAudioDecoder::EndFlush()
{
	m_size = 0;

	return __super::EndFlush();
}

HRESULT CRealAudioDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_start = tStart;
	m_size = 0;
	m_buffstart = 0;

	return __super::NewSegment(tStart, tStop, dRate);
}