#include "stdafx.h"
#include "StrobeSource.h"
#include "DirectShow.h"
#include "AsyncReader.h"
#include "MediaTypeEx.h"
#include <initguid.h>
#include "moreuuids.h"

// CStrobeSourceFilter

CStrobeSourceFilter::CStrobeSourceFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CStrobeSourceFilter"), lpunk, this, __uuidof(this))
	, m_current(0)
	, m_start(0)
	, m_duration(0)
	, m_file(NULL)
	, m_current_chunk(m_chunks.end())
	, m_mdat_pos(0)
{
}

CStrobeSourceFilter::~CStrobeSourceFilter()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}

	delete m_file;
}

STDMETHODIMP CStrobeSourceFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

int CStrobeSourceFilter::GetPinCount()
{
	return m_outputs.size();
}

CBasePin* CStrobeSourceFilter::GetPin(int n)
{
	return n >= 0 && n < m_outputs.size() ? m_outputs[n] : NULL;
}

STDMETHODIMP CStrobeSourceFilter::Pause()
{
	FILTER_STATE state = m_State;

	HRESULT hr = __super::Pause();

	if(state == State_Stopped)
	{
		m_start = 0;

		m_samples.clear();

		m_current_chunk = m_chunks.end();

		if(!m_chunks.empty())
		{
			m_current_chunk = m_chunks.begin();
		}

		CAMThread::Create();

		CAMThread::CallWorker(CMD_START);
	}

	return hr;
}

STDMETHODIMP CStrobeSourceFilter::Stop()
{
	if(m_State != State_Stopped)
	{
		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverBeginFlush();
		}

		CAMThread::CallWorker(CMD_EXIT);

		CAMThread::Close();

		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverEndFlush();
		}
		
		m_samples.clear();
	}

	return __super::Stop();
}

DWORD CStrobeSourceFilter::ThreadProc()
{
	std::list<CStrobeSourceOutputPin*> pins;

	while(1)
	{
		DWORD cmd = GetRequest();

		if(cmd == CMD_EXIT)
		{
			Reply(S_OK);

			break;
		}

		switch(cmd)
		{
		case CMD_FLUSH:

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				(*i)->DeliverEndFlush();
			}

			Reply(S_OK);

			break;

		case CMD_START:

			Reply(S_OK);

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				(*i)->DeliverNewSegment(m_start, m_duration, 1.0f);
			}

			pins.clear();
			pins.insert(pins.end(), m_outputs.begin(), m_outputs.end());

			while(!CheckRequest(NULL) && QueueNextSample());

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				(*i)->DeliverEndOfStream();
			}

			break;
		}
	}

	return 0;
}

bool CStrobeSourceFilter::SeekToNextSample()
{
	if(m_samples.empty())
	{
		if(m_current_chunk == m_chunks.end())
		{
			return false;
		}

		Chunk& c = *m_current_chunk++;

		REFERENCE_TIME start = c.start;

		delete m_file; m_file = NULL;

		std::string url = m_baseurl + Util::Format("Seg%d-Frag%d", c.segment, c.fragment);

		CHttpStream* s = new CHttpStream();

		if(!s->Load(url.c_str())) {delete s; return false;}

		HRESULT hr;

		m_file = new CBaseSplitterFile(new CAsyncHttpStreamReader(s), hr);

		ParseMOOF(*m_file, start, m_track_id, m_samples);

		// remove elements from m_samples until the first syncpoint before m_start
		
		for(auto i = m_samples.begin(), j = m_samples.begin(); i != m_samples.end(); i++)
		{
			if(i->start >= m_start)
			{
				if(i != m_samples.begin() && j != m_samples.begin())
				{
					m_samples.erase(m_samples.begin(), j);
				}

				break;
			}

			if(i->syncpoint)
			{
				j = i;
			}
		}
	}

	return true;
}

bool CStrobeSourceFilter::QueueNextSample()
{
	if(!SeekToNextSample() || m_samples.empty())
	{
		return false;
	}

	Sample s = m_samples.front();
		
	m_samples.pop_front();

	m_file->Seek(s.pos);

	BYTE Reserved = (BYTE)m_file->BitRead(2);
	BYTE Filter = (BYTE)m_file->BitRead();
	BYTE TagType = (BYTE)m_file->BitRead(5);

	if(Filter) return true; // encrypted?

	CStrobeSourceOutputPin* pin = NULL;

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		const CMediaType& mt = (*i)->CurrentMediaType();

		if(TagType == 9 && mt.majortype == MEDIATYPE_Video
		|| TagType == 8 && mt.majortype == MEDIATYPE_Audio)
		{
			pin = *i;

			break;
		}
	}

	if(pin == NULL)
	{
		return true; // skip unknown type
	}

	DWORD DataSize = (DWORD)m_file->BitRead(24);
	DWORD TimeStamp = (DWORD)m_file->BitRead(24) | ((DWORD)m_file->BitRead(8) << 24);
	DWORD StreamId = (DWORD)m_file->BitRead(24);

	bool syncpoint = true;

	if(TagType == 9)
	{
		CBaseSplitterFile::FLVVideoPacket h;

		if(!m_file->Read(h, DataSize))
		{
			return false;
		}

		syncpoint = h.type == CBaseSplitterFile::FLVVideoPacket::Intra;

		DataSize -= 1;

		if(h.codec == CBaseSplitterFile::FLVVideoPacket::AVC)
		{
			if((BYTE)m_file->BitRead(8) != 1)
			{
				return true;
			}

			m_file->BitRead(24);

			DataSize -= 4;
		}
	}
	else if(TagType == 8)
	{
		CBaseSplitterFile::FLVAudioPacket h;

		if(!m_file->Read(h, DataSize))
		{
			return false;
		}

		DataSize -= 1;

		if(h.format == CBaseSplitterFile::FLVAudioPacket::AAC)
		{
			if((BYTE)m_file->BitRead(8) == 0)
			{
				return true;
			}

			DataSize -= 1;
		}
	}

	if(Filter == 1)
	{
		// TODO
	}

	__int64 pos = m_file->GetPos();

	QueueItem* item = new QueueItem();

	item->sample = s;
	item->sample.syncpoint = syncpoint;

	item->buff.resize(DataSize);
		
	m_file->ByteRead(item->buff.data(), item->buff.size());

	m_current = s.start;

	//printf("[%p] s %I64d\n", pin, item->sample.start / 10000);

	return pin->Deliver(item) == S_OK;
}

HRESULT CStrobeSourceFilter::Load(Util::Socket& s, LPCSTR url, int bitrate)
{
	std::map<std::string, std::string> headers;

	if(!s.HttpGet(url, headers))
	{
		return E_FAIL;
	}

	std::string type = s.GetContentType();
	
	if(type != "text/xml" && type != "application/xml" && type != "application/f4m+xml" || s.GetContentLength() == 0)
	{
		return E_FAIL;
	}

	std::vector<BYTE> buff;

	buff.reserve(s.GetContentLength());

	if(!s.ReadToEnd(buff, s.GetContentLength()) || buff.empty())
	{
		return E_FAIL;
	}

	std::string str;

	if(buff.size() > 2 && buff[0] == 0xff && buff[1] == 0xfe)
	{
		str = Util::UTF16To8(std::wstring((LPCWSTR)buff.data() + 1, buff.size() / 2 - 1).c_str());
	}
	else if(wcsstr((LPCWSTR)buff.data(), L"utf-16") != NULL || wcsstr((LPCWSTR)buff.data(), L"UTF-16") != NULL)
	{
		str = Util::UTF16To8(std::wstring((LPCWSTR)buff.data(), buff.size() / 2).c_str());
	}
	else
	{
		str = std::string((LPCSTR)buff.data(), buff.size());
	}

	TiXmlDocument doc;

	doc.Parse(str.c_str(), NULL, TIXML_ENCODING_UTF8);

	if(TiXmlElement* servers = doc.FirstChildElement("servers"))
	{
		for(TiXmlElement* n = servers->FirstChildElement("url"); 
			n != NULL; 
			n = n->NextSiblingElement("url"))
		{
			m_baseurl = n->GetText();

			if(SUCCEEDED(Load(s, n->GetText())))
			{
				m_baseurl = m_baseurl.substr(0, m_baseurl.find('?'));
				m_baseurl = m_baseurl.substr(0, m_baseurl.find_last_of('/'));

				return S_OK;
			}
		}
	}

	if(TiXmlElement* manifest = doc.FirstChildElement("manifest")) // FIXME
	{
		std::string xmlns;

		if(manifest->QueryValueAttribute("xmlns", &xmlns) == TIXML_SUCCESS && xmlns == "http://ns.adobe.com/f4m/2.0") 
		{
			for(TiXmlElement* n = manifest->FirstChildElement("media"); 
				n != NULL; 
				n = n->NextSiblingElement("media"))
			{
				std::string href;
				int bitrate;

				if(n->QueryValueAttribute("href", &href) == TIXML_SUCCESS
				&& n->QueryValueAttribute("bitrate", &bitrate) == TIXML_SUCCESS) 
				{
					if(SUCCEEDED(Load(s, href.c_str(), bitrate)))
					{
						m_baseurl = href;

						return S_OK;
					}
				}
			}
		}
	}

	if(!m_manifest.Parse(doc.FirstChildElement("manifest"), bitrate))
	{
		return E_FAIL;
	}

	m_duration = (REFERENCE_TIME)(10000000.0 * m_manifest.m_duration);

	return S_OK;
}

// TODO: move these to CBaseSplitterFile

bool CStrobeSourceFilter::ParseAFRA(CBaseSplitterFile& f, AFRA& afra)
{
	afra = AFRA();

	BYTE LongIDs = (BYTE)f.BitRead();
	BYTE LongOffsets = (BYTE)f.BitRead();
	BYTE GlobalEntries = (BYTE)f.BitRead();

	f.BitRead(5); // Reserved

	afra.TimeScale = (DWORD)f.BitRead(32);

	for(DWORD i = 0, n = (DWORD)f.BitRead(32); i < n; i++)
	{
		AFRA::Entry e;

		e.Time = (UINT64)f.BitRead(64);
		e.Offset = (UINT64)f.BitRead(LongOffsets ? 64 : 32);

		afra.Entries.push_back(e);
	}

	if(GlobalEntries)
	{
		for(DWORD i = 0, n = (DWORD)f.BitRead(32); i < n; i++)
		{
			AFRA::GlobalEntry e;

			e.Time = (UINT64)f.BitRead(64);
			e.Segment = (DWORD)f.BitRead(LongIDs ? 32 : 16);
			e.Fragment = (DWORD)f.BitRead(LongIDs ? 32 : 16);
			e.AfraOffset = (UINT64)f.BitRead(LongOffsets ? 64 : 32);
			e.OffsetFromAfra = (UINT64)f.BitRead(LongOffsets ? 64 : 32);

			afra.GlobalEntries.push_back(e);
		}
	}

	return true;
}

bool CStrobeSourceFilter::ParseABST(CBaseSplitterFile& f, ABST& abst)
{
	abst = ABST();

	abst.BootstrapinfoVersion = (DWORD)f.BitRead(32);

	BYTE Profile = (BYTE)f.BitRead(2);
	BYTE Live = (BYTE)f.BitRead();
	BYTE Update = (BYTE)f.BitRead();
				
	f.BitRead(4); // Reserved

	abst.TimeScale = (DWORD)f.BitRead(32);

	abst.CurrentMediaTime = (UINT64)f.BitRead(64);
	abst.SmpteTimeCodeOffset = (UINT64)f.BitRead(64);

	while(char c = (char)f.BitRead(8)) abst.MovieIdentifier += c;

	for(BYTE i = 0, n = (BYTE)f.BitRead(8); i < n; i++)
	{
		std::string s;

		while(char c = (char)f.BitRead(8)) s += c;

		abst.ServerBaseURL.push_back(s);
	}

	for(BYTE i = 0, n = (BYTE)f.BitRead(8); i < n; i++)
	{
		std::string s;

		while(char c = (char)f.BitRead(8)) s += c;

		abst.QualitySegmentUrlModifier.push_back(s);
	}

	while(char c = (char)f.BitRead(8)) abst.DrmData += c;

	while(char c = (char)f.BitRead(8)) abst.MetaData += c;

	for(BYTE i = 0, n = (BYTE)f.BitRead(8); i < n; i++)
	{
		CBaseSplitterFile::MP4BoxEx h;

		ABST::Segment segment;

		if(!f.Read(h) || h.tag != 'asrt')
		{
			return false;
		}

		for(BYTE i = 0, n = (BYTE)f.BitRead(8); i < n; i++)
		{
			std::string s;

			while(char c = (char)f.BitRead(8)) s += c;

			segment.QualitySegmentUrlModifiers.push_back(s);
		}

		for(DWORD i = 0, n = (DWORD)f.BitRead(32); i < n; i++)
		{
			ABST::Segment::Entry e;

			e.FirstSegment = (DWORD)f.BitRead(32);
			e.FragmentsPerSegment = (DWORD)f.BitRead(32);

			segment.Entries.push_back(e);
		}

		abst.Segments.push_back(segment);

		f.Seek(h.next);
	}

	for(BYTE i = 0, n = (BYTE)f.BitRead(8); i < n; i++)
	{
		CBaseSplitterFile::MP4BoxEx h;

		if(!f.Read(h) || h.tag != 'afrt')
		{
			return false;
		}

		ABST::Fragment fragment;

		fragment.TimeScale = (DWORD)f.BitRead(32);

		for(BYTE i = 0, n = (BYTE)f.BitRead(8); i < n; i++)
		{
			std::string s;

			while(char c = (char)f.BitRead(8)) s += c;

			fragment.QualitySegmentUrlModifiers.push_back(s);
		}

		for(DWORD i = 0, n = (DWORD)f.BitRead(32); i < n; i++)
		{
			ABST::Fragment::Entry e;

			e.FirstFragment = (DWORD)f.BitRead(32);
			e.FirstFragmentTimestamp = (UINT64)f.BitRead(64);
			e.FragmentDuration = (DWORD)f.BitRead(32);

			if(e.FragmentDuration == 0)
			{
				e.DiscontinuityIndicator = (BYTE)f.BitRead(8);
			}

			if(i > 0 && i == n - 1)
			{
				if(e.FirstFragment == 0
				&& e.FirstFragmentTimestamp == 0
				&& e.FragmentDuration == 0
				&& e.DiscontinuityIndicator == 0)
				{
					// HACK

					const ABST::Fragment::Entry& e2 = fragment.Entries.back();

					e.FirstFragment = e2.FirstFragment + 1;
					e.FirstFragmentTimestamp = e2.FirstFragmentTimestamp + e2.FragmentDuration;
					e.FragmentDuration = e2.FragmentDuration;

					if(abst.CurrentMediaTime >= e.FirstFragmentTimestamp)
					{
						e.FragmentDuration = abst.CurrentMediaTime - e.FirstFragmentTimestamp;
					}
				}
			}

			fragment.Entries.push_back(e);
		}

		abst.Fragments.push_back(fragment);

		f.Seek(h.next);
	}

	return true;
}

bool CStrobeSourceFilter::ParseMOOF(CBaseSplitterFile& f, REFERENCE_TIME chunk_start, DWORD cur_track_id, std::list<Sample>& samples)
{
	samples.clear();

	DWORD tfhd_flags = 0;

	UINT64 base_data_offset = 0;
	DWORD sample_description_index = 0;
	DWORD default_sample_duration = 0;
	DWORD default_sample_size = 0;
	DWORD default_sample_flags = 0;

	DWORD trun_flags = 0;
	DWORD track_id = 0;

	int data_offset = 0;
	DWORD first_sample_flags = 0;
	UINT64 mdat_pos = 0;

	CBaseSplitterFile::MP4Box h;

	f.Seek(0);

	while(f.Read(h))
	{
		if(h.tag == 'mdat')
		{
			mdat_pos = f.GetPos();

			break;
		}

		f.Seek(h.next);
	}

	base_data_offset = mdat_pos;

	UINT64 cur_data_offset = base_data_offset;
	REFERENCE_TIME cur_start = chunk_start;

	f.Seek(0);

	while(f.Read(h))
	{
		if(h.tag == 'moof')
		{
			UINT64 moof_pos = f.GetPos();

			CBaseSplitterFile::MP4Box h2;

			while(f.Read(h2))
			{
				if(h2.tag == 'mfhd')
				{
				}
				else if(h2.tag == 'traf')
				{
					CBaseSplitterFile::MP4Box h3;

					while(f.Read(h3))
					{
						if(h3.tag == 'tfhd')
						{
							tfhd_flags = (DWORD)f.BitRead(32) & 0xffffff;

							track_id = f.BitRead(32);
//printf("tfhd %d\t%08x\n", track_id, tfhd_flags);
							if(tfhd_flags & 0x01) cur_data_offset = f.BitRead(64);
//if(tfhd_flags & 0x01) printf("tfhd cur_data_offset\t%08x\n", cur_data_offset);
							if(tfhd_flags & 0x02) sample_description_index = (DWORD)f.BitRead(32);
							if(tfhd_flags & 0x08) default_sample_duration = (DWORD)f.BitRead(32);
							if(tfhd_flags & 0x10) default_sample_size = (DWORD)f.BitRead(32);
							if(tfhd_flags & 0x20) default_sample_flags = (DWORD)f.BitRead(32);

							cur_start = chunk_start;
						}
						else if(h3.tag == 'trun' && (cur_track_id == 0 || cur_track_id == track_id))// && track_id == 3) // FIXME
						{
							trun_flags = (DWORD)f.BitRead(32) & 0xffffff;

							DWORD sample_count = (DWORD)f.BitRead(32);
//printf("trun %d\t%08x\n", sample_count, trun_flags);

							if(trun_flags & 0x01) data_offset = (int)f.BitRead(32);
//if(trun_flags & 0x01) printf("trun data_offset\t%08x %08x\n", data_offset, (int)cur_data_offset + data_offset);
							if(trun_flags & 0x04) first_sample_flags = (DWORD)f.BitRead(32);

							if((trun_flags & 0x01) && data_offset == 0) 
							{
								cur_data_offset = mdat_pos; // HACK
							}

							Sample s;
							
							s.start = cur_start;
							s.pos = cur_data_offset + data_offset; // TODO: data_offset not set

							for(DWORD i = 0; i < sample_count; i++)
							{
								DWORD sample_duration = (trun_flags & 0x0100) ? (DWORD)f.BitRead(32) : default_sample_duration;
								DWORD sample_size = (trun_flags & 0x0200) ? (DWORD)f.BitRead(32) : default_sample_size;
								DWORD sample_flags = (trun_flags & 0x0400) ? (DWORD)f.BitRead(32) : default_sample_flags;
								DWORD sample_composition_time_offset = (trun_flags & 0x0800) ? (DWORD)f.BitRead(32) : 0;

								if(i == 0 && (trun_flags & 0x04))
								{
									sample_flags = first_sample_flags;
								}

								s.id = track_id;
								s.duration = 10000i64 * sample_duration;
								s.size = sample_size;
								s.syncpoint = true;

								samples.push_back(s);
//printf("sample %08x\n", (int)s.pos);
								s.start += s.duration;
								s.pos += s.size;
							}

							cur_data_offset = s.pos;
							cur_start = s.start;
						}
						else if(h3.tag == 'sdtp')
						{
							f.BitRead(32);

							for(auto i = samples.begin(); i != samples.end(); i++)
							{
								BYTE flags = (BYTE)f.BitRead(8);

								i->syncpoint = ((flags >> 4) & 3) == 2;
							}
						}

						f.Seek(h3.next);
					}
				}

				f.Seek(h2.next);
			}
		}
		else if(h.tag == 'mdat')
		{
			break;
		}

		f.Seek(h.next);
	}

	return true;
}

// IFileSourceFilter

STDMETHODIMP CStrobeSourceFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		return VFW_E_NOT_STOPPED;
	}

	m_url.clear();

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}

	m_outputs.clear();

	m_chunks.clear();

	m_current_chunk = m_chunks.end();

	delete m_file; m_file = NULL;

	std::wstring fn = pszFileName;

	Util::Replace(fn, L"strobe://", L"http://");

	std::string url = std::string(fn.begin(), fn.end());

	Util::Socket::Startup();

	Util::Socket s;

	HRESULT hr;

	hr = Load(s, url.c_str());

	if(SUCCEEDED(hr))
	{
		m_url = fn;
	}

	s.Close();

	Util::Socket::Cleanup();

	std::vector<Strobe::Media*> media;

	for(auto i = m_manifest.m_media.begin(); i != m_manifest.m_media.end(); i++)
	{
		media.push_back(&*i);
	}

	std::sort(media.begin(), media.end(), [] (const Strobe::Media* a, const Strobe::Media* b) -> bool 
	{
		return a->m_bitrate > b->m_bitrate;
	});

	for(auto i = media.begin(); i != media.end(); i++)
	{
		Strobe::Media* m = *i;

		m_chunks.clear();

		{
			CBaseSplitterFile f((const BYTE*)m->m_bootstrapInfo.c_str(), m->m_bootstrapInfo.size());

			CBaseSplitterFile::MP4BoxEx h;

			while(f.Read(h))
			{
				if(h.tag == 'abst')
				{
					if(!ParseABST(f, m_abst))
					{
						return E_FAIL;
					}

					if(m_abst.Segments.empty() || m_abst.Segments[0].Entries.empty() || m_abst.Fragments.empty())
					{
						return E_FAIL;
					}

					DWORD segment = 1;
					DWORD fragment = 1;
					UINT64 start = 0;
					DWORD duration = 0;

					// TODO: find a multi-segment stream 

					/*
					for(auto i = m_abst.Segments.begin(); i != m_abst.Segments.end(); i++)
					{
						for(auto j = i->Entries.begin(); j != i->Entries.end(); j++)
						{
							segment = j->FirstSegment;
					*/

					std::vector<REFERENCE_TIME> durations;

					for(auto k = m_abst.Fragments.begin(); k != m_abst.Fragments.end(); k++)
					{
						for(auto l = k->Entries.begin(); l != k->Entries.end(); l++)
						{
							durations.push_back(10000000i64 * l->FragmentDuration / k->TimeScale);
						}
					}

					std::sort(durations.begin(), durations.end(), [] (const REFERENCE_TIME& a, const REFERENCE_TIME& b) -> bool 
					{
						return a < b;
					});

					REFERENCE_TIME def_duration = !durations.empty() ? durations[(durations.size() + 1) / 2] : 0;

					for(auto k = m_abst.Fragments.begin(); k != m_abst.Fragments.end(); k++)
					{
						for(auto l = k->Entries.begin(); l != k->Entries.end(); l++)
						{
							if(!m_chunks.empty() && m_chunks.back().segment == segment)
							{
								Chunk* back = &m_chunks.back();

								while(back->fragment + 1 < l->FirstFragment)
								{
									Chunk c = *back;

									c.fragment++;
									c.start += def_duration;//c.duration;

									m_chunks.push_back(c);

									back = &m_chunks.back();
								}
							}

							Chunk c;

							c.segment = segment;
							c.fragment = l->FirstFragment;
							c.start = 10000000i64 * l->FirstFragmentTimestamp / k->TimeScale;
							c.duration = 10000000i64 * l->FragmentDuration / k->TimeScale;

							m_chunks.push_back(c);
						}

						if(!m_chunks.empty() && m_chunks.back().segment == segment)
						{
							Chunk* back = &m_chunks.back();

							while(back->fragment + 1 <= m_abst.Segments[0].Entries[0].FragmentsPerSegment)
							{
								Chunk c = *back;

								c.fragment++;
								c.start += def_duration;//c.duration;

								m_chunks.push_back(c);

								back = &m_chunks.back();
							}
						}
						

						break;
					}

					/*
						}
					}
					*/

					break;
				}

				f.Seek(h.next);
			}
		}

		if(m_chunks.empty()) continue;

		Chunk& c = m_chunks.front();

		std::string url = m_baseurl + '/' + m->m_url + Util::Format("Seg%d-Frag%d", c.segment, c.fragment);

		CHttpStream* s = new CHttpStream();

		if(!s->Load(url.c_str())) {delete s; continue;}

		HRESULT hr;

		CBaseSplitterFile f(new CAsyncHttpStreamReader(s), hr);

		std::list<Sample> samples;

		m_track_id = 0;

		ParseMOOF(f, c.start, m_track_id, samples);

		CStrobeSourceOutputPin* video = NULL;
		CStrobeSourceOutputPin* audio = NULL;

		for(auto i = samples.begin(); i != samples.end(); i++)
		{
			f.Seek(i->pos);

			BYTE Reserved = (BYTE)f.BitRead(2);
			BYTE Filter = (BYTE)f.BitRead();
			BYTE TagType = (BYTE)f.BitRead(5);

			if(Filter) continue;

			DWORD DataSize = (DWORD)f.BitRead(24);
			DWORD TimeStamp = (DWORD)f.BitRead(24) | ((DWORD)f.BitRead(8) << 24);
			DWORD StreamId = (DWORD)f.BitRead(24);

			CMediaType mt;

			if(TagType == 9 && !m->m_metadata.videocodecid.empty() && video == NULL)
			{
				CBaseSplitterFile::FLVVideoPacket h;

				if(f.Read(h, DataSize, &mt))
				{
					video = new CStrobeSourceOutputPin(mt, this, &hr);

					if(m_track_id != i->id)
					{
						delete audio;

						audio = NULL;
					}

					m_track_id = i->id;
				}
			}
			else if(TagType == 8 && !m->m_metadata.audiocodecid.empty() && audio == NULL)
			{
				CBaseSplitterFile::FLVAudioPacket h;

				if(f.Read(h, DataSize, &mt))
				{
					audio = new CStrobeSourceOutputPin(mt, this, &hr);

					m_track_id = i->id;
				}
			}

			if((video != NULL || m->m_metadata.videocodecid.empty()) 
			&& (audio != NULL || m->m_metadata.audiocodecid.empty()))
			{
				m_track_id = i->id;

				break;
			}
		}

		if(video != NULL) m_outputs.push_back(video);
		if(audio != NULL) m_outputs.push_back(audio);

		if(!m_outputs.empty())
		{
			m_baseurl += '/' + m->m_url;
			
			break;
		}
	}

	return !m_outputs.empty() ? S_OK : E_FAIL;
}

STDMETHODIMP CStrobeSourceFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_url.size() + 1) * sizeof(WCHAR))))
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_url.c_str());

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CStrobeSourceFilter::GetCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL)
	{
		return E_POINTER;
	}
	
	*pCapabilities = 
		AM_SEEKING_CanGetStopPos | 
		AM_SEEKING_CanGetDuration | 
		AM_SEEKING_CanSeekAbsolute |
		AM_SEEKING_CanSeekForwards |
		AM_SEEKING_CanSeekBackwards;

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::CheckCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL) return E_POINTER;

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;

	GetCapabilities(&caps);

	caps &= *pCapabilities;

	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CStrobeSourceFilter::IsFormatSupported(const GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CStrobeSourceFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CStrobeSourceFilter::GetTimeFormat(GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CStrobeSourceFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CStrobeSourceFilter::GetDuration(LONGLONG* pDuration)
{
	if(pDuration == NULL)
	{
		return E_POINTER;
	}
	
	*pDuration = m_duration;

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::GetStopPosition(LONGLONG* pStop)
{
	if(pStop == NULL)
	{
		return E_POINTER;
	}

	*pStop = m_duration;

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	return E_NOTIMPL;
}

STDMETHODIMP CStrobeSourceFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CStrobeSourceFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if(ThreadExists())
	{
		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverBeginFlush();
		}

		CallWorker(CMD_FLUSH);

		m_start = *pCurrent;

		m_samples.clear();

		m_current_chunk = m_chunks.end();

		for(auto i = m_chunks.begin(); i != m_chunks.end(); )
		{
			auto j = i++;

			if(i == m_chunks.end() || i->start > m_start - 50000000)
			{
				m_current_chunk = j;

				break;
			}
		}

		SeekToNextSample();

		CallWorker(CMD_START);
	}

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent == NULL || pStop == NULL)
	{
		return E_POINTER;
	}

	*pCurrent = m_current;
	*pStop = m_duration;

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest == NULL || pLatest == NULL)
	{
		return E_POINTER;
	}

	*pEarliest = 0;
	*pLatest = m_duration;

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::SetRate(double dRate)
{
	return dRate == 1.0 ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CStrobeSourceFilter::GetRate(double* pdRate)
{
	if(pdRate == NULL)
	{
		return E_POINTER;
	}

	*pdRate = 1.0;

	return S_OK;
}

STDMETHODIMP CStrobeSourceFilter::GetPreroll(LONGLONG* pllPreroll)
{
	if(pllPreroll == NULL)
	{
		return E_POINTER;
	}

	*pllPreroll = 0;

	return S_OK;
}

// CStrobeSourceOutputPin

CStrobeSourceFilter::CStrobeSourceOutputPin::CStrobeSourceOutputPin(const CMediaType& mt, CStrobeSourceFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(NAME("CStrobeSourceOutputPin"), pFilter, pFilter, phr, L"Output")
	, m_queue(100)
	, m_flushing(false)
{
	m_mt = mt;

	m_h264 = m_mt.subtype == MEDIASUBTYPE_H264 || m_mt.subtype == MEDIASUBTYPE_AVC1;

	*phr = S_OK;
}

CStrobeSourceFilter::CStrobeSourceOutputPin::~CStrobeSourceOutputPin()
{
	FlushH264();
}

void CStrobeSourceFilter::CStrobeSourceOutputPin::FlushH264()
{
	for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++) 
	{
		delete *i;
	}

	m_h264queue.clear();
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::Active()
{
	HRESULT hr;

	hr = __super::Active();

	CAMThread::Create();

	return hr;
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::Inactive()
{
	m_exit.Set();

	CAMThread::Close();

	FlushH264();

	return __super::Inactive();
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, float dRate)
{
	m_start = tStart;

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::Deliver(QueueItem* item)
{
	if(m_flushing) {delete item; return S_FALSE;}

	if(m_h264) 
	{
		bool b_frame = false;

		if(!item->sample.syncpoint)
		{
			CBaseSplitterFile f(item->buff.data(), item->buff.size());

			while(f.GetRemaining() > 4)
			{
				DWORD size = (DWORD)f.BitRead(32);

				__int64 next = f.GetPos() + size;

				BYTE forbidden_zero_bit = f.BitRead(1);
				BYTE nal_ref_idc = f.BitRead(2);
				BYTE nal_unit_type = f.BitRead(5);

				if(nal_unit_type == 1 || nal_unit_type == 5)
				{
					BYTE first_mb_in_slice = f.UExpGolombRead();
					BYTE slice_type = f.UExpGolombRead();
					BYTE pic_patameter_set_id = f.UExpGolombRead();
					BYTE frame_num = f.BitRead(2);

					if(slice_type == 1 || slice_type == 6) 
					{
						b_frame = true;

						break;
					}
				}

				f.Seek(next);
			}
		}

		if(b_frame)
		{
			if(!m_h264queue.empty())
			{
				QueueItem* item2 = m_h264queue.front();

				REFERENCE_TIME atpf = item->sample.start - item2->sample.start;

				item2->sample.start += atpf;
				item->sample.start -= atpf;

				m_h264queue.push_back(item);
			}
		}
		else
		{
			while(!m_h264queue.empty())
			{
				QueueItem* item2 = m_h264queue.front();

				m_h264queue.pop_front();

				m_queue.Enqueue(item2);
			}

			m_h264queue.push_back(item);
		}
	}
	else
	{
		m_queue.Enqueue(item);
	}

	return S_OK;
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::DeliverBeginFlush()
{
	m_flushing = true;

	return __super::DeliverBeginFlush();
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::DeliverEndFlush()
{
	m_queue.GetDequeueEvent().Wait();

	FlushH264();

	m_flushing = false;

	return __super::DeliverEndFlush();
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_mt == *pmt ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mt;

	return S_OK;
}

HRESULT CStrobeSourceFilter::CStrobeSourceOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1 << 20;

	return S_OK;
}

DWORD CStrobeSourceFilter::CStrobeSourceOutputPin::ThreadProc()
{
	HRESULT hr;
	
	HANDLE handles[] = {m_exit, m_queue.GetEnqueueEvent()};

	while(m_hThread != NULL)
	{
		switch(::WaitForMultipleObjects(countof(handles), handles, FALSE, INFINITE))
		{
		default:
		case WAIT_OBJECT_0 + 0: 

			return 0;

		case WAIT_OBJECT_0 + 1:

			while(m_queue.GetCount() > 0)
			{
				QueueItem* item = m_queue.Dequeue();

				int size = item->buff.size();

				if(!m_flushing)
				{
					CComPtr<IMediaSample> sample;

					hr = GetDeliveryBuffer(&sample, NULL, NULL, 0);

					if(SUCCEEDED(hr) && sample->GetSize() >= size)
					{
						BYTE* dst = NULL;
						
						hr = sample->GetPointer(&dst);

						if(SUCCEEDED(hr) && dst != NULL)
						{
							memcpy(dst, item->buff.data(), size);

							sample->SetActualDataLength(size);
				
							REFERENCE_TIME start = item->sample.start - m_start;
							REFERENCE_TIME stop = item->sample.start + item->sample.duration - m_start;

							sample->SetTime(&start, &stop);
							sample->SetMediaTime(NULL, NULL);

							sample->SetDiscontinuity(FALSE);
							sample->SetSyncPoint(item->sample.syncpoint);
							sample->SetPreroll(stop < 0);

							__super::Deliver(sample);
						}
					}
				}

				delete item;
			}

			break;
		}
	}

	return 0;
}
