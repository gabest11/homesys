#include "StdAfx.h"
#include "DSMSplitterFile.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

CDSMSplitterFile::CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr, IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap) 
	: CBaseSplitterFile(pReader, hr)
	, m_key(NULL)
{
	if(FAILED(hr)) return;

	hr = Init(res, chap);
}

CDSMSplitterFile::~CDSMSplitterFile()
{
	delete m_key;
}

HRESULT CDSMSplitterFile::Init(IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap)
{
	Seek(0);

	if(BitRead(DSMSW_SIZE << 3) != DSMSW || BitRead(5) != DSMP_FILEINFO)
	{
		return E_FAIL;
	}

	Seek(0);

	m_mts.clear();
	m_time.first = 0;
	m_time.dur = 0;
	m_fim.clear();
	m_sim.clear();
	res.ResRemoveAll();
	chap.ChapRemoveAll();

	delete m_key; m_key = NULL;

	dsmp_t type;
	UINT64 len;
	int limit = 65536;

	// examine the beginning of the file ...

	while(Sync(type, len, 0))
	{
		__int64 pos = GetPos();

		if(type == DSMP_MEDIATYPE)
		{
			BYTE id;
			CMediaType mt;

			if(Read(len, id, mt)) 
			{
				m_mts[id] = mt;
			}
		}
		else if(type == DSMP_SAMPLE)
		{
			CPacket p;

			if(Read(len, &p, false) && (p.flags & CPacket::TimeValid))
			{
				m_time.first = p.start;

				break;
			}
		}
		else if(type == DSMP_FILEINFO) 
		{
			BYTE version = (BYTE)BitRead(8);

			if(version > DSMF_VERSION) 
			{
				return E_FAIL;
			}

			Read(len - 1, m_fim);
		}
		else if(type == DSMP_KEY && m_key == NULL)
		{
			m_key = new DSMKey();
			m_key->type = (BYTE)BitRead(8);
			m_key->id = (DWORD)BitRead(32);
			m_key->len = len - 5;
			m_key->data = new BYTE[m_key->len];

			ByteRead(m_key->data, m_key->len);
		}
		else if(type == DSMP_STREAMINFO) 
		{
			Read(len - 1, m_sim[(BYTE)BitRead(8)]);
		}
		else if(type == DSMP_SYNCPOINTS) 
		{
			Read(len, m_sps);
		}
		else if(type == DSMP_RESOURCE) 
		{
			Read(len, res);
		}
		else if(type == DSMP_CHAPTERS) 
		{
			Read(len, chap);
		}

		Seek(pos + len);
	}

	if(type != DSMP_SAMPLE)
	{
		return E_FAIL;
	}

	// ... and the end 

	int end = (int)((GetLength() + limit / 2) / limit);

	for(int i = 1; i <= end; i++)
	{
		__int64 seekpos = max(0, (__int64)GetLength() - i * limit);

		Seek(seekpos);

		while(Sync(type, len, limit) && GetPos() < seekpos + limit)
		{
			__int64 pos = GetPos();

			if(type == DSMP_SAMPLE)
			{
				CPacket p;
				
				if(Read(len, &p, false) && (p.flags & CPacket::TimeValid))
				{
					m_time.dur = max(m_time.dur, p.stop - m_time.first); // max isn't really needed, only for safety

					i = end;
				}	
			}
			else if(type == DSMP_SYNCPOINTS) 
			{
				Read(len, m_sps);
			}
			else if(type == DSMP_RESOURCE) 
			{
				Read(len, res);
			}
			else if(type == DSMP_CHAPTERS) 
			{
				Read(len, chap);
			}

			Seek(pos + len);
		}
	}

	if(m_time.first < 0)
	{
		m_time.dur += m_time.first;
		m_time.first = 0;
	}

	return !m_mts.empty() ? S_OK : E_FAIL;
}

bool CDSMSplitterFile::Sync(dsmp_t& type, UINT64& len, __int64 limit)
{
	UINT64 pos;

	return Sync(pos, type, len, limit);
}

bool CDSMSplitterFile::Sync(UINT64& syncpos, dsmp_t& type, UINT64& len, __int64 limit)
{
	BitByteAlign();

	limit += DSMSW_SIZE;

	for(UINT64 id = 0; (id & ((1ui64 << (DSMSW_SIZE << 3)) - 1)) != DSMSW; )
	{
		if(limit-- <= 0 || GetRemaining() <= 2)
		{
			return false;
		}
		
		id = (id << 8) | (BYTE)BitRead(8);
	}

	syncpos = GetPos() - (DSMSW_SIZE << 3);

	type = (dsmp_t)BitRead(5);

	len = BitRead(((int)BitRead(3) + 1) << 3);

	return(true);
}

bool CDSMSplitterFile::Read(__int64 len, BYTE& id, CMediaType& mt)
{
	id = (BYTE)BitRead(8);
	
	ByteRead((BYTE*)&mt.majortype, sizeof(mt.majortype));
	ByteRead((BYTE*)&mt.subtype, sizeof(mt.subtype));
	
	mt.bFixedSizeSamples = (BOOL)BitRead(1);
	mt.bTemporalCompression = (BOOL)BitRead(1);
	mt.lSampleSize = (ULONG)BitRead(30);
	
	ByteRead((BYTE*)&mt.formattype, sizeof(mt.formattype));

	len -= 5 + sizeof(GUID)*3;
	
	ASSERT(len >= 0);
	
	if(len > 0) 
	{
		ByteRead(mt.AllocFormatBuffer(len), len);
	}
	else
	{
		mt.ResetFormatBuffer();
	}

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, CPacket* p, bool data)
{
	if(!p) return false;

	p->id = (DWORD)BitRead(8);
	
	if(BitRead(1) != 0) 
	{
		p->flags |= CPacket::SyncPoint;
	}

	bool sign = !!BitRead(1);

	int pts_len = (int)BitRead(3);
	int dur_len = (int)BitRead(3);

	if(!sign || pts_len > 0)
	{
		p->start = (REFERENCE_TIME)BitRead(pts_len << 3) * (sign ? -1 : 1);
		p->stop = p->start + BitRead(dur_len << 3);

		p->flags |= CPacket::TimeValid;
	}

	if(data)
	{
		int size = len - (2 + pts_len + dur_len);

		p->buff.resize(size);

		ByteRead(&p->buff[0], size);

		if(m_key)
		{
			m_key->Decode(&p->buff[0], size);
		}
	}

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, std::vector<SyncPoint>& sps)
{
	SyncPoint sp = {0, 0};

	sps.clear();

	while(len > 0)
	{
		bool sign = !!BitRead(1);

		int pts_len = (int)BitRead(3);
		int pos_len = (int)BitRead(3);

		BitRead(1); // reserved

		sp.m_rt += (REFERENCE_TIME)BitRead(pts_len << 3) * (sign ? -1 : 1);
		sp.m_fp += BitRead(pos_len << 3);
		
		sps.push_back(sp);

		len -= 1 + pts_len + pos_len;
	}

	if(len != 0)
	{
		sps.clear();

		return false;
	}

	// TODO: sort sps (it should be already but anyway)

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, StreamInfoMap& im)
{
	while(len >= 5)
	{
		std::wstring key;
		std::wstring value;

		ReadAString(key, 4);

		len -= 4;
		len -= Read(len, value);

		im[key] = value;
	}

	return len == 0;
}

bool CDSMSplitterFile::Read(__int64 len, IDSMResourceBagImpl& res)
{
	BYTE compression = (BYTE)BitRead(2);
	BYTE reserved = (BYTE)BitRead(6);

	len--;

	CDSMResource r;

	len -= Read(len, r.m_name);
	len -= Read(len, r.m_desc);
	len -= Read(len, r.m_mime);

	if(compression != 0 || len <= 0) // TODO: compression
	{
		return false;
	}

	r.m_data.resize(len);

	ByteRead(&r.m_data[0], len);

	res += r;

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, IDSMChapterBagImpl& chap)
{
	CDSMChapter c;

	while(len > 0)
	{
		bool sign = !!BitRead(1);

		int pts_len = (int)BitRead(3);

		BitRead(4); // reserved

		len--;

		c.m_rt += (REFERENCE_TIME)BitRead(pts_len << 3) * (sign ? -1 : 1);

		len -= pts_len;
		len -= Read(len, c.m_name);

		chap += c;
	}

	chap.ChapSort();

	return len == 0;
}

__int64 CDSMSplitterFile::Read(__int64 len, std::wstring& str)
{
	std::string s;
	
	__int64 i = 0;
	
	while(i++ < len)
	{
		char c = (char)BitRead(8);

		if(c == 0)
		{
			break;
		}

		s += c;
	}

	str = Util::UTF8To16(s.c_str());

	return i;
}

__int64 CDSMSplitterFile::FindSyncPoint(REFERENCE_TIME rt)
{
	if(m_sps.size() > 1)
	{
		int i = range_bsearch(m_sps, m_time.first + rt);

		return i >= 0 ? m_sps[i].m_fp : 0;
	}

	if(m_time.dur <= 0 || rt <= 0)
	{
		return 0;
	}

	// ok, do the hard way then

	UINT64 syncpos, len;

	// 1. find some boundaries close to rt's position (minpos, maxpos)

	__int64 minpos = 0, maxpos = GetLength();

	for(int i = 0; i < 10 && (maxpos - minpos) >= 1024 * 1024; i++)
	{
		Seek((minpos + maxpos) / 2);

		while(GetPos() < maxpos)
		{
			dsmp_t type;

			if(!Sync(syncpos, type, len))
			{
				continue;
			}

			__int64 pos = GetPos();

			if(type == DSMP_SAMPLE)
			{
				CPacket p;

				if(Read(len, &p, false) && (p.flags & CPacket::TimeValid))
				{
					if(p.start - m_time.first >= rt)
					{
						maxpos = max((__int64)syncpos - 65536, minpos);
					}
					else 
					{
						minpos = syncpos;
					}

					break;
				}
			}

			Seek(pos + len);
		}
	}

	// 2. find the first packet just after rt (maxpos)

	Seek(minpos);

	while(GetRemaining())
	{
		dsmp_t type;

		if(!Sync(syncpos, type, len))
		{
			continue;
		}

		__int64 pos = GetPos();

		if(type == DSMP_SAMPLE)
		{
			CPacket p;

			if(Read(len, &p, false) && (p.flags & CPacket::TimeValid))
			{
				if(p.start - m_time.first >= rt) 
				{
					maxpos = (__int64)syncpos;

					break;
				}
			}
		}

		Seek(pos + len);
	}

	printf("%lld %lld %lld\n", rt, minpos, maxpos);

	// 3. iterate backwards from maxpos and find at least one syncpoint for every stream, except for subtitle streams

	std::map<BYTE, BYTE> ids;

	for(auto i = m_mts.begin(); i != m_mts.end(); i++)
	{
		const CMediaType& mt = i->second;

		if(mt.majortype != MEDIATYPE_Text && mt.majortype != MEDIATYPE_Subtitle)
		{
			ids[i->first] = 0;
		}
	}

	__int64 ret = maxpos;

	while(maxpos > 0 && !ids.empty())
	{
		minpos = max(0, maxpos - 65536);

		Seek(minpos);

		dsmp_t type;

		while(Sync(syncpos, type, len) && GetPos() < maxpos && !ids.empty())
		{
			UINT64 pos = GetPos();

			if(type == DSMP_SAMPLE)
			{
				CPacket p;

				if(Read(len, &p, false) && (p.flags & CPacket::TimeValid) && (p.flags & CPacket::SyncPoint))
				{
					auto i = ids.find(p.id);

					if(i != ids.end())
					{
						ids.erase(i);

						ret = min(ret, (__int64)syncpos);
					}
				}	
			}

			Seek(pos + len);
		}

		maxpos = minpos;
	}

	printf("%lld\n", ret);

	Sleep(2000);

	return ret;
}
