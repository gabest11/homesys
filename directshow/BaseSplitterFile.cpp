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
#include "BaseSplitterFile.h"
#include "AsyncReader.h"
#include "DirectShow.h"
#include "../3rdparty/libfaad2/faad.h"
#include <initguid.h>
#include "moreuuids.h"

// CBaseSplitterFile

CBaseSplitterFile::CBaseSplitterFile(IAsyncReader* reader, HRESULT& hr, int cachelen)
	: m_reader(reader)
	, m_pos(0)
	, m_len(0)
	, m_tslen(0)
	, m_mem(NULL)
{
	m_bit.buff = 0;
	m_bit.len = 0;

	if(m_reader == NULL)
	{
		hr = E_UNEXPECTED; 

		return;
	}

	LONGLONG total = 0;
	LONGLONG available = 0;

	hr = m_reader->Length(&total, &available);

	if(FAILED(hr) /*|| total == 0 || total != available*/)
	{
		hr = E_FAIL;

		return;
	}

	m_len = available;

	if(!SetCacheSize(cachelen))
	{
		hr = E_OUTOFMEMORY;

		return;
	}

	hr = S_OK;
}

CBaseSplitterFile::CBaseSplitterFile(const BYTE* buff, int len)
	: m_pos(0)
	, m_len(len)
	, m_tslen(0)
	, m_mem(buff)
{
	m_bit.buff = 0;
	m_bit.len = 0;

	SetCacheSize(0);
}

bool CBaseSplitterFile::SetCacheSize(int len)
{
	m_cache.buff.resize(len);
	m_cache.pos = 0;
	m_cache.len = 0;

	return true;
}

void CBaseSplitterFile::ClearCache()
{
	m_cache.len = 0;
}

__int64 CBaseSplitterFile::GetPos()
{
	return m_pos - (m_bit.len >> 3);
}

__int64 CBaseSplitterFile::GetLength()
{
	return m_len;
}

__int64 CBaseSplitterFile::GetRemaining()
{
	return std::max<__int64>(GetLength() - GetPos(), 0);
}

void CBaseSplitterFile::Seek(__int64 pos)
{
	__int64 len = GetLength();

	if(pos < 0) 
	{
		pos = 0;
	}
	else if(pos > len) 
	{
		pos = len;
	}

	m_pos = pos;

	BitFlush();
}

HRESULT CBaseSplitterFile::Read(BYTE* data, __int64 len)
{
	if(m_mem != NULL)
	{
		if(m_pos >= 0 && m_pos + len <= m_len)
		{
			if(len == 1) *data = m_mem[m_pos];
			else memcpy(data, &m_mem[m_pos], len);

			m_pos += len;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	CheckPointer(m_reader, E_NOINTERFACE);

	HRESULT hr = S_OK;

	if(m_cache.buff.empty())
	{
		hr = m_reader->SyncRead(m_pos, (long)len, data);

		m_pos += len;

		return hr;
	}

	BYTE* cache = &m_cache.buff[0];

	if(m_cache.pos <= m_pos && m_pos < m_cache.pos + m_cache.len)
	{
		__int64 minlen = std::min<__int64>(len, m_cache.len - (m_pos - m_cache.pos));

		memcpy(data, &cache[m_pos - m_cache.pos], (size_t)minlen);

		len -= minlen;
		m_pos += minlen;
		data += minlen;
	}

	int size = m_cache.buff.size();

	while(len > size)
	{
		//printf("%lld %d\n", m_pos, size);

		hr = m_reader->SyncRead(m_pos, size, data);

		if(S_OK != hr)
		{
			return hr;
		}

		len -= size;
		m_pos += size;
		data += size;
	}

	while(len > 0)
	{
		__int64 tmplen = GetLength();
		__int64 maxlen = std::min<__int64>(tmplen - m_pos, size);
		__int64 minlen = std::min<__int64>(len, maxlen);

		if(minlen <= 0)
		{
			return S_FALSE;
		}

		//printf("%lld %d\n", m_pos, maxlen);

		hr = m_reader->SyncRead(m_pos, (long)maxlen, cache);

		if(S_OK != hr) 
		{
			return hr;
		}

		m_cache.pos = m_pos;
		m_cache.len = maxlen;

		memcpy(data, cache, (size_t)minlen);

		len -= minlen;
		m_pos += minlen;
		data += minlen;
	}

	return hr;
}

BYTE CBaseSplitterFile::BitRead()
{
	BYTE ret;

	if(m_bit.len <= 24)
	{
		if(m_bit.len == 0)
		{
			if(S_OK != Read((BYTE*)&m_bit.buff32, 1)) // EOF? // ASSERT(0);
			{
				return 0;
			}
			
			m_bit.len = 8;
		}

		int bitlen = m_bit.len - 1;

		ret = (BYTE)(m_bit.buff32 >> bitlen) & 1;

		// if(!peek)
		{
			m_bit.buff32 &= ((DWORD)1 << bitlen) - 1;
			m_bit.len = bitlen;
		}
	}
	else
	{
		if(m_bit.len == 0)
		{
			if(S_OK != Read((BYTE*)&m_bit.buff, 1)) // EOF? // ASSERT(0);
			{
				return 0;
			}
			
			m_bit.len = 8;
		}

		int bitlen = m_bit.len - 1;

		ret = (BYTE)(m_bit.buff >> bitlen) & 1;

		// if(!peek)
		{
			m_bit.buff &= (1ui64 << bitlen) - 1;
			m_bit.len = bitlen;
		}
	}

	return ret;
}

UINT64 CBaseSplitterFile::BitRead(int bits, bool peek)
{
	ASSERT(bits >= 0 && bits <= 64);

	UINT64 ret;

	if(m_bit.len <= 24 && bits <= 24)
	{
		while(m_bit.len < bits)
		{
			m_bit.buff32 <<= 8;

			if(S_OK != Read((BYTE*)&m_bit.buff32, 1)) // EOF? // ASSERT(0);
			{
				return 0;
			}
			
			m_bit.len += 8;
		}

		int bitlen = m_bit.len - bits;

		ret = (m_bit.buff32 >> bitlen) & (((DWORD)1 << bits) - 1);

		if(!peek)
		{
			m_bit.buff32 &= ((DWORD)1 << bitlen) - 1;
			m_bit.len = bitlen;
		}
	}
	else
	{
		while(m_bit.len < bits)
		{
			m_bit.buff <<= 8;

			if(S_OK != Read((BYTE*)&m_bit.buff, 1)) // EOF? // ASSERT(0);
			{
				return 0;
			}
			
			m_bit.len += 8;
		}

		int bitlen = m_bit.len - bits;

		ret = (m_bit.buff >> bitlen) & ((1ui64 << bits) - 1);

		if(!peek)
		{
			m_bit.buff &= (1ui64 << bitlen) - 1;
			m_bit.len = bitlen;
		}
	}

	return ret;
}

void CBaseSplitterFile::BitByteAlign()
{
	m_bit.len &= ~7;
}

void CBaseSplitterFile::BitFlush()
{
	m_bit.len = 0;
}

HRESULT CBaseSplitterFile::ByteRead(BYTE* data, __int64 len)
{
    if(m_bit.len > 0)
	{
		Seek(GetPos());
	}

	return Read(data, len);
}

HRESULT CBaseSplitterFile::ReadString(std::string& s, int len)
{
	s.clear();

	char* buff = new char[len + 1];

	buff[len] = 0;

	HRESULT hr;

	hr = ByteRead((BYTE*)buff, len);

	if(SUCCEEDED(hr))
	{
		s = buff;
	}

	delete [] buff;

	return hr;
}

HRESULT CBaseSplitterFile::ReadAString(std::wstring& s, int len)
{
	HRESULT hr;

	std::string str;

	hr = ReadString(str, len);

	if(SUCCEEDED(hr))
	{
		int size = str.size();

		wchar_t* buff = new wchar_t[size + 1];

		size = MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), size, buff, size);

		buff[size] = 0;

		s = buff;

		delete buff;
	}

	return hr;
}

HRESULT CBaseSplitterFile::ReadWString(std::wstring& s, int len)
{
	s.clear();

	s.resize(len, 0);

	wchar_t* buff = new wchar_t[len + 1];

	buff[len] = 0;

	HRESULT hr;

	hr = ByteRead((BYTE*)buff, len * 2);

	if(SUCCEEDED(hr))
	{
		s = buff;
	}

	delete [] buff;

	return hr;
}

UINT64 CBaseSplitterFile::UExpGolombRead()
{
	int n = -1;
	for(BYTE b = 0; !b; n++) b = (BYTE)BitRead();
	return (1ui64 << n) - 1 + BitRead(n);
}

INT64 CBaseSplitterFile::SExpGolombRead()
{
	UINT64 k = UExpGolombRead();
	return ((k & 1) ? 1 : -1) * ((k + 1) >> 1);
}

//

bool CBaseSplitterFile::NextMpegStartCode(BYTE& code, int len, BYTE id)
{
	len = (int)std::min<__int64>(GetRemaining(), len);

	if(len <= 0)
	{
		return false;
	}

	BitByteAlign();

	DWORD dw = 0xffffffff;

	do
	{
		if(len-- == 0) 
		{
			return false;
		}

		dw = (dw << 8) | (BYTE)BitRead(8);
	}
	while((dw & 0xffffff00) != 0x00000100 || id != 0 && id != (BYTE)(dw & 0xff));

	code = (BYTE)(dw & 0xff);

	return true;
}

bool CBaseSplitterFile::NextNALU(BYTE& id, __int64& start, int len)
{
	BitByteAlign();

	__int64 remaining = GetRemaining();

	if(len < 0 || len > remaining) 
	{
		len = remaining;
	}

	int zeros = 0;
	
	while(len >= 2)
	{
		BYTE b;

		if(FAILED(ByteRead(&b, 1))) 
		{
			return false;
		}

		if(b == 0)
		{
			zeros++;
		
			continue;
		}
		
		if(b == 1 && zeros >= 2)
		{
			start = GetPos() - zeros - 1;
			id = BitRead(8);
			return true;
		}

		zeros = 0;
	}

	return false;

/*
	int zeros = 0;
	
	while(len >= 4 && BitRead(24, true) != 1)
	{
		zeros = BitRead(8) == 0 ? zeros + 1 : 0;
		len--;
	}

	if(len >= 4 && BitRead(24, true) == 1)
	{
		start = GetPos() - zeros;

		id = BitRead(32) & 0xff;

		return true;
	}

	return false;
*/
/*
	__int64 remaining = GetRemaining();

	if(len < 0 || len > remaining) 
	{
		len = remaining;
	}

	while(len > 0)
	{
		while(len-- > 0 && BitRead(8) != 0);

		if(len == 0)
		{
			return false;
		}

		start = GetPos() - 1;

		int zeros = 1;

		while(len-- > 0 && (id = (BYTE)BitRead(8)) == 0) 
		{
			zeros++;
		}

		if(len > 0 && zeros >= 3 && id == 1)
		{
			id = (BYTE)BitRead(8);

			return true;
		}
	}

	return false;
*/
}

bool CBaseSplitterFile::NextTSPacket(int& tslen, int len)
{
	BitByteAlign();

	if(m_tslen == 0)
	{
		__int64 pos = GetPos();

		for(int i = 0; i < 192; i++)
		{
			if(BitRead(8, true) == 0x47)
			{
				__int64 pos = GetPos();
				Seek(pos + 188);
				if(BitRead(8, true) == 0x47) {m_tslen = 188; break;}
				Seek(pos + 192);
				if(BitRead(8, true) == 0x47) {m_tslen = 192; break;}
			}

			BitRead(8);
		}

		Seek(pos);

		if(m_tslen == 0)
		{
			return false;
		}
	}

	for(int i = 0; i < m_tslen; i++)
	{
		if(BitRead(8, true) == 0x47)
		{
			if(i == 0) break;
			Seek(GetPos() + m_tslen);
			if(BitRead(8, true) == 0x47) {Seek(GetPos() - m_tslen); break;}
		}

		BitRead(8);

		if(i == m_tslen - 1)
		{
			return false;
		}
	}

	tslen = m_tslen;

	return true;
}

//

#define MARKER if(BitRead() != 1) {ASSERT(0); return false;}

bool CBaseSplitterFile::Read(PSHeader& h)
{
	memset(&h, 0, sizeof(h));

	BYTE b = (BYTE)BitRead(8, true);

	if((b & 0xf1) == 0x21)
	{
		h.type = Mpeg1;

		EXECUTE_ASSERT(BitRead(4) == 2);

		h.scr = 0;
		h.scr |= BitRead(3) << 30; MARKER; // 32..30
		h.scr |= BitRead(15) << 15; MARKER; // 29..15
		h.scr |= BitRead(15); MARKER; MARKER; // 14..0
		h.bitrate = BitRead(22); MARKER;
	}
	else if((b & 0xc4) == 0x44)
	{
		h.type = Mpeg2;

		EXECUTE_ASSERT(BitRead(2) == 1);

		h.scr = 0;
		h.scr |= BitRead(3) << 30; MARKER; // 32..30
		h.scr |= BitRead(15) << 15; MARKER; // 29..15
		h.scr |= BitRead(15); MARKER; // 14..0
		h.scr = (h.scr*300 + BitRead(9)) * 10 / 27; MARKER;
		h.bitrate = BitRead(22); MARKER; MARKER;
		BitRead(5); // reserved
		UINT64 stuffing = BitRead(3);
		while(stuffing-- > 0) EXECUTE_ASSERT(BitRead(8) == 0xff);
	}
	else
	{
		return false;
	}

	h.bitrate *= 400;

	return true;
}

bool CBaseSplitterFile::Read(PSSystemHeader& h)
{
	memset(&h, 0, sizeof(h));

	WORD len = (WORD)BitRead(16); MARKER;
	h.rate_bound = (DWORD)BitRead(22); MARKER;
	h.audio_bound = (BYTE)BitRead(6);
	h.fixed_rate = !!BitRead();
	h.csps = !!BitRead();
	h.sys_audio_loc_flag = !!BitRead();
	h.sys_video_loc_flag = !!BitRead(); MARKER;
	h.video_bound = (BYTE)BitRead(5);

	EXECUTE_ASSERT((BitRead(8) & 0x7f) == 0x7f); // reserved (should be 0xff, but not in reality)

	for(len -= 6; len > 3; len -= 3) // TODO: also store these, somewhere, if needed
	{
		UINT64 stream_id = BitRead(8);
		EXECUTE_ASSERT(BitRead(2) == 3);
		UINT64 p_std_buff_size_bound = (BitRead() ? 1024 : 128) * BitRead(13);
	}

	return(true);
}

bool CBaseSplitterFile::Read(PESHeader& h, BYTE code)
{
	memset(&h, 0, sizeof(h));

	if(!(code >= 0xbd && code < 0xf0 || code == 0xfd))
	{
		return false;
	}

	h.len = (WORD)BitRead(16);

	if(code == 0xbe || code == 0xbf)
	{
		return true;
	}

	// mpeg1 stuffing (ff ff .. , max 16x)

	for(int i = 0; i < 16 && BitRead(8, true) == 0xff; i++)
	{
		BitRead(8); 

		if(h.len) 
		{
			h.len--;
		}
	}

	h.type = (BYTE)BitRead(2, true) == Mpeg2 ? Mpeg2 : Mpeg1;

	if(h.type == Mpeg1)
	{
		BYTE b = (BYTE)BitRead(2);

		if(b == 1)
		{
			h.std_buff_size = (BitRead() ? 1024 : 128) * BitRead(13);

			if(h.len) 
			{
				h.len -= 2;
			}

			b = (BYTE)BitRead(2);
		}

		if(b == 0)
		{
			h.fpts = (BYTE)BitRead();
			h.fdts = (BYTE)BitRead();
		}
	}
	else if(h.type == Mpeg2)
	{
		EXECUTE_ASSERT(BitRead(2) == Mpeg2);
		h.scrambling = (BYTE)BitRead(2);
		h.priority = (BYTE)BitRead();
		h.alignment = (BYTE)BitRead();
		h.copyright = (BYTE)BitRead();
		h.original = (BYTE)BitRead();
		h.fpts = (BYTE)BitRead();
		h.fdts = (BYTE)BitRead();
		h.escr = (BYTE)BitRead();
		h.esrate = (BYTE)BitRead();
		h.dsmtrickmode = (BYTE)BitRead();
		h.morecopyright = (BYTE)BitRead();
		h.crc = (BYTE)BitRead();
		h.extension = (BYTE)BitRead();
		h.hdrlen = (BYTE)BitRead(8);
	}
	else
	{
		if(h.len) 
		{
			while(h.len-- > 0) BitRead(8);
		}

		return false;
	}

	if(h.fpts)
	{
		if(h.type == Mpeg2)
		{
			BYTE b = (BYTE)BitRead(4);

			if(!(h.fdts && b == 3 || !h.fdts && b == 2)) 
			{
				ASSERT(0); 
				
				return false;
			}
		}

		h.pts = 0;
		h.pts |= BitRead(3) << 30; MARKER; // 32..30
		h.pts |= BitRead(15) << 15; MARKER; // 29..15
		h.pts |= BitRead(15); MARKER; // 14..0
		h.pts = 10000*h.pts/90;
	}

	if(h.fdts)
	{
		if((BYTE)BitRead(4) != 1) 
		{
			ASSERT(0); 
			
			return(false);
		}

		h.dts = 0;
		h.dts |= BitRead(3) << 30; MARKER; // 32..30
		h.dts |= BitRead(15) << 15; MARKER; // 29..15
		h.dts |= BitRead(15); MARKER; // 14..0
		h.dts = 10000*h.dts/90;
	}

	// skip to the end of header

	if(h.type == Mpeg1)
	{
		if(!h.fpts && !h.fdts && BitRead(4) != 0xf)
		{
			/*ASSERT(0);*/ 
			
			return false;
		}

		if(h.len)
		{
			h.len--;
			if(h.pts) h.len -= 4;
			if(h.dts) h.len -= 5;
		}
	}

	if(h.type == Mpeg2)
	{
		if(h.len) 
		{
			h.len -= 3 + h.hdrlen;
		}

		int left = h.hdrlen;
		if(h.fpts) left -= 5;
		if(h.fdts) left -= 5;
		while(left-- > 0) BitRead(8);
/*
		// mpeg2 stuffing (ff ff .. , max 32x)
		while(BitRead(8, true) == 0xff) {BitRead(8); if(h.len) h.len--;}
		Seek(GetPos()); // put last peeked byte back for Read()

		// FIXME: this doesn't seems to be here, 
		// infact there can be ff's as part of the data 
		// right at the beginning of the packet, which 
		// we should not skip...
*/
	}

	return true;
}

bool CBaseSplitterFile::Read(MpegSequenceHeader& h, int len, CMediaType* pmt)
{
	__int64 endpos = GetPos() + len; // - sequence header length

	BYTE id = 0;

	while(GetPos() < endpos && id != 0xb3)
	{
		if(!NextMpegStartCode(id, len))
		{
			return false;
		}
	}

	if(id != 0xb3)
	{
		return false;
	}

	__int64 shpos = GetPos() - 4;

	h.width = (WORD)BitRead(12);
	h.height = (WORD)BitRead(12);
	h.ar = BitRead(4);
    static int ifps[16] = {0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000, 0, 0, 0, 0, 0, 0, 0};
	h.ifps = ifps[BitRead(4)];
	h.bitrate = (DWORD)BitRead(18); MARKER;
	h.vbv = (DWORD)BitRead(10);
	h.constrained = BitRead();

	if(h.fiqm = BitRead())
	{
		for(int i = 0; i < countof(h.iqm); i++)
		{
			h.iqm[i] = (BYTE)BitRead(8);
		}
	}

	if(h.fniqm = BitRead())
	{
		for(int i = 0; i < countof(h.niqm); i++)
		{
			h.niqm[i] = (BYTE)BitRead(8);
		}
	}

	__int64 shlen = GetPos() - shpos;

	static float ar[] = 
	{
		1.0000f, 1.0000f, 0.6735f, 0.7031f, 0.7615f, 0.8055f, 0.8437f, 0.8935f,
		0.9157f, 0.9815f, 1.0255f, 1.0695f, 1.0950f, 1.1575f, 1.2015f, 1.0000f
	};

	h.arx = (int)((float)h.width / ar[h.ar] + 0.5);
	h.ary = h.height;

	MpegType type = Mpeg1;

	__int64 shextpos = 0, shextlen = 0;

	if(NextMpegStartCode(id, 8) && id == 0xb5) // sequence header ext
	{
		shextpos = GetPos() - 4;

		h.startcodeid = BitRead(4);
		h.profile_levelescape = BitRead(); // reserved, should be 0
		h.profile = BitRead(3);
		h.level = BitRead(4);
		h.progressive = BitRead();
		h.chroma = BitRead(2);
		h.width |= (BitRead(2)<<12);
		h.height |= (BitRead(2)<<12);
		h.bitrate |= (BitRead(12)<<18); MARKER;
		h.vbv |= (BitRead(8)<<10);
		h.lowdelay = BitRead();
		h.ifps = (DWORD)(h.ifps * (BitRead(2)+1) / (BitRead(5)+1));

		shextlen = GetPos() - shextpos;

		struct {DWORD x, y;} ar[] = {{h.width,h.height},{4,3},{16,9},{221,100},{h.width,h.height}};
		int i = min(max(h.ar, 1), 5)-1;
		h.arx = ar[i].x;
		h.ary = ar[i].y;

		type = Mpeg2;
	}

	h.ifps = 10 * h.ifps / 27;
	h.bitrate = h.bitrate == (1<<30)-1 ? 0 : h.bitrate * 400;

	DWORD a = h.arx, b = h.ary;
    while(a) {DWORD tmp = a; a = b % tmp; b = tmp;}
	if(b) h.arx /= b, h.ary /= b;

	if(!pmt) return true;

	pmt->majortype = MEDIATYPE_Video;

	if(type == Mpeg1)
	{
		pmt->subtype = MEDIASUBTYPE_MPEG1Payload;
		pmt->formattype = FORMAT_MPEGVideo;
		int len = FIELD_OFFSET(MPEG1VIDEOINFO, bSequenceHeader) + shlen + shextlen;
		MPEG1VIDEOINFO* vi = (MPEG1VIDEOINFO*)new BYTE[len];
		memset(vi, 0, len);
		vi->hdr.dwBitRate = h.bitrate;
		vi->hdr.AvgTimePerFrame = h.ifps;
		vi->hdr.bmiHeader.biSize = sizeof(vi->hdr.bmiHeader);
		vi->hdr.bmiHeader.biWidth = h.width;
		vi->hdr.bmiHeader.biHeight = h.height;
		vi->hdr.bmiHeader.biXPelsPerMeter = h.width * h.ary;
		vi->hdr.bmiHeader.biYPelsPerMeter = h.height * h.arx;
		vi->cbSequenceHeader = shlen + shextlen;
		Seek(shpos);
		ByteRead((BYTE*)&vi->bSequenceHeader[0], shlen);
		if(shextpos && shextlen) Seek(shextpos);
		ByteRead((BYTE*)&vi->bSequenceHeader[0] + shlen, shextlen);
		pmt->SetFormat((BYTE*)vi, len);
		delete [] vi;
	}
	else if(type == Mpeg2)
	{
		pmt->subtype = MEDIASUBTYPE_MPEG2_VIDEO;
		pmt->formattype = FORMAT_MPEG2_VIDEO;
		int len = FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + shlen + shextlen;
		MPEG2VIDEOINFO* vi = (MPEG2VIDEOINFO*)new BYTE[len];
		memset(vi, 0, len);
		vi->hdr.dwBitRate = h.bitrate;
		vi->hdr.AvgTimePerFrame = h.ifps;
		vi->hdr.dwPictAspectRatioX = h.arx;
		vi->hdr.dwPictAspectRatioY = h.ary;
		vi->hdr.bmiHeader.biSize = sizeof(vi->hdr.bmiHeader);
		vi->hdr.bmiHeader.biWidth = h.width;
		vi->hdr.bmiHeader.biHeight = h.height;
		vi->dwProfile = h.profile;
		vi->dwLevel = h.level;
		vi->cbSequenceHeader = shlen + shextlen;
		Seek(shpos);
		ByteRead((BYTE*)&vi->dwSequenceHeader[0], shlen);
		if(shextpos && shextlen) Seek(shextpos);
		ByteRead((BYTE*)&vi->dwSequenceHeader[0] + shlen, shextlen);
		pmt->SetFormat((BYTE*)vi, len);
		delete [] vi;
	}
	else
	{
		return false;
	}

	return true;
}

bool CBaseSplitterFile::Read(VC1Header& h, int len, CMediaType* pmt)
{
	__int64 endpos = GetPos() + len;

	BYTE id = 0;

	while(GetPos() < endpos && id != 0x0f)
	{
		if(!NextMpegStartCode(id, len))
		{
			return false;
		}
	}

	if(id != 0x0f)
	{
		return false;
	}

	__int64 start = GetPos() - 4 - 1;

	memset(&h, 0, sizeof(h));

	h.profile = (BYTE)BitRead(2);

	if(h.profile != 3) // advanced
	{
		return false;
	}

	h.level = (BYTE)BitRead(3);

	BYTE chromaformat = (BYTE)BitRead(2);
	BYTE frmrtq_postproc = (BYTE)BitRead(3);
	BYTE bitrtq_postproc = (BYTE)BitRead(5);
	BYTE postprocflag = BitRead();

	h.width = ((WORD)BitRead(12) + 1) << 1;
	h.height = ((WORD)BitRead(12) + 1) << 1;

	BYTE broadcast = BitRead();
	BYTE interlace = BitRead();
	BYTE tfcntrflag = BitRead();
	BYTE finterpflag = BitRead();
	
	BitRead(); // reserved

	BYTE psf = BitRead();

	if((BYTE)BitRead()) // display info
	{
		h.disp_width = (WORD)BitRead(14) + 1;
		h.disp_height = (WORD)BitRead(14) + 1;

		if(BitRead())
		{
			BYTE ar = (WORD)BitRead(4);

			if(ar > 0 && ar < 14)
			{
				static const struct {BYTE arx, ary;} ar_table[16] =
				{
					{0, 1}, {1, 1}, {12, 11}, {10, 11},
					{16, 11}, {40, 33}, {24, 11}, {20, 11},
					{32, 11}, {80, 33}, {18, 11}, {15, 11},
					{64, 33}, {160, 99}, {0, 1}, {0, 1}
				};

				h.arx = ar_table[ar].arx;
				h.ary = ar_table[ar].ary;
			}
			else
			{
				h.arx = (BYTE)BitRead(8);
				h.ary = (BYTE)BitRead(8);
			}
		}

		if(BitRead())
		{
			if(BitRead())
			{
				h.fps.num = 32;
				h.fps.denum = (DWORD)BitRead(8) + 1;
			}
			else
			{
				DWORD nr = (DWORD)BitRead(8);
				DWORD dr = (DWORD)BitRead(4);

				if(nr > 0 && nr < 8 && dr > 0 && dr < 3)
				{
					static const DWORD nr_table[] = {24, 25, 30, 50, 60};
					static const DWORD dr_table[] = {1000, 1001};

					h.fps.num = nr_table[nr - 1] * 1000;
					h.fps.denum = dr_table[dr - 1];
				}
			}
		}
	}

	if(BitRead()) // hrd_param_flag
	{
		BYTE num_leaky_buckets = (BYTE)BitRead(5);

		BYTE bitrate_exponent = (BYTE)BitRead(4);
		BYTE buffer_size_exponent = (BYTE)BitRead(4);

		for(int i = 0; i < num_leaky_buckets; i++)
		{
			BYTE hrd_rate = (WORD)BitRead(16);
			BYTE hrd_buffer = (WORD)BitRead(16);
		}
	}

	if(!pmt) return true;

	while(GetPos() < endpos && id != 0x0d)
	{
		if(!NextMpegStartCode(id, len))
		{
			return false;
		}
	}

	if(id != 0x0d)
	{
		return false;
	}

	int size = (GetPos() - 4) - start;

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = MEDIASUBTYPE_WVC1;
	pmt->formattype = FORMAT_VideoInfo;

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + size);

	memset(vih, 0, pmt->cbFormat);

	vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
	vih->bmiHeader.biCompression = pmt->subtype.Data1;
	vih->bmiHeader.biWidth = h.width;
	vih->bmiHeader.biHeight = h.height;

	if(h.fps.num > 0 && h.fps.denum > 0)
	{
		vih->AvgTimePerFrame = 10000000i64 * h.fps.denum / h.fps.num;
	}

	Seek(start);

	ByteRead((BYTE*)(vih + 1), size);

	return true;
}

bool CBaseSplitterFile::Read(MpegAudioHeader& h, int len, bool fAllowV25, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	int syncbits = fAllowV25 ? 11 : 12;

	for(; len >= 4 && BitRead(syncbits, true) != (1 << syncbits) - 1; len--)
	{
		BitRead(8);
	}

	if(len < 4)
	{
		return false;
	}

	h.start = GetPos();

	h.sync = BitRead(11);
	h.version = BitRead(2);
	h.layer = BitRead(2);
	h.crc = BitRead();
	h.bitrate = BitRead(4);
	h.freq = BitRead(2);
	h.padding = BitRead();
	h.privatebit = BitRead();
	h.channels = BitRead(2);
	h.modeext = BitRead(2);
	h.copyright = BitRead();
	h.original = BitRead();
	h.emphasis = BitRead(2);

	if(h.version == 1 || h.layer == 0 || h.freq == 3 || h.bitrate == 15 || h.emphasis == 2)
	{
		return false;
	}

	if(h.version == 3 && h.layer == 2)
	{
		if((h.bitrate == 1 || h.bitrate == 2 || h.bitrate == 3 || h.bitrate == 5) && h.channels != 3
		&& (h.bitrate >= 11 && h.bitrate <= 14) && h.channels == 3)
		{
			return false;
		}
	}

	h.layer = 4 - h.layer;

	//

	static int brtbl[][5] = 
	{
		{0, 0, 0, 0, 0},
		{32, 32, 32, 32, 8},
		{64, 48, 40, 48, 16},
		{96, 56, 48, 56, 24},
		{128, 64, 56, 64, 32},
		{160, 80, 64, 80, 40},
		{192, 96, 80, 96, 48},
		{224, 112, 96, 112, 56},
		{256, 128, 112, 128, 64},
		{288, 160, 128, 144, 80},
		{320, 192, 160, 160, 96},
		{352, 224, 192, 176, 112},
		{384, 256, 224, 192, 128},
		{416, 320, 256, 224, 144},
		{448, 384, 320, 256, 160},
		{0, 0, 0, 0, 0},
	};

	static int brtblcol[][4] = 
	{
		{0, 3, 4, 4}, 
		{0, 0, 1, 2}
	};

	int bitrate = 1000 * brtbl[h.bitrate][brtblcol[h.version & 1][h.layer]];
	
	if(bitrate == 0) 
	{
		return false;
	}

	static int freq[][4] = 
	{
		{11025, 0, 22050, 44100}, 
		{12000, 0, 24000, 48000},
		{8000, 0, 16000, 32000}
	};

	bool l3ext = h.layer == 3 && !(h.version & 1);

	h.nSamplesPerSec = freq[h.freq][h.version];

	h.FrameSize = h.layer == 1
		? (12 * bitrate / h.nSamplesPerSec + h.padding) * 4
		: (l3ext ? 72 : 144) * bitrate / h.nSamplesPerSec + h.padding;

	h.rtDuration = 10000000i64 * (h.layer == 1 ? 384 : l3ext ? 576 : 1152) / h.nSamplesPerSec; // / (h.channels == 3 ? 1 : 2);

	h.nBytesPerSec = bitrate / 8;

	h.bytes = h.FrameSize - 4;

	if(!pmt) return true;

	/*int*/ len = h.layer == 3 
		? sizeof(WAVEFORMATEX/*MPEGLAYER3WAVEFORMAT*/) // no need to overcomplicate this...
		: sizeof(MPEG1WAVEFORMAT);

	pmt->majortype = MEDIATYPE_Audio;
	pmt->formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)new BYTE[len];

	memset(wfe, 0, len);

	wfe->cbSize = len - sizeof(WAVEFORMATEX);

	if(h.layer == 3)
	{
		pmt->subtype = MEDIASUBTYPE_MPEG_AUDIO;

		wfe->wFormatTag = WAVE_FORMAT_MP3;
/*
		MPEGLAYER3WAVEFORMAT* f = (MPEGLAYER3WAVEFORMAT*)wfe;

		f->wfx.wFormatTag = WAVE_FORMAT_MP3;
		f->wID = MPEGLAYER3_ID_UNKNOWN;
		f->fdwFlags = h.padding ? MPEGLAYER3_FLAG_PADDING_ON : MPEGLAYER3_FLAG_PADDING_OFF; // _OFF or _ISO ?
*/
	}
	else
	{
		pmt->subtype = MEDIASUBTYPE_MP3;

		MPEG1WAVEFORMAT* f = (MPEG1WAVEFORMAT*)wfe;

		f->wfx.wFormatTag = WAVE_FORMAT_MPEG;
		f->fwHeadMode = 1 << h.channels;
		f->fwHeadModeExt = 1 << h.modeext;
		f->wHeadEmphasis = h.emphasis+1;
		if(h.privatebit) f->fwHeadFlags |= ACM_MPEG_PRIVATEBIT;
		if(h.copyright) f->fwHeadFlags |= ACM_MPEG_COPYRIGHT;
		if(h.original) f->fwHeadFlags |= ACM_MPEG_ORIGINALHOME;
		if(h.crc == 0) f->fwHeadFlags |= ACM_MPEG_PROTECTIONBIT;
		if(h.version == 3) f->fwHeadFlags |= ACM_MPEG_ID_MPEG1;
		f->fwHeadLayer = 1 << (h.layer-1);
		f->dwHeadBitrate = bitrate;
	}

	wfe->nChannels = h.channels == 3 ? 1 : 2;
	wfe->nSamplesPerSec = h.nSamplesPerSec;
	wfe->nBlockAlign = h.FrameSize;
	wfe->nAvgBytesPerSec = h.nBytesPerSec;

	pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

	delete [] wfe;

	return(true);
}

bool CBaseSplitterFile::Read(LATMHeader& h, int len, CMediaType* pmt, bool* nosync)
{
	memset(&h, 0, sizeof(h));

	DWORD frame_size = 0;

	do
	{
		DWORD sync = (DWORD)BitRead(32, true);

		if((sync >> 21) != 0x2b7)
		{
			return false;
		}

		frame_size = ((sync >> 8) & 0x1fff) + 3;

		if(frame_size == 0 || frame_size > len)
		{
			return false;
		}

		if((sync & 0x80) == 0)
		{
			break;
		}

		len -= frame_size;

		Seek(GetPos() + frame_size);
	}
	while(len > 0);

	if(len <= 0)
	{
		if(nosync) *nosync = true;

		return false;
	}

	NeAACDecHandle hDecoder = NeAACDecOpen();

	BYTE* buff = new BYTE[frame_size];

	ByteRead(buff, frame_size);

	if(NeAACDecInit(hDecoder, buff, frame_size, &h.rate, &h.channels) >= 0)
	{
		if(pmt)
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)new BYTE[sizeof(WAVEFORMATEX) + frame_size];
			memset(wfe, 0, sizeof(WAVEFORMATEX) + frame_size);
			wfe->wFormatTag = WAVE_FORMAT_LATM;
			wfe->nChannels = h.channels;
			wfe->nSamplesPerSec = h.rate;
			wfe->nBlockAlign = 0;
			wfe->nAvgBytesPerSec = 0;
			wfe->cbSize = frame_size;
			memcpy(wfe + 1, buff, frame_size);

			pmt->majortype = MEDIATYPE_Audio;
			pmt->subtype = MEDIASUBTYPE_LATM;
			pmt->formattype = FORMAT_WaveFormatEx;
			pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

			delete [] wfe;
		}

		NeAACDecClose(hDecoder);
	}

	delete [] buff;

	return h.rate > 0 && h.channels > 0;
}

bool CBaseSplitterFile::Read(AACHeader& h, int len, CMediaType* pmt)
{
	for(; len >= 7 && BitRead(12, true) != 0xfff; len--)
	{
		BitRead(8);
	}

	if(len < 7) 
	{
		return false;
	}

	h.start = GetPos();

	h.sync = BitRead(12);
	h.version = BitRead();
	h.layer = BitRead(2);
	h.fcrc = BitRead();
	h.profile = BitRead(2);
	h.freq = BitRead(4);
	h.privatebit = BitRead();
	h.channels = BitRead(3);
	h.original = BitRead();
	h.home = BitRead();

	h.copyright_id_bit = BitRead();
	h.copyright_id_start = BitRead();
	h.aac_frame_length = BitRead(13);
	h.adts_buffer_fullness = BitRead(11);
	h.no_raw_data_blocks_in_frame = BitRead(2);

	h.bytes = h.aac_frame_length - (h.fcrc == 0 ? 9 : 7);

	if(h.fcrc == 0) 
	{
		h.crc = BitRead(16);
	}

	if(h.layer != 0 || h.freq >= 12 || h.bytes <= 0)
	{
		return false;
	}

	if(h.aac_frame_length > h.channels * 6144 / 8)
	{
		return false;
	}

    static int freq[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};

	h.FrameSize = h.aac_frame_length - (h.fcrc == 0 ? 9 : 7);
	h.nBytesPerSec = h.aac_frame_length * freq[h.freq] / 1024; // ok?
	h.rtDuration = 10000000i64 * 1024 / freq[h.freq]; // ok?

	if(!pmt) return true;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_AAC;
	pmt->formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)new BYTE[sizeof(WAVEFORMATEX) + 5];

	memset(wfe, 0, sizeof(WAVEFORMATEX) + 5);

	wfe->wFormatTag = WAVE_FORMAT_AAC;
	wfe->nChannels = h.channels <= 6 ? h.channels : 2;
	wfe->nSamplesPerSec = freq[h.freq];
	wfe->nBlockAlign = h.aac_frame_length;
	wfe->nAvgBytesPerSec = h.nBytesPerSec;
	wfe->cbSize = DirectShow::AACInitData((BYTE*)(wfe + 1), h.profile, wfe->nSamplesPerSec, wfe->nChannels);

	pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

	delete [] wfe;

	return true;
}

bool CBaseSplitterFile::Read(AC3Header& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	for(; len >= 7 && BitRead(16, true) != 0x0b77; len--)
	{
		BitRead(8);
	}

	if(len < 7)
	{
		return false;
	}

	h.sync = (WORD)BitRead(16);
	h.crc1 = (WORD)BitRead(16);
	h.fscod = BitRead(2);
	h.frmsizecod = BitRead(6);
	h.bsid = BitRead(5);
	h.bsmod = BitRead(3);
	h.acmod = BitRead(3);
	if((h.acmod & 1) && h.acmod != 1) h.cmixlev = BitRead(2);
	if(h.acmod & 4) h.surmixlev = BitRead(2);
	if(h.acmod == 2) h.dsurmod = BitRead(2);
	h.lfeon = BitRead();

	if(h.bsid >= 12 || h.fscod == 3 || h.frmsizecod >= 38)
	{
		return false;
	}

	if(!pmt) return true;

	static int channels[] = {2, 1, 2, 3, 3, 4, 4, 5};
	static int freq[] = {48000, 44100, 32000, 0};
	static int rate[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};

	WAVEFORMATEX wfe;

	memset(&wfe, 0, sizeof(wfe));

	wfe.wFormatTag = WAVE_FORMAT_DOLBY_AC3;
	wfe.nChannels = channels[h.acmod] + h.lfeon;
	wfe.nSamplesPerSec = freq[h.fscod];

	switch(h.bsid)
	{
	case 9: wfe.nSamplesPerSec >>= 1; break;
	case 10: wfe.nSamplesPerSec >>= 2; break;
	case 11: wfe.nSamplesPerSec >>= 3; break;
	default: break;
	}

	wfe.nAvgBytesPerSec = rate[h.frmsizecod >> 1] * 1000 / 8;
	wfe.nBlockAlign = (WORD)(1536 * wfe.nAvgBytesPerSec / wfe.nSamplesPerSec);

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_DOLBY_AC3;
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	return true;
}

bool CBaseSplitterFile::Read(DTSHeader& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	for(; len >= 10 && BitRead(32, true) != 0x7ffe8001; len--)
	{
		BitRead(8);
	}

	if(len < 10)
	{
		return false;
	}

	h.sync = (DWORD)BitRead(32);
	h.frametype = BitRead();
	h.deficitsamplecount = BitRead(5);
	h.fcrc = BitRead();
	h.nblocks = BitRead(7);
	h.framebytes = (WORD)BitRead(14)+1;
	h.amode = BitRead(6);
	h.sfreq = BitRead(4);
	h.rate = BitRead(5);

	if(!pmt) return true;

	static int channels[] = 
	{
		1, 2, 2, 2, 2, 3, 3, 4, 
		4, 5, 6, 6, 6, 7, 8, 8
	};

	static int freq[] = 
	{
		0, 8000, 16000, 32000, 0, 0, 11025, 22050, 
		44100, 0, 0, 12000, 24000, 48000, 0, 0
	};

	static int rate[] = 
	{
		32000, 56000, 64000, 96000, 112000, 128000, 192000, 224000,
		256000, 320000, 384000, 448000, 512000, 576000, 640000, 754500,
		960000, 1024000, 1152000, 1280000, 1344000, 1408000, 1411200, 1472000,
		1509750, 1920000, 2048000, 3072000, 3840000, 0, 0, 0
	};

	WAVEFORMATEX wfe;

	memset(&wfe, 0, sizeof(wfe));

	wfe.wFormatTag = WAVE_FORMAT_DVD_DTS;
	if(h.amode < countof(channels)) wfe.nChannels = channels[h.amode];
	wfe.nSamplesPerSec = freq[h.sfreq];
	
	wfe.nAvgBytesPerSec = rate[h.rate] / 8;
	wfe.nBlockAlign = h.framebytes;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_DTS;
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	return true;
}

bool CBaseSplitterFile::Read(LPCMHeader& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	h.emphasis = BitRead();
	h.mute = BitRead();
	h.reserved1 = BitRead();
	h.framenum = BitRead(5);
	h.quantwordlen = BitRead(2);
	h.freq = BitRead(2);
	h.reserved2 = BitRead();
	h.channels = BitRead(3);
	h.drc = (BYTE)BitRead(8);

	if(h.channels > 2 || h.reserved1 || h.reserved2)
	{
		return false;
	}

	if(!pmt) return true;

	static int freq[] = {48000, 96000, 44100, 32000};

	WAVEFORMATEX wfe;

	memset(&wfe, 0, sizeof(wfe));

	wfe.wFormatTag = WAVE_FORMAT_PCM;
	wfe.nChannels = h.channels + 1;
	wfe.nSamplesPerSec = freq[h.freq];
	wfe.wBitsPerSample = 16; // TODO: 20, 24
	wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample >> 3;
	wfe.nAvgBytesPerSec = wfe.nBlockAlign * wfe.nSamplesPerSec;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_DVD_LPCM_AUDIO;
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	// TODO: what to do with dvd-audio lpcm?

	return true;
}

bool CBaseSplitterFile::Read(BPCMHeader& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	h.unknown = BitRead(16);
	h.channels = BitRead(4);
	h.freq = BitRead(4);
	h.bps = BitRead(2);
	h.reserved = BitRead(6);

	static int channels[] = 
	{
		0, 
		1, // c
		0, 
		2, // l r
		3, // l r c
		3, // l r cb
		4, // l r cb
		4, // l r lb rb
		5, // l r c lb rb
		6, // l r c lb rb lfe
		7, // l r c lb rb lm rm
		8, // l r c lb rb lm rm lfe
		0, 0, 0, 0,
	};

	static int chmask[] = 
	{
		0,
		SPEAKER_FRONT_CENTER,
		0,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_LOW_FREQUENCY,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT,
		SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT|SPEAKER_LOW_FREQUENCY,
		0,
	};

	static int freq[] = 
	{
		0, 48000, 0, 0,
		96000, 192000, 0, 0,
		0, 0, 0, 0, 
		0, 0, 0, 0, 
	};

	static int bps[] = 
	{
		0, 16, 20, 24, 
	};

	if(channels[h.channels] == 0 || freq[h.freq] == 0 || bps[h.bps] == 0)
	{
		return false;
	}

	if(!pmt) return true;

	if(channels[h.channels] > 2 || bps[h.bps] > 16)
	{
		// TODO
	}

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_PCM;
	pmt->formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));

	memset(wfe, 0, pmt->FormatLength());

	wfe->wFormatTag = WAVE_FORMAT_PCM;
	wfe->nChannels = channels[h.channels];
	wfe->nSamplesPerSec = freq[h.freq];
	wfe->wBitsPerSample = bps[h.bps] == 20 ? 24 : bps[h.bps]; // round up 20 bps to 24
	wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample >> 3;
	wfe->nAvgBytesPerSec = wfe->nBlockAlign * wfe->nSamplesPerSec;

	if(wfe->nChannels > 2)
	{
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)pmt->ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));

		wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfex->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		wfex->dwChannelMask = chmask[h.channels];
		wfex->Samples.wValidBitsPerSample = wfex->Format.wBitsPerSample;
		wfex->SubFormat = pmt->subtype;
	}

	return true;
}

bool CBaseSplitterFile::Read(DVDSubpictureHeader& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(!pmt) return true;

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = MEDIASUBTYPE_DVD_SUBPICTURE;
	pmt->formattype = FORMAT_None;

	return true;
}

bool CBaseSplitterFile::Read(SVCDSubpictureHeader& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(!pmt) return true;

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = MEDIASUBTYPE_SVCD_SUBPICTURE;
	pmt->formattype = FORMAT_None;

	return true;
}

bool CBaseSplitterFile::Read(CVDSubpictureHeader& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(!pmt) return true;

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = MEDIASUBTYPE_CVD_SUBPICTURE;
	pmt->formattype = FORMAT_None;

	return true;
}

bool CBaseSplitterFile::Read(PS2AudioHeader& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(BitRead(16, true) != 'SS')
	{
		return false;
	}

	__int64 pos = GetPos();

	while(BitRead(16, true) == 'SS')
	{
		DWORD tag = (DWORD)BitRead(32, true);
		DWORD size = 0;
		
		if(tag == 'SShd')
		{
			BitRead(32);
			ByteRead((BYTE*)&size, sizeof(size));
			ASSERT(size == 0x18);
			Seek(GetPos());
			ByteRead((BYTE*)&h, sizeof(h));
		}
		else if(tag == 'SSbd')
		{
			BitRead(32);
			ByteRead((BYTE*)&size, sizeof(size));
			break;
		}
	}

	Seek(pos);

	if(!pmt) return true;

	WAVEFORMATEXPS2 wfe;

	wfe.wFormatTag = 
		h.unk1 == 0x01 ? WAVE_FORMAT_PS2_PCM : 
		h.unk1 == 0x10 ? WAVE_FORMAT_PS2_ADPCM :
		WAVE_FORMAT_UNKNOWN;
	wfe.nChannels = (WORD)h.channels;
	wfe.nSamplesPerSec = h.freq;
	wfe.wBitsPerSample = 16; // always?
	wfe.nBlockAlign = wfe.nChannels*wfe.wBitsPerSample>>3;
	wfe.nAvgBytesPerSec = wfe.nBlockAlign*wfe.nSamplesPerSec;
	wfe.dwInterleave = h.interleave;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = FOURCCMap(wfe.wFormatTag);
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	return true;
}

bool CBaseSplitterFile::Read(PS2SubpictureHeader& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(!pmt) return true;

	pmt->majortype = MEDIATYPE_Subtitle;
	pmt->subtype = MEDIASUBTYPE_PS2_SUB;
	pmt->formattype = FORMAT_None;

	return true;
}

bool CBaseSplitterFile::Read(TSHeader& h, bool sync)
{
	memset(&h, 0, sizeof(h));

	BitByteAlign();

	if(m_tslen == 0)
	{
		__int64 pos = GetPos();

		for(int i = 0; i < 192; i++)
		{
			if(BitRead(8, true) == 0x47)
			{
				__int64 pos = GetPos();
				Seek(pos + 188);
				if(BitRead(8, true) == 0x47) {m_tslen = 188; break;}
				Seek(pos + 192);
				if(BitRead(8, true) == 0x47) {m_tslen = 192; break;}
			}

			BitRead(8);
		}

		Seek(pos);

		if(m_tslen == 0)
		{
			return false;
		}
	}

	if(sync)
	{
		for(int i = 0; i < m_tslen; i++)
		{
			if(BitRead(8, true) == 0x47)
			{
				if(i == 0) break;
				Seek(GetPos() + m_tslen);
				if(BitRead(8, true) == 0x47) {Seek(GetPos() - m_tslen); break;}
			}

			BitRead(8);

			if(i == m_tslen - 1)
			{
				return false;
			}
		}
	}

	if(BitRead(8, true) != 0x47)
	{
		return false;
	}

	h.start = GetPos();
	h.next = h.start + m_tslen;
	h.bytes = 188 - 4;

	DWORD dw = (DWORD)BitRead(32);

	h.sync = dw >> 24;
	h.error = (dw >> 23) & 1;

	if(!h.error)
	{
		h.payloadstart = (dw >> 22) & 1;
		h.transportpriority = (dw >> 21) & 1;
		h.pid = (dw >> 8) & 0x1fff;
		h.flags = (BYTE)dw;

		if(h.adapfield)
		{
			h.length = (BYTE)BitRead(8);

			if(h.length > 0)
			{
				h.adapfield_value = (BYTE)BitRead(8);

				int i = 1;

				if(h.fpcr)
				{
					h.pcr = BitRead(33);
					BitRead(6);
					h.pcr = (h.pcr * 300 + BitRead(9)) * 10 / 27;
					i += 6;
				}

				// ASSERT(i <= h.length);

				for(; i < h.length; i++)
				{
					BitRead(8);
				}
			}

			h.bytes -= h.length + 1;

			if(h.bytes < 0) 
			{
				return false;
			}
		}
	}

	return true;
}

bool CBaseSplitterFile::Read(TSSectionHeader& h)
{
	BYTE pointer_field = BitRead(8);
	while(pointer_field-- > 0) BitRead(8);
	h.table_id = BitRead(8);
	h.section_syntax_indicator = BitRead();
	h.zero = BitRead();
	h.reserved1 = BitRead(2);
	h.section_length = BitRead(12);
	h.transport_stream_id = BitRead(16);
	h.reserved2 = BitRead(2);
	h.version_number = BitRead(5);
	h.current_next_indicator = BitRead();
	h.section_number = BitRead(8);
	h.last_section_number = BitRead(8);

	if(h.section_number > h.last_section_number)
	{
		return false;
	}

	if(h.section_syntax_indicator && h.last_section_number > 0 || h.section_length > 177)
	{
		int i = 0;
	}

	return h.section_syntax_indicator == 1 /*&& h.zero == 0 && h.reserved1 == 3 && h.reserved2 == 3*/;
}

bool CBaseSplitterFile::Read(PVAHeader& h, bool sync)
{
	memset(&h, 0, sizeof(h));

	BitByteAlign();

	if(sync)
	{
		for(int i = 0; i < 65536; i++)
		{
			if((BitRead(64, true) & 0xfffffc00ffe00000i64) == 0x4156000055000000i64) 
			{
				break;
			}

			BitRead(8);
		}
	}

	if((BitRead(64, true)&0xfffffc00ffe00000i64) != 0x4156000055000000i64)
	{
		return false;
	}

	h.sync = (WORD)BitRead(16);
	h.streamid = (BYTE)BitRead(8);
	h.counter = (BYTE)BitRead(8);
	h.res1 = (BYTE)BitRead(8);
	h.res2 = BitRead(3);
	h.fpts = BitRead();
	h.postbytes = BitRead(2);
	h.prebytes = BitRead(2);
	h.length = (WORD)BitRead(16);

	if(h.length > 6136)
	{
		return false;
	}

	__int64 pos = GetPos();

	if(h.streamid == 1 && h.fpts)
	{
		h.pts = 10000*BitRead(32)/90;
	}
	else if(h.streamid == 2 && (h.fpts || (BitRead(32, true)&0xffffffe0) == 0x000001c0))
	{
		BYTE b;
		if(!NextMpegStartCode(b, 4)) return false;
		PESHeader h2;
		if(!Read(h2, b)) return false;
		if(h.fpts = h2.fpts) h.pts = h2.pts;
	}

	BitRead(8*h.prebytes);

	h.length -= GetPos() - pos;

	return true;
}

bool CBaseSplitterFile::Read(AVCHeader& h, int len, CMediaType* pmt) // TODO: get rid of this, use Read(H264Header...
{
	__int64 endpos = GetPos() + len; // - sequence header length

	if(endpos > GetLength()) endpos = GetLength();

	__int64 spspos = -1, spslen = 0;
	__int64 ppspos = -1, ppslen = 0;

	while(GetPos() <= endpos - 5 && BitRead(32, true) == 0x00000000)
	{
		BitRead(8);
	}

	while(GetPos() <= endpos - 5 && BitRead(32, true) == 0x00000001)
	{
		__int64 pos = GetPos();

		BitRead(32);

		BYTE id = BitRead(8);

		if(spspos >= 0 && spslen == 0) spslen = pos - spspos;
		else if(ppspos >= 0 && ppslen == 0) ppslen = pos - ppspos;
		
		if((id & 0x9f) == 0x07 && (id & 0x60) != 0)
		{
			spspos = pos;

			Read(h.sps);
		}
		else if((id & 0x9f) == 0x08 && (id & 0x60) != 0)
		{
			ppspos = pos;
		}

		BitByteAlign();

		while(GetPos() <= endpos - 5 && BitRead(32, true) != 0x00000001)
		{
			BitRead(8);
		}
	}

	if(spspos < 0 || spslen == 0 || ppspos < 0 || ppslen == 0) 
	{
		return false;
	}

	if(!pmt) return true;

	{
		int extra = 2+spslen-4 + 2+ppslen-4;

		pmt->majortype = MEDIATYPE_Video;
		pmt->subtype = FOURCCMap('1CVA');
		pmt->formattype = FORMAT_MPEG2_VIDEO;
		int len = FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + extra;
		MPEG2VIDEOINFO* vi = (MPEG2VIDEOINFO*)new BYTE[len];
		memset(vi, 0, len);
		// vi->hdr.dwBitRate = ;
		// vi->hdr.AvgTimePerFrame = ;
		vi->hdr.dwPictAspectRatioX = h.sps.width * h.sps.sar_width / h.sps.sar_height;
		vi->hdr.dwPictAspectRatioY = h.sps.height;
		vi->hdr.bmiHeader.biSize = sizeof(vi->hdr.bmiHeader);
		vi->hdr.bmiHeader.biWidth = h.sps.width;
		vi->hdr.bmiHeader.biHeight = h.sps.height;
		vi->hdr.bmiHeader.biCompression = '1CVA';
		vi->dwProfile = h.sps.profile;
		vi->dwFlags = 4; // ?
		vi->dwLevel = h.sps.level;
		vi->cbSequenceHeader = extra;
		BYTE* p = (BYTE*)&vi->dwSequenceHeader[0];
		*p++ = (spslen - 4) >> 8;
		*p++ = (spslen  - 4) & 0xff;
		Seek(spspos+4);
		ByteRead(p, spslen-4);
		p += spslen-4;
		*p++ = (ppslen - 4) >> 8;
		*p++ = (ppslen - 4) & 0xff;
		Seek(ppspos + 4);
		ByteRead(p, ppslen- 4);
		p += ppslen - 4;
		pmt->SetFormat((BYTE*)vi, len);
		delete [] vi;
	}

	return true;
}

bool CBaseSplitterFile::Read(H264Header& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	__int64 endpos = GetPos() + len; // - sequence header length

	if(endpos > GetLength()) endpos = GetLength();

	while(GetPos() <= endpos - 5 && BitRead(32, true) == 0x00000000)
	{
		BitRead(8);
	}

	while(GetPos() <= endpos - 5 && BitRead(32, true) == 0x00000001)
	{
		BitRead(32);
		BYTE id = BitRead(8);
		
		if((id & 0x9f) == 0x07 && (id & 0x60) != 0)
		{
			Read(h.sps);

			break;
		}

		BitByteAlign();

		while(GetPos() <= endpos - 5 && BitRead(32, true) != 0x00000001)
		{
			BitRead(8);
		}
	}

	if(h.sps.width <= 0 || h.sps.height <= 0) 
	{
		return false;
	}

	if(pmt)
	{
		pmt->majortype = MEDIATYPE_Video;
		pmt->subtype = MEDIASUBTYPE_H264;
		pmt->formattype = FORMAT_VideoInfo2;
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memset(vih, 0, sizeof(VIDEOINFOHEADER2));
		vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
		vih->bmiHeader.biCompression = '462H';
		vih->bmiHeader.biWidth = h.sps.width;
		vih->bmiHeader.biHeight = h.sps.height;
		vih->dwPictAspectRatioX = h.sps.width * h.sps.sar_width / h.sps.sar_height;
		vih->dwPictAspectRatioY = h.sps.height;
		// vih->dwBitRate = ;
		// vih->AvgTimePerFrame = ;

		if(vih->bmiHeader.biHeight == 1088)
		{
			vih->bmiHeader.biHeight = 1080;
			vih->dwPictAspectRatioY = 1080;
		}
	}

	return true;
}

bool CBaseSplitterFile::Read(H264SPS& h)
{
	memset(&h, 0, sizeof(h));

	h.profile = (BYTE)BitRead(8);
	BitRead(8);
	h.level = (BYTE)BitRead(8);

	UExpGolombRead(); // seq_parameter_set_id

	if(h.profile >= 100) // high profile
	{
		if(UExpGolombRead() == 3) // chroma_format_idc
		{
			BitRead(); // residue_transform_flag
		}

		UExpGolombRead(); // bit_depth_luma_minus8
		UExpGolombRead(); // bit_depth_chroma_minus8

		BitRead(); // qpprime_y_zero_transform_bypass_flag

		if(BitRead()) // seq_scaling_matrix_present_flag
			for(int i = 0; i < 8; i++)
				if(BitRead()) // seq_scaling_list_present_flag
					for(int j = 0, size = i < 6 ? 16 : 64, next = 8; j < size && next != 0; ++j)
						next = (next + SExpGolombRead() + 256) & 255;
	}

	UExpGolombRead(); // log2_max_frame_num_minus4

	UINT64 pic_order_cnt_type = UExpGolombRead();

	if(pic_order_cnt_type == 0)
	{
		UExpGolombRead(); // log2_max_pic_order_cnt_lsb_minus4
	}
	else if(pic_order_cnt_type == 1)
	{
		BitRead(); // delta_pic_order_always_zero_flag
		SExpGolombRead(); // offset_for_non_ref_pic
		SExpGolombRead(); // offset_for_top_to_bottom_field
		UINT64 num_ref_frames_in_pic_order_cnt_cycle = UExpGolombRead();
		for(int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
			SExpGolombRead(); // offset_for_ref_frame[i]
	}

	UExpGolombRead(); // num_ref_frames
	BitRead(); // gaps_in_frame_num_value_allowed_flag

	UINT64 pic_width_in_mbs_minus1 = UExpGolombRead();
	UINT64 pic_height_in_map_units_minus1 = UExpGolombRead();
	BYTE frame_mbs_only_flag = (BYTE)BitRead();

	h.width = (pic_width_in_mbs_minus1 + 1) * 16;
	h.height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;
	h.sar_width = 1;
	h.sar_height = 1;

	if(!frame_mbs_only_flag)
	{
		BitRead();
	}

	BitRead();

	if(BitRead())
	{
		UExpGolombRead();
		UExpGolombRead();
		UExpGolombRead();
		UExpGolombRead();
	}

	if(BitRead()) // vui
	{
		if(BitRead()) // ar info present
		{
			BYTE ar_idc = (BYTE)BitRead(8);

			if(ar_idc == 255)
			{
				h.sar_width = (DWORD)BitRead(16);
				h.sar_height = (DWORD)BitRead(16);
			}
			else
			{
				static struct {DWORD w, h;} ar_table[] = 
				{
					{0, 0}, {1, 1}, {12, 11}, {10, 11}, 
					{16, 11}, {40, 33}, {24, 11}, {20, 11}, 
					{32, 11}, {80, 33}, {18, 11}, {15, 11}, 
					{64, 33}, {160, 99}, {4, 3}, {3, 2}, 
					{2, 1},
				};

				if(ar_idc > 0 && ar_idc < countof(ar_table))
				{
					h.sar_width = ar_table[ar_idc].w;
					h.sar_height = ar_table[ar_idc].h;
				}
			}
		}

		if(BitRead()) // overscan info present
		{
			BitRead(); // overscan appropriate
		}

		if(BitRead()) // video signal type present
		{
			h.video_format = BitRead(3); // video format
			h.video_full_range = BitRead(); // video full range flag

			if(BitRead()) // color description present flag
			{
				BitRead(8); // color primaries
				BitRead(8); // transfer characteristic
				BitRead(8); // matrix coefficients
			}
		}
	}

	return true;
}

bool CBaseSplitterFile::Read(H265Header& h, int len, CMediaType* pmt)
{
	return false;

	// TODO

	/*
Video: HEVC 960x540 50fps

AM_MEDIA_TYPE: 
majortype: MEDIATYPE_Video {73646976-0000-0010-8000-00AA00389B71}
subtype: Unknown GUID Name {43564548-0000-0010-8000-00AA00389B71}
formattype: FORMAT_MPEG2_VIDEO {E06D80E3-DB46-11CF-B4D1-00805F6CBBEA}
bFixedSizeSamples: 0
bTemporalCompression: 1
lSampleSize: 1
cbFormat: 401

VIDEOINFOHEADER:
rcSource: (0,0)-(960,540)
rcTarget: (0,0)-(960,540)
dwBitRate: 0
dwBitErrorRate: 0
AvgTimePerFrame: 200000

VIDEOINFOHEADER2:
dwInterlaceFlags: 0x00000000
dwCopyProtectFlags: 0x00000000
dwPictAspectRatioX: 16
dwPictAspectRatioY: 9
dwControlFlags: 0x00000000
dwReserved2: 0x00000000

MPEG2VIDEOINFO:
dwStartTimeCode: 0
cbSequenceHeader: 269
dwProfile: 0x00000001
dwLevel: 0x0000005d
dwFlags: 0x00000000

BITMAPINFOHEADER:
biSize: 40
biWidth: 960
biHeight: 540
biPlanes: 1
biBitCount: 12
biCompression: HEVC
biSizeImage: 777600
biXPelsPerMeter: 0
biYPelsPerMeter: 0
biClrUsed: 0
biClrImportant: 0

pbFormat:
0000: 00 00 00 00 00 00 00 00 c0 03 00 00 1c 02 00 00
0010: 00 00 00 00 00 00 00 00 c0 03 00 00 1c 02 00 00
0020: 00 00 00 00 00 00 00 00 40 0d 03 00 00 00 00 00
0030: 00 00 00 00 00 00 00 00 10 00 00 00 09 00 00 00
0040: 00 00 00 00 00 00 00 00 28 00 00 00 c0 03 00 00
0050: 1c 02 00 00 01 00 0c 00 48 45 56 43 80 dd 0b 00
0060: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0070: 00 00 00 00 0d 01 00 00 01 00 00 00 5d 00 00 00
0080: 00 00 00 00|00 00 01 46 01 10 00 00 00 01 40 01
0090: 0c 01 ff ff 01 60 00 00 03 00 b0 00 00 03 00 00
00a0: 03 00 5d 08 11 02 40 00 00 00 01 42 01 01 01 60
00b0: 00 00 03 00 b0 00 00 03 00 00 03 00 5d a0 07 82
00c0: 00 88 7d e3 42 04 64 91 be 10 ff fe 45 08 7f ff
00d0: 22 84 3f ff ff ff ff ff ff ff 91 42 1f ff ff ff
00e0: ff ff ff ff c8 a1 0f ff ff ff ff ff ff ff f2 28
00f0: 43 ff ff ff ff ff ff ff fc 8a 10 ff ff ff ff ff
0100: ff ff ff 84 3f ff ff ff ff ff ff ff d5 e0 28 40
0110: 40 40 79 fc 00 00 03 00 04 00 00 03 00 c8 20 00
0120: 00 00 01 44 01 c0 72 4f 87 64 00 00 00 01 44 01
0130: 50 1e 93 e1 d9 00 00 00 01 44 01 70 1e 93 e1 f0
0140: 61 83 8f ff 22 83 0c 1c 7f f9 14 18 7f c1 c7 ff
0150: ff ff ff ff ff c8 a0 c3 fe 0e 3f ff ff ff ff ff
0160: fe 45 06 1f f8 38 ff ff ff ff ff ff f9 14 18 7f
0170: e0 e3 ff ff ff ff ff ff e4 50 61 ff 83 8f ff ff
0180: ff ff ff ff c1 87 fe 0e 3f ff ff ff ff ff ff 90
0190: 00                                             

    HEVC_NAL_VPS            = 32,
    HEVC_NAL_SPS            = 33,
    HEVC_NAL_PPS            = 34,
    HEVC_NAL_AUD            = 35,

    if (get_bits1(gb) != 0)
        return AVERROR_INVALIDDATA;

    nal->type = get_bits(gb, 6);

    nal->nuh_layer_id = get_bits(gb, 6);
    nal->temporal_id = get_bits(gb, 3) - 1;
    if (nal->temporal_id < 0)
        return AVERROR_INVALIDDATA;

	ffmpeg\libavcodec\hevc_ps.c: ff_hevc_parse_sps, ff_hevc_decode_nal_pps, ff_hevc_decode_nal_vps

	*/

	memset(&h, 0, sizeof(h));

	if(pmt)
	{
		pmt->majortype = MEDIATYPE_Video;
		pmt->subtype = FOURCCMap('CVEH');
		pmt->formattype = FORMAT_MPEG2_VIDEO;
		MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)pmt->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO));
		memset(vih, 0, sizeof(MPEG2VIDEOINFO));
		vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
		vih->hdr.bmiHeader.biCompression = 'CVEH';
		vih->hdr.bmiHeader.biWidth = 960;
		vih->hdr.bmiHeader.biHeight = 540;
		vih->hdr.dwPictAspectRatioX = 16;
		vih->hdr.dwPictAspectRatioY = 9;
		// vih->dwBitRate = ;
		// vih->AvgTimePerFrame = ;

		if(vih->hdr.bmiHeader.biHeight == 1088)
		{
			vih->hdr.bmiHeader.biHeight = 1080;
			vih->hdr.dwPictAspectRatioY = 1080;
		}

		vih->dwProfile = 1;
		vih->dwLevel = 0x5d;
		vih->dwFlags = 0;
	}

	return true;
}

bool CBaseSplitterFile::Read(FLVPacket& h, bool first)
{
	if(GetRemaining() < 16) 
	{
		return false;
	}

	h.prevsize = !first ? (DWORD)BitRead(32) : 0;
	h.type = (BYTE)BitRead(8);
	h.size = (DWORD)BitRead(24);
	h.timestamp = (DWORD)BitRead(24);
	h.timestamp |= (DWORD)BitRead(8) << 24;
	h.streamid = (DWORD)BitRead(24);

	return true;
}

bool CBaseSplitterFile::Read(FLVVideoPacket& h, int len, CMediaType* pmt)
{
	if(GetRemaining() < 1) 
	{
		return false;
	}

	__int64 start = GetPos();

	h.type = (BYTE)BitRead(4);
	h.codec = (BYTE)BitRead(4);

	if(pmt != NULL)
	{
		if(h.type != 1)
		{
			return false;
		}

		pmt->majortype = MEDIATYPE_Video;
		pmt->formattype = FORMAT_VideoInfo;

		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));

		memset(vih, 0, sizeof(VIDEOINFOHEADER));

		BITMAPINFOHEADER* bih = &vih->bmiHeader;

		// int w, h, arx, ary;

		switch(h.codec)
		{
		case FLVVideoPacket::H263:

			if(BitRead(17) != 1)
			{
				return false;
			}

			BitRead(13); // Version (5), TemporalReference (8)

			switch(BYTE PictureSize = (BYTE)BitRead(3)) // w00t
			{
			case 0: case 1:
				vih->bmiHeader.biWidth = (WORD)BitRead(8 * (PictureSize + 1));
				vih->bmiHeader.biHeight = (WORD)BitRead(8 * (PictureSize + 1));
				break;
			case 2: case 3: case 4: 
				vih->bmiHeader.biWidth = 704 / PictureSize;
				vih->bmiHeader.biHeight = 576 / PictureSize;
				break;
			case 5: case 6: 
				PictureSize -= 3;
				vih->bmiHeader.biWidth = 640 / PictureSize;
				vih->bmiHeader.biHeight = 480 / PictureSize;
				break;
			}

			if(vih->bmiHeader.biWidth == 0 || vih->bmiHeader.biHeight == 0) 
			{
				return false;
			}

			pmt->subtype = FOURCCMap(vih->bmiHeader.biCompression = '1VLF');

			break;

		case FLVVideoPacket::VP6A:
			
			BitRead(24);
			
			// fall through

		case FLVVideoPacket::VP6:

			{
				BYTE crop_x = (BYTE)BitRead(4);
				BYTE crop_y = (BYTE)BitRead(4);

				if(BitRead() != 0) // keyframe?
				{
					return false;
				}

				BYTE dequant = (BYTE)BitRead(6);
				BYTE separated_coeff = (BYTE)BitRead();

				BYTE sub_version = (BYTE)BitRead(5); // TODO: if > 8 break
				BYTE filter_header = (BYTE)BitRead(2);
				BYTE interlaced = (BYTE)BitRead();

				if(separated_coeff && !filter_header)
				{
					BitRead(16);
				}

				int height = BitRead(8) * 16;
				int width = BitRead(8) * 16;

				int ary = BitRead(8) * 16;
				int arx = BitRead(8) * 16;

				if(arx != 0 && arx != width || ary != 0 && ary != height)
				{
					VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));

					memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

					vih2->dwPictAspectRatioX = arx;
					vih2->dwPictAspectRatioY = ary;

					bih = &vih2->bmiHeader;						
				}

				bih->biWidth = width;
				bih->biHeight = height;

				pmt->subtype = FOURCCMap(bih->biCompression = '4VLF');
			}

			break;

		case FLVVideoPacket::AVC:

			{
				if(BitRead(8) != 0)
				{
					return false;
				}

				BitRead(24);

				UINT64 size = len - (GetPos() - start);

				std::vector<BYTE> buff;

				// AVCDecoderConfigurationRecord

				if(BitRead(8) != 1) // version
				{
					return false;
				}

				BYTE profile = (BYTE)BitRead(8);
				BYTE profile_compatibility = (BYTE)BitRead(8);
				BYTE level = (BYTE)BitRead(8);
				
				if(BitRead(6) != 0x3f)
				{
					// return false; // TODO: not always 0x3f
				}

				BYTE length = (BYTE)BitRead(2) + 1;

				if(BitRead(3) != 0x07 && length != 4) // reserved should be 111b, but sometimes it isn't, if length == 4 we may continue for now
				{
					return false;
				}

				BYTE sps_count = (BYTE)BitRead(5);

				while(sps_count-- > 0)
				{
					buff.push_back(0);
					buff.push_back(0);
					buff.push_back(0);
					buff.push_back(1);

					WORD sps_length = (WORD)BitRead(16);

					while(sps_length-- > 0)
					{
						buff.push_back((BYTE)BitRead(8));
					}
				}

				BYTE pps_count = (BYTE)BitRead(8);

				while(pps_count-- > 0)
				{
					buff.push_back(0);
					buff.push_back(0);
					buff.push_back(0);
					buff.push_back(1);

					WORD pps_length = (WORD)BitRead(16);

					while(pps_length-- > 0)
					{
						buff.push_back((BYTE)BitRead(8));
					}
				}

				buff.push_back(0);
				buff.push_back(0);
				buff.push_back(0);
				buff.push_back(1);
				buff.push_back(0);

				CBaseSplitterFile f(&buff[0], buff.size());

				{
					CBaseSplitterFile::AVCHeader h;

					CMediaType mt;

					if(f.Read(h, f.GetLength(), &mt))
					{
						((MPEG2VIDEOINFO*)mt.pbFormat)->dwFlags = length;

						*pmt = mt;
					}
				}
			}

			break;
		}

		if(pmt->subtype == GUID_NULL)
		{
			return false;
		}
	}

	return true;
}

bool CBaseSplitterFile::Read(FLVAudioPacket& h, int len, CMediaType* pmt)
{
	if(GetRemaining() < 1) 
	{
		return false;
	}

	__int64 start = GetPos();

	h.format = (BYTE)BitRead(4);
	h.rate = (BYTE)BitRead(2);
	h.bits = (BYTE)BitRead();
	h.stereo = (BYTE)BitRead();

	if(pmt != NULL)
	{
		pmt->majortype = MEDIATYPE_Audio;
		pmt->formattype = FORMAT_WaveFormatEx; 

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));

		memset(wfe, 0, sizeof(WAVEFORMATEX));

		wfe->nSamplesPerSec = 44100 * (1 << h.rate) / 8;
		wfe->wBitsPerSample = 8 * (h.bits + 1);
		wfe->nChannels = h.stereo ? 2 : 1;

		switch(h.format)
		{
		case FLVAudioPacket::LPCMBE:
		case FLVAudioPacket::LPCMLE:

			pmt->subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_PCM);

			break;

		case FLVAudioPacket::MP3:

			pmt->subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MP3);

			{
				CBaseSplitterFile::MpegAudioHeader h;

				CMediaType mt;

				if(Read(h, 4, false, &mt))
				{
					*pmt = mt;
				}
			}

			break;

		case FLVAudioPacket::AAC:

			if(BitRead(8) == 0)
			{
				pmt->subtype = MEDIASUBTYPE_AAC;

				int size = len - (GetPos() - start);

				wfe = (WAVEFORMATEX*)pmt->ReallocFormatBuffer(sizeof(WAVEFORMATEX) + size);

				wfe->cbSize += size;
				wfe->wFormatTag = pmt->subtype.Data1;

				ByteRead((BYTE*)(wfe + 1), size);
			}

			break;

		case FLVAudioPacket::ADPCM:

			pmt->subtype = FOURCCMap(MAKEFOURCC('A','S','W','F'));

			break;

		case FLVAudioPacket::Nelly8:

			pmt->subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));

			wfe->nSamplesPerSec = 8000;

			break;

		case FLVAudioPacket::Nelly:

			pmt->subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));

			break;
		}

		if(pmt->subtype == GUID_NULL)
		{
			return false;
		}
	}

	return true;
}

bool CBaseSplitterFile::Read(MP4Box& h)
{
	if(GetRemaining() < 8) 
	{
		return false;
	}

	__int64 start = GetPos();

	h.size = (DWORD)BitRead(32);
	h.tag = (DWORD)BitRead(32);

	if(h.size == 0)
	{
		h.size = GetRemaining() + 8;
	}
	else if(h.size == 1)
	{
		h.size = BitRead(64);
	}

	if(h.tag == 'uuid')
	{
		h.id.Data1 = (DWORD)BitRead(32);
		h.id.Data2 = (WORD)BitRead(16);
		h.id.Data3 = (WORD)BitRead(16);
		ByteRead(h.id.Data4, 8);
	}

	h.next = start + h.size;

	return h.next <= GetLength();
}

bool CBaseSplitterFile::Read(MP4BoxEx& h)
{
	if(!Read((MP4Box&)h))
	{
		return false;
	}

	h.version = (BYTE)BitRead(8);
	h.flags = (DWORD)BitRead(24);

	return true;
}

bool CBaseSplitterFile::Read(FLACStreamHeader& h)
{
	if(GetRemaining() < 34)
	{
		return false;
	}

	h.spb_min = (DWORD)BitRead(16);
	h.spb_max = (DWORD)BitRead(16);
	h.fs_min = (DWORD)BitRead(24);
	h.fs_max = (DWORD)BitRead(24);
	h.freq = (DWORD)BitRead(20);
	h.channels = (DWORD)BitRead(3) + 1;
	h.bits = (DWORD)BitRead(5) + 1;
	h.samples = (UINT64)BitRead(36);
	ByteRead(h.md5sig, sizeof(h.md5sig));

	return true;
}

/* CRC-8, poly = x^8 + x^2 + x^1 + x^0, init = 0 */

static const BYTE s_FLAC_crc8_table[256] =
{
	0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
	0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
	0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
	0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
	0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
	0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
	0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
	0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
	0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
	0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
	0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
	0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
	0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
	0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
	0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
	0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
	0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
	0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
	0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
	0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
	0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
	0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
	0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
	0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
	0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
	0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
	0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
	0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
	0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
	0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
	0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
	0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

bool CBaseSplitterFile::Read(FLACFrameHeader& h, const FLACStreamHeader& s)
{
	__int64 start = GetPos();
	__int64 end = std::min<__int64>(start + s.fs_max, GetLength()) - 2;

	for(__int64 cur = start; cur < end; cur++)
	{
		Seek(cur);

		if(BitRead(14, true) == 0x3FFE)
		{
			h.fs_variable = (DWORD)BitRead(16) & 1;
			h.samples = (DWORD)BitRead(4);
			h.freq = (DWORD)BitRead(4);
			h.channel_config = (DWORD)BitRead(4);
			h.bits = (DWORD)BitRead(3);
			BitRead(); // reserved

			BYTE b = (BYTE)BitRead(8);
			int i = 7;
			while(i >= 0 && (b & (1 << i))) i--;
			h.pos = b & ((1 << i) - 1);
			while(++i < 7) {h.pos = (h.pos << 6) | (BitRead(8) & 0x3f);}

			switch(h.samples)
			{
			case 0: break;
			case 1: h.samples = 192; break;
			case 2: h.samples = 576; break;
			case 3: h.samples = 1152; break;
			case 4: h.samples = 2304; break;
			case 5: h.samples = 4608; break;
			case 6: h.samples = (DWORD)BitRead(8) + 1; break;
			case 7: h.samples = (DWORD)BitRead(16) + 1; break;
			default: h.samples = 256 << (h.samples - 8); break;
			}

			if(h.samples < s.spb_max || h.samples > s.spb_max)
			{
				continue;
			}

			if(!h.fs_variable)
			{
				h.pos *= s.spb_min;
			}

			switch(h.freq)
			{
			case 0: h.freq = s.freq; break;
			case 1: h.freq = 88200; break;
			case 2: h.freq = 176400; break;
			case 3: h.freq = 192000; break;
			case 4: h.freq = 8000; break;
			case 5: h.freq = 16000; break;
			case 6: h.freq = 22050; break;
			case 7: h.freq = 24000; break;
			case 8: h.freq = 32000; break;
			case 9: h.freq = 44100; break;
			case 10: h.freq = 48000; break;
			case 11: h.freq = 96000; break;
			case 12: h.freq = (DWORD)BitRead(8) * 1000; break;
			case 13: h.freq = (DWORD)BitRead(16); break;
			case 14: h.freq = (DWORD)BitRead(16) * 10; break; // ???
			case 15: break;
			}

			if(h.freq != s.freq)
			{
				continue;
			}

			int size = (int)(GetPos() - cur);

			BYTE crc = (BYTE)BitRead(8);

			Seek(cur);

			b = 0;

			while(size-- > 0)
			{
				b = s_FLAC_crc8_table[b ^ (BYTE)BitRead(8)];
			}

			if(b == crc)
			{
				h.start = cur;

				return true;
			}
		}
	}

	return false;
}
