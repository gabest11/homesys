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
#include "OggSplitter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// bitstream

class bitstream
{
	const BYTE* m_buff;
	int m_len, m_pos;

public:
	bitstream(const BYTE* buff, int len, bool rev = false) 
		: m_buff(buff)
		, m_len(len * 8) 
	{
		m_pos = !rev ? 0 : m_len;
	}

	bool hasbits(int cnt)
	{
		int pos = m_pos + cnt;

		return pos >= 0 && pos < m_len;
	}

	DWORD showbits(int cnt) // a bit unclean, but works and can read backwards too! :P
	{
		if(!hasbits(cnt)) {ASSERT(0); return 0;}

		DWORD ret = 0, off = 0;

		const BYTE* p = m_buff;

		if(cnt < 0)
		{
			p += (m_pos + cnt) >> 3;
			off = (m_pos + cnt) & 7;
			cnt = abs(cnt);
			ret = (*p++ & (~0 << off)) >> off; 
			off = 8 - off; 
			cnt -= off;
		}
		else
		{
			p += m_pos >> 3;
			off = m_pos & 7;
			ret = (*p++ >> off) & ((1 << min(cnt, 8)) - 1); 
			off = 0; 
			cnt -= 8 - off;
		}
		
		while(cnt > 0) 
		{
			ret |= (*p++ & ((1 << min(cnt, 8)) - 1)) << off; 
			off += 8; 
			cnt -= 8;
		}

		return ret;
	}

	DWORD getbits(int cnt)
	{
		DWORD ret = showbits(cnt);

		m_pos += cnt;

		return ret;
	}
};

// COggSplitterFilter

COggSplitterFilter::COggSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("COggSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
{
}

COggSplitterFilter::~COggSplitterFilter()
{
	delete m_file;
}

HRESULT COggSplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	delete m_file;

	m_file = new COggFile(pAsyncReader, hr);

	if(FAILED(hr)) 
	{
		delete m_file;

		m_file = NULL;
		
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = 0;

	m_rtDuration = 0;

	m_file->Seek(0);
	
	OggPage page;

	for(int i = 0, nWaitForMore = 0; m_file->Read(page); i++)
	{
		BYTE* p = &page.m_buff[0];

		if((page.m_hdr.header_type_flag & OggPageHeader::continued) == 0)
		{
			BYTE type = *p++;

			std::wstring name = Util::Format(L"Stream %d", i);

			HRESULT hr;

			if(type == 1 && (page.m_hdr.header_type_flag & OggPageHeader::first))
			{
				CBaseSplitterOutputPin* pin = NULL;

				if(memcmp(p, "vorbis", 6) == 0)
				{
					name = Util::Format(L"Vorbis %d", i);

					pin = new COggVorbisOutputPin((OggVorbisIdHeader*)(p + 6), name.c_str(), this, this, &hr);

					nWaitForMore++;
				}
				else if(memcmp(p, "video", 5) == 0)
				{
					name = Util::Format(L"Video %d", i);

					pin = new COggVideoOutputPin((OggStreamHeader*)p, name.c_str(), this, this, &hr);
				}
				else if(memcmp(p, "audio", 5) == 0)
				{
					name = Util::Format(L"Audio %d", i);

					pin = new COggAudioOutputPin((OggStreamHeader*)p, name.c_str(), this, this, &hr);
				}
				else if(memcmp(p, "text", 4) == 0)
				{
					name = Util::Format(L"Text %d", i);

					pin = new COggTextOutputPin((OggStreamHeader*)p, name.c_str(), this, this, &hr);
				}
				else if(memcmp(p, "Direct Show Samples embedded in Ogg", 35) == 0)
				{
					name = Util::Format(L"DirectShow %d", i);

					pin = new COggDirectShowOutputPin((AM_MEDIA_TYPE*)(p + 35 + sizeof(GUID)), name.c_str(), this, this, &hr);
				}

				if(pin != NULL)
				{
					AddOutputPin(page.m_hdr.bitstream_serial_number, pin);
				}
			}
			else if(type == 3 && memcmp(p, "vorbis", 6) == 0)
			{
				if(COggSplitterOutputPin* pin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number)))
				{
					pin->AddComment(p + 6, page.m_buff.size() - 6 - 1);
				}
			}
			else if(!(type & 1) && nWaitForMore == 0)
			{
				break;
			}
		}

		if(COggVorbisOutputPin* pin = dynamic_cast<COggVorbisOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number)))
		{
			pin->UnpackInitPage(page);

			if(pin->IsInitialized()) 
			{
				nWaitForMore--;
			}
		}
	}

	if(m_pOutputs.empty())
	{
		return E_FAIL;
	}

	m_file->Seek(max(m_file->GetLength() - 65536, 0));

	while(m_file->Read(page))
	{
		COggSplitterOutputPin* pin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));

		if(pin != NULL && page.m_hdr.granule_position >= 0) 	
		{
			REFERENCE_TIME rt = pin->GetRefTime(page.m_hdr.granule_position);

			m_rtDuration = max(rt, m_rtDuration);
		}
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration;

	// comments

	{
		std::map<std::wstring, std::wstring> tagmap;

		tagmap[L"TITLE"] = L"TITL";
		tagmap[L"ARTIST"] = L"AUTH"; // not quite
		tagmap[L"COPYRIGHT"] = L"CPYR";
		tagmap[L"DESCRIPTION"] = L"DESC";

		for(auto i = tagmap.begin(); i != tagmap.end(); i++)
		{
			const std::wstring& oggtag = i->first;
			const std::wstring& dsmtag = i->second;

			for(auto i = m_pOutputs.begin(); i != m_pOutputs.end(); i++)
			{
				COggSplitterOutputPin* pin = dynamic_cast<COggSplitterOutputPin*>(*i);

				if(pin != NULL)
				{
					std::wstring value = pin->GetComment(oggtag.c_str());

					if(!value.empty())
					{
						SetProperty(dsmtag.c_str(), value.c_str());

						break;
					}
				}
			}
		}

		for(auto i = m_pOutputs.begin(); i != m_pOutputs.end() && !ChapGetCount(); i++)
		{
			COggSplitterOutputPin* pin = dynamic_cast<COggSplitterOutputPin*>(*i);

			if(pin != NULL)
			{
				for(int i = 1; pin != NULL; i++)
				{
					std::wstring time = pin->GetComment(Util::Format(L"CHAPTER%02d", i).c_str());

					if(time.empty()) 
					{
						break;
					}

					std::wstring name = pin->GetComment(Util::Format(L"CHAPTER%02dNAME", i).c_str());

					if(name.empty()) 
					{
						name = Util::Format(L"Chapter %d", i);
					}

					int h, m, s, ms;

					WCHAR c;

					if(7 != swscanf(time.c_str(), L"%d%c%d%c%d%c%d", &h, &c, &m, &c, &s, &c, &ms)) 
					{
						break;
					}

					ChapAppend(((((REFERENCE_TIME)h * 60 + m) * 60 + s) * 1000 + ms) * 10000, name.c_str());
				}

			}
		}
	}

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool COggSplitterFilter::DemuxInit()
{
	return m_file != NULL;
}

void COggSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if(rt <= 0)
	{
		m_file->Seek(0);
	}
	else if(m_rtDuration > 0)
	{
		// oh, the horror...

		__int64 len = m_file->GetLength();
		__int64 startpos = len * rt / m_rtDuration;
		__int64 diff = 0;

		REFERENCE_TIME rtMinDiff = _I64_MAX;

		while(1)
		{
			__int64 endpos = startpos;
			REFERENCE_TIME rtPos = -1;

			m_file->Seek(startpos);

			OggPage page;

			while(m_file->Read(page, false))
			{
				if(page.m_hdr.granule_position == -1) continue;

				COggSplitterOutputPin* pin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));

				if(pin != NULL)
				{
					rtPos = pin->GetRefTime(page.m_hdr.granule_position);
					endpos = m_file->GetPos();

					break;
				}
			}

			__int64 rtDiff = rtPos - rt;

			if(rtDiff < 0)
			{
				rtDiff = -rtDiff;

				if(rtDiff < 1000000 || rtDiff >= rtMinDiff)
				{
					m_file->Seek(startpos);

					break;
				}

				rtMinDiff = rtDiff;
			}

			__int64 newpos = startpos;

			if(rtPos < rt && rtPos < m_rtDuration)
			{
				newpos = startpos + (__int64)((1.0*(rt - rtPos) / (m_rtDuration - rtPos)) * (len - startpos)) + 1024;

	            if(newpos < endpos) 
				{
					newpos = endpos + 1024;
				}
			}
			else if(rtPos > rt && rtPos > 0)
			{
				newpos = startpos - (__int64)((1.0*(rtPos - rt) / (rtPos - 0)) * (startpos - 0)) - 1024;

	            if(newpos >= startpos)
				{
					newpos = startpos - 1024;
				}
			}
			else if(rtPos == rt)
			{
				m_file->Seek(startpos);

				break;
			}
			else
			{
				m_file->Seek(0);

				break;
			}

			diff = newpos - startpos;

			startpos = max(min(newpos, len), 0);
		}

		m_file->Seek(startpos);

		for(auto i = m_pOutputs.begin(); i != m_pOutputs.end(); i++)
		{
			COggVideoOutputPin* pin = dynamic_cast<COggVideoOutputPin*>(*i);

			bool fKeyFrameFound = false;
			bool fSkipKeyFrame = true;

			__int64 endpos = _I64_MAX;

			while(!(fKeyFrameFound && !fSkipKeyFrame) && startpos > 0)
			{
				OggPage page;

				while(!(fKeyFrameFound && !fSkipKeyFrame) && m_file->GetPos() < endpos && m_file->Read(page))
				{
					if(page.m_hdr.granule_position == -1)
					{
						continue;
					}

					if(pin != dynamic_cast<COggVideoOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number)))
					{
						continue;
					}

					if(pin->GetRefTime(page.m_hdr.granule_position) > rt)
					{
						break;
					}

					if(!fKeyFrameFound)
					{
						pin->UnpackPage(page);

						while(OggPacket* p = pin->GetPacket())
						{
							if(p->flags & CPacket::SyncPoint)
							{
								fKeyFrameFound = true;
								fSkipKeyFrame = p->skip;
							}

							delete p;
						}

						if(fKeyFrameFound) 
						{
							break;
						}
					}
					else
					{
						pin->UnpackPage(page);

						while(OggPacket* p = pin->GetPacket())
						{
							bool skip = p->skip;

							delete p;

							if(!skip)
							{
								fSkipKeyFrame = false;

								break;
							}
						}
					}
				}

				if(!(fKeyFrameFound && !fSkipKeyFrame)) 
				{
					endpos = startpos; 
					startpos = max(startpos - 10 * 65536, 0);
				}

				m_file->Seek(startpos);
			}

#ifdef DEBUG
			// verify kf

			{
				fKeyFrameFound = false;

				OggPage page;

				while(!fKeyFrameFound && m_file->Read(page))
				{
					if(page.m_hdr.granule_position == -1)
					{
						continue;
					}

					if(pin != dynamic_cast<COggVideoOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number)))
					{
						continue;
					}

					REFERENCE_TIME rtPos = pin->GetRefTime(page.m_hdr.granule_position);

					if(rtPos > rt)
					{
						break;
					}

					pin->UnpackPage(page);

					while(OggPacket* p = pin->GetPacket())
					{
						bool syncpoint = (p->flags & CPacket::SyncPoint) != 0;

						delete p;

						if(syncpoint)
						{
							fKeyFrameFound = true;

							break;
						}
					}
				}

				ASSERT(fKeyFrameFound);

				m_file->Seek(startpos);
			}
#endif
			break;
		}
	}
}

bool COggSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	OggPage page;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_file->Read(page, true, GetRequestHandle()))
	{
		COggSplitterOutputPin* pin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));

		if(pin == NULL || !pin->IsConnected())
		{
			continue;
		}

		hr = pin->UnpackPage(page);

		if(FAILED(hr)) 
		{
			break;
		}

		OggPacket* p = NULL;

		while(!CheckRequest(NULL) && SUCCEEDED(hr) && (p = pin->GetPacket()))
		{
			if(!p->skip)
			{
				hr = DeliverPacket(p);
			}
			else
			{
				delete p;
			}
		}
	}

	return true;
}

// COggSourceFilter

COggSourceFilter::COggSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: COggSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}

// COggSplitterOutputPin

COggSplitterOutputPin::COggSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(pName, pFilter, pLock, phr)
	, m_lastpacket(NULL)
{
	ResetState(-1);
}

COggSplitterOutputPin::~COggSplitterOutputPin()
{
	ResetState(-1);
}


void COggSplitterOutputPin::AddComment(BYTE* p, int len)
{
	bitstream bs(p, len);

	bs.getbits(bs.getbits(32) * 8);

	for(int n = bs.getbits(32); n-- > 0; )
	{
		std::string str;

		for(int len = bs.getbits(32); len-- > 0; )
		{
			str += (char)bs.getbits(8);
		}

		std::list<std::string> sl;

		Util::Explode(str, sl, '=', 2);

		if(sl.size() == 2)
		{
			std::wstring key = Util::UTF8To16(sl.front().c_str());
			std::wstring value = Util::UTF8To16(sl.back().c_str());

			Comment* p = new Comment(key.c_str(), value.c_str());

			if(p->m_key == L"LANGUAGE")
			{
				std::wstring lang = Util::ISO6392ToLanguage(value.c_str());
				std::wstring code = Util::LanguageToISO6392(value.c_str());

				if(value.size() == 3 && !lang.empty())
				{
					SetName(lang.c_str());
					SetProperty(L"LANG", value.c_str());
				}
				else if(!code.empty())
				{
					SetName(value.c_str());
					SetProperty(L"LANG", code.c_str());
				}
				else
				{
					SetName(value.c_str());
					SetProperty(L"NAME", value.c_str());
				}
			}

			m_comments.push_back(p);
		}
	}

	ASSERT(bs.getbits(1) == 1);
}

std::wstring COggSplitterOutputPin::GetComment(LPCWSTR key)
{
	std::wstring s = Util::MakeUpper(key);

	std::list<std::wstring> sl;

	for(auto i = m_comments.begin(); i != m_comments.end(); i++)
	{
		if(s == (*i)->m_key)
		{
			sl.push_back((*i)->m_value);
		}
	}

	return Util::Implode(sl, ';');
}

void COggSplitterOutputPin::ResetState(DWORD seqnum)
{
	CAutoLock csAutoLock(&m_csPackets);

	for(auto i = m_packets.begin(); i != m_packets.end(); i++) 
	{
		delete *i;
	}

	m_packets.clear();

	delete m_lastpacket;

	m_lastpacket = NULL;

	m_lastseqnum = seqnum;

	m_rtLast = 0;

	m_skip = true;
}

HRESULT COggSplitterOutputPin::UnpackPage(OggPage& page)
{
	if(m_lastseqnum != page.m_hdr.page_sequence_number - 1)
	{
		ResetState(page.m_hdr.page_sequence_number);

		return S_FALSE; // FIXME
	}
	else
	{
		m_lastseqnum = page.m_hdr.page_sequence_number;
	}

	std::vector<int> lens(page.m_lens.begin(), page.m_lens.end());

	int first = 0;
	while(first < lens.size() && lens[first] == 255)  first++;
	if(first == lens.size()) first--;

	int last = lens.size() - 1;
	while(last >= 0 && lens[last] == 255) last--;
	if(last < 0) last++;

	const BYTE* data = &page.m_buff[0];

	int start = 0, end = 0;

	for(int i = 0; i < lens.size(); i++)
	{
		int len = lens[i];

		end += len;

		if(len == 255 && i < lens.size() - 1)
		{
			continue;
		}

		if(i == first && (page.m_hdr.header_type_flag & OggPageHeader::continued))
		{
			if(m_lastpacket != NULL)
			{
				int size = m_lastpacket->buff.size();

				if(end > start)
				{
					m_lastpacket->buff.resize(size + end - start);
				
					memcpy(&m_lastpacket->buff[size], &data[start], end - start);
				}

				if(len < 255) 
				{
					CAutoLock csAutoLock(&m_csPackets);

					m_packets.push_back(m_lastpacket);

					m_lastpacket = NULL;
				}
			}
		}
		else
		{
			OggPacket* p = new OggPacket();

			if(i == last && page.m_hdr.granule_position != -1)
			{
				if(m_skip)
				{
					p->flags |= CPacket::Discontinuity;
				}

				REFERENCE_TIME rtLast = m_rtLast;

				m_rtLast = GetRefTime(page.m_hdr.granule_position);

				// some bad encodings have a +/-1 frame difference from the normal timeline, 
				// but these seem to cancel eachother out nicely so we can just ignore them 
				// to make it play a bit more smooth.

				if(abs(rtLast - m_rtLast) == GetRefTime(1)) 
				{
					m_rtLast = rtLast; // FIXME
				}

				m_skip = false;
			}

			p->id = page.m_hdr.bitstream_serial_number;

			if(S_OK == UnpackPacket(p, &data[start], end - start))
			{
				if(p->start <= p->stop && p->stop <= p->start + 10000000i64*60)
				{
					CAutoLock csAutoLock(&m_csPackets);

					m_rtLast = p->stop;

					p->skip = m_skip;

					if(len < 255)
					{
						m_packets.push_back(p);
					}
					else
					{
						ASSERT(m_lastpacket == NULL);

						m_lastpacket = p;
					}
				}
				else
				{
					ASSERT(0);
				}
			}
		}

		start = end;
	}

	return S_OK;
}

OggPacket* COggSplitterOutputPin::GetPacket()
{
	CAutoLock csAutoLock(&m_csPackets);
	
	OggPacket* p = NULL;
	
	if(!m_packets.empty()) 
	{
		p = m_packets.front();

		m_packets.pop_front();
	}

	return p;
}

HRESULT COggSplitterOutputPin::DeliverEndFlush()
{
	ResetState();

	return __super::DeliverEndFlush();
}

HRESULT COggSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	ResetState();

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

COggSplitterOutputPin::Comment::Comment(LPCWSTR key, LPCWSTR value)
{
	m_key = Util::MakeUpper(key);
	m_value = value;
}

// COggVorbisOutputPin

COggVorbisOutputPin::COggVorbisOutputPin(OggVorbisIdHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	m_audio_sample_rate = h->audio_sample_rate;
	
	m_blocksize[0] = 1 << h->blocksize_0;
	m_blocksize[1] = 1 << h->blocksize_1;

	m_lastblocksize = 0;

	CMediaType mt;

	//

	mt.InitMediaType();

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_Vorbis2;
	mt.formattype = FORMAT_VorbisFormat2;

	VORBISFORMAT2* vf2 = (VORBISFORMAT2*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT2));

	memset(mt.Format(), 0, mt.FormatLength());

	vf2->Channels = h->audio_channels;
	vf2->SamplesPerSec = h->audio_sample_rate;

	mt.SetSampleSize(8192);

	m_mts.push_back(mt);

	//

	mt.InitMediaType();

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_Vorbis;
	mt.formattype = FORMAT_VorbisFormat;

	VORBISFORMAT* vf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));

	memset(mt.Format(), 0, mt.FormatLength());

	vf->nChannels = h->audio_channels;
	vf->nSamplesPerSec = h->audio_sample_rate;
	vf->nAvgBitsPerSec = h->bitrate_nominal;
	vf->nMinBitsPerSec = h->bitrate_minimum;
	vf->nMaxBitsPerSec = h->bitrate_maximum;
	vf->fQuality = -1;

	mt.SetSampleSize(8192);

	m_mts.push_back(mt);
}

HRESULT COggVorbisOutputPin::UnpackInitPage(OggPage& page)
{
	HRESULT hr = __super::UnpackPage(page);

	while(!m_packets.empty())
	{
		CPacket* p = m_packets.front();

		if(p->buff.size() >= 6 && p->buff[0] == 0x05)
		{
			// yeah, right, we are going to be parsing this backwards! :P

			bitstream bs(&p->buff[0], p->buff.size(), true);

			while(bs.hasbits(-1) && bs.getbits(-1) != 1);

			for(int cnt = 0; bs.hasbits(-8-16-16-1-6); cnt++)
			{
				DWORD modes = bs.showbits(-6) + 1;
				DWORD mapping = bs.getbits(-8);
				DWORD transformtype = bs.getbits(-16);
				DWORD windowtype = bs.getbits(-16);
				DWORD blockflag = bs.getbits(-1);

				if(transformtype != 0 || windowtype != 0)
				{
					ASSERT(modes == cnt);

					break;
				}

				m_blockflags.insert(m_blockflags.begin(), blockflag ? 1 : 0);
			}
		}

		int cnt = m_initpackets.size();

		if(cnt <= 3)
		{
			ASSERT(p->buff.size() >= 6 && p->buff[0] == 1 + cnt * 2);

			VORBISFORMAT2* vf2 = (VORBISFORMAT2*)m_mts[0].pbFormat;

			vf2->HeaderSize[cnt] = p->buff.size();

			int len = m_mts[0].FormatLength();

			memcpy(m_mts[0].ReallocFormatBuffer(len + p->buff.size()) + len, &p->buff[0], p->buff.size());
		}

		m_initpackets.push_back(m_packets.front());

		m_packets.pop_front();
	}

	return hr;
}

REFERENCE_TIME COggVorbisOutputPin::GetRefTime(__int64 granule_position)
{
	return granule_position * 10000000 / m_audio_sample_rate;
}

HRESULT COggVorbisOutputPin::UnpackPacket(OggPacket* p, const BYTE* data, int len)
{
	if(len > 0 && m_blockflags.size())
	{
		bitstream bs(data, len);

		if(bs.getbits(1) == 0)
		{
			int n = 0;
			int x = m_blockflags.size() - 1;

			while(x) 
			{
				n++; 
				x >>= 1;
			}

			DWORD blocksize = m_blocksize[m_blockflags[bs.getbits(n)] ? 1 : 0];

			if(m_lastblocksize) 
			{
				m_rtLast += GetRefTime((m_lastblocksize + blocksize) >> 2);
			}

			m_lastblocksize = blocksize;
		}
	}

	p->flags |= CPacket::SyncPoint | CPacket::TimeValid;
	
	p->start = m_rtLast;
	p->stop = m_rtLast + 1;

	p->SetData(data, len);

	return S_OK;
}

HRESULT COggVorbisOutputPin::DeliverPacket(OggPacket* p)
{
	return p->buff.empty() || (p->buff[0] & 1) == 0 ? __super::DeliverPacket(p) : S_OK;
}

HRESULT COggVorbisOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	HRESULT hr;
	
	hr = __super::DeliverNewSegment(tStart, tStop, dRate);

	m_lastblocksize = 0;

	if(m_mt.subtype == MEDIASUBTYPE_Vorbis)
	{
		for(auto i = m_initpackets.begin(); i != m_initpackets.end(); i++)
		{
			OggPacket* p = new OggPacket();
			
			p->id = (*i)->id;

			p->start = 0;
			p->stop = 0;

			p->flags |= CPacket::TimeValid; 
			
			if((p->flags & CPacket::SyncPoint) == 0) 
			{
				p->flags |= CPacket::Discontinuity; 
			}

			p->buff = (*i)->buff;

			__super::DeliverPacket(p);
		}
	}

	return hr;
}

// COggDirectShowOutputPin

COggDirectShowOutputPin::COggDirectShowOutputPin(AM_MEDIA_TYPE* pmt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	CMediaType mt;

	memcpy((AM_MEDIA_TYPE*)&mt, pmt, FIELD_OFFSET(AM_MEDIA_TYPE, pUnk));

	mt.SetFormat((BYTE*)(pmt + 1), pmt->cbFormat);

	mt.SetSampleSize(1);

	if(mt.majortype == MEDIATYPE_Video) // TODO: find samples for audio and find out what to return in GetRefTime...
	{
		m_mts.push_back(mt);
	}
}

REFERENCE_TIME COggDirectShowOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = 0;

	if(m_mt.majortype == MEDIATYPE_Video)
	{
		rt = granule_position * ((VIDEOINFOHEADER*)m_mt.Format())->AvgTimePerFrame;
	}
	else if(m_mt.majortype == MEDIATYPE_Audio)
	{
		rt = granule_position; // ((WAVEFORMATEX*)m_mt.Format())-> // TODO
	}

	return rt;
}

HRESULT COggDirectShowOutputPin::UnpackPacket(OggPacket* p, const BYTE* data, int len)
{
	int i = 0;

	BYTE hdr = data[i++];

	if((hdr & 1) == 0)
	{
		// TODO: verify if this was still present in the old format (haven't found one sample yet)

		BYTE bytes = (hdr >> 6) | ((hdr & 2) << 1);

		__int64 total = 0;

		for(int j = 0; j < bytes; j++)
		{
			total |= (__int64)data[i++] << (j << 3);
		}

		if(len < i) 
		{
			ASSERT(0); 

			return E_FAIL;
		}

		p->start = m_rtLast;
		p->stop = m_rtLast + (bytes > 0 ? GetRefTime(total) : GetRefTime(1));

		if(hdr & 8)
		{
			p->flags |= CPacket::SyncPoint;
		}

		p->flags |= CPacket::TimeValid;

		p->SetData(&data[i], len - i);

		return S_OK;
	}

	return S_FALSE;
}

// COggStreamOutputPin

COggStreamOutputPin::COggStreamOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	m_time_unit = h->time_unit;
	m_samples_per_unit = h->samples_per_unit;
	m_default_len = h->default_len;
}

REFERENCE_TIME COggStreamOutputPin::GetRefTime(__int64 granule_position)
{
	return granule_position * m_time_unit / m_samples_per_unit;
}

HRESULT COggStreamOutputPin::UnpackPacket(OggPacket* p, const BYTE* data, int len)
{
	int i = 0;

	BYTE hdr = data[i++];

	if((hdr & 1) == 0)
	{
		BYTE bytes = (hdr >> 6) | ((hdr & 2) << 1);

		__int64 total = 0;

		for(int j = 0; j < bytes; j++)
		{
			total |= (__int64)data[i++] << (j << 3);
		}

		if(len < i) 
		{
			ASSERT(0); 

			return E_FAIL;
		}

		p->start = m_rtLast;
		p->stop = m_rtLast + (bytes > 0 ? GetRefTime(total) : GetRefTime(m_default_len));

		if(hdr & 8)
		{
			p->flags |= CPacket::SyncPoint;
		}

		p->flags |= CPacket::TimeValid;

		p->SetData(&data[i], len - i);

		return S_OK;
	}

	return S_FALSE;
}

// COggVideoOutputPin

COggVideoOutputPin::COggVideoOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	int extra = (int)h->size - sizeof(OggStreamHeader);

	extra = max(extra, 0);	

	CMediaType mt;

	mt.majortype = MEDIATYPE_Video;
	mt.subtype = FOURCCMap(MAKEFOURCC(h->subtype[0], h->subtype[1], h->subtype[2], h->subtype[3]));
	mt.formattype = FORMAT_VideoInfo;

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + extra);

	memset(mt.Format(), 0, mt.FormatLength());
	memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), h + 1, extra);

	vih->AvgTimePerFrame = h->time_unit / h->samples_per_unit;
	vih->bmiHeader.biWidth = h->v.w;
	vih->bmiHeader.biHeight = h->v.h;
	vih->bmiHeader.biBitCount = (WORD)h->bps;
	vih->bmiHeader.biCompression = mt.subtype.Data1;

	switch(vih->bmiHeader.biCompression)
	{
	case BI_RGB: 
	case BI_BITFIELDS: 
		mt.subtype = 
			vih->bmiHeader.biBitCount == 1 ? MEDIASUBTYPE_RGB1 :
			vih->bmiHeader.biBitCount == 4 ? MEDIASUBTYPE_RGB4 :
			vih->bmiHeader.biBitCount == 8 ? MEDIASUBTYPE_RGB8 :
			vih->bmiHeader.biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
			vih->bmiHeader.biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
			vih->bmiHeader.biBitCount == 32 ? MEDIASUBTYPE_RGB32 :
			MEDIASUBTYPE_NULL;
		break;
	case BI_RLE8: 
		mt.subtype = MEDIASUBTYPE_RGB8; 
		break;
	case BI_RLE4: 
		mt.subtype = MEDIASUBTYPE_RGB4; 
		break;
	}

	mt.SetSampleSize(max(h->buffersize, 1));
	
	m_mts.push_back(mt);
}

// COggAudioOutputPin

COggAudioOutputPin::COggAudioOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	int extra = (int)h->size - sizeof(OggStreamHeader);

	extra = max(extra, 0);	

	CMediaType mt;

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = FOURCCMap(strtol(CStringA(h->subtype, 4), NULL, 16));
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + extra);

	memset(mt.Format(), 0, mt.FormatLength());
	memcpy(mt.Format() + sizeof(WAVEFORMATEX), h + 1, extra);

	wfe->cbSize = extra;
	wfe->wFormatTag = (WORD)mt.subtype.Data1;
	wfe->nChannels = h->a.nChannels;
	wfe->nSamplesPerSec = (DWORD)(10000000i64 * h->samples_per_unit / h->time_unit);
	wfe->wBitsPerSample = (WORD)h->bps;
	wfe->nAvgBytesPerSec = h->a.nAvgBytesPerSec; // TODO: verify for PCM
	wfe->nBlockAlign = h->a.nBlockAlign; // TODO: verify for PCM
	
	mt.SetSampleSize(max(h->buffersize, 1));

	m_mts.push_back(mt);
}

// COggTextOutputPin

COggTextOutputPin::COggTextOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	CMediaType mt;

	mt.majortype = MEDIATYPE_Text;
	mt.subtype = MEDIASUBTYPE_NULL;
	mt.formattype = FORMAT_None;
	
	mt.SetSampleSize(1);

	m_mts.push_back(mt);
}

