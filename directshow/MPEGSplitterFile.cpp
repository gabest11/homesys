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
#include "MPEGSplitterFile.h"
#include <initguid.h>
#include "moreuuids.h"

#define MEGABYTE (1 << 20)

CMPEGSplitterFile::CMPEGSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr)
	, m_type(None)
	, m_bitrate(0)

{
	m_time.start = m_time.end = 0;
	m_pos.start = m_pos.end = 0;

	if(SUCCEEDED(hr)) 
	{
		hr = Init();
	}
}

HRESULT CMPEGSplitterFile::Init()
{
	HRESULT hr;

	m_type = None;

	Seek(0);

	CBaseSplitterFile::TSHeader h;

	if(Read(h))
	{
		return E_FAIL;
	}

	Seek(0);

	if(m_type == None)
	{
		int i = 0, limit = 4;

		for(PVAHeader h; i < limit && Read(h); i++) 
		{
			Seek(GetPos() + h.length);
		}

		if(i >= limit) 
		{
			m_type = PVA;
		}
	}

	Seek(0);

	if(m_type == None)
	{
		BYTE b;

		for(int i = 0; (i < 4 || GetPos() < 65536) && m_type == None && NextMpegStartCode(b); i++)
		{
			if(b == 0xba)
			{
				PSHeader h;

				if(Read(h)) 
				{
					m_type = PS;
					m_bitrate = h.bitrate;

					break;
				}
			}
			else 
			{
				if((b & 0xe0) == 0xc0 // audio, 110xxxxx, mpeg1/2/3
				|| (b & 0xf0) == 0xe0 // video, 1110xxxx, mpeg1/2
				// || (b & 0xbd) == 0xbd) // private stream 1, 0xbd, ac3/dts/lpcm/subpic
				|| b == 0xbd) // private stream 1, 0xbd, ac3/dts/lpcm/subpic
				{
					PESHeader h;

					if(Read(h, b) && h.len > 0 && BitRead(24, true) == 0x000001)
					{
						m_type = ES;
					}
				}
			}
		}
	}

	Seek(0);

	if(m_type == None)
	{
		return E_FAIL;
	}

	// min/max pts & bitrate

	m_time.start = m_pos.start = _I64_MAX;
	m_time.end = m_pos.end = 0;

	std::list<__int64> fps;

	for(int i = 0, j = 5; i <= j; i++)
	{
		fps.push_back(i * GetLength() / j);
	}

	__int64 cur = 0;

	for(auto i = fps.begin(); i != fps.end(); i++)
	{
		__int64 pos = max(min(*i, GetLength() - MEGABYTE / 8), cur);
		__int64 next = pos + (cur == 0 ? 5 * MEGABYTE : MEGABYTE / 8);

		if(FAILED(hr = SearchStreams(pos, next)))
		{
			return hr;
		}

		cur = next;
	}

	if(m_pos.start >= m_pos.end || m_time.start >= m_time.end)
	{
		return E_FAIL;
	}

	int indicated = m_bitrate / 8;
	int detected = 10000000i64 * (m_pos.end - m_pos.start) / (m_time.end - m_time.start);

	// normally "detected" should always be less than "indicated", but sometimes it can be a few percent higher (+10% is allowed here)
	// (update: also allowing +/-50k/s)

	if(indicated == 0 || (float)detected / indicated < 1.1f || abs(detected - indicated) < 50 * 1024)
	{
		m_bitrate = detected;
	}
	else 
	{
		// TODO: in this case disable seeking, or try doing something less drastical...
	}

	Seek(0);

	return S_OK;
}

REFERENCE_TIME CMPEGSplitterFile::NextPTS(DWORD id)
{
	REFERENCE_TIME rt = -1;
	__int64 pos = -1;

	BYTE b;

	while(GetRemaining() > 0)
	{
		if(m_type == PS || m_type == ES)
		{
			if(!NextMpegStartCode(b)) // continue;
			{
				ASSERT(0); 

				break;
			}

			pos = GetPos() - 4;

			if(b >= 0xbd && b < 0xf0)
			{
				PESHeader h;

				if(!Read(h, b) || h.len == 0)
				{
					continue;
				}

				__int64 next = GetPos() + h.len;

				if(h.fpts && AddStream(b, h.len) == id)
				{
					ASSERT(h.pts >= m_time.start && h.pts <= m_time.end);

					rt = h.pts;

					break;
				}

				Seek(next);
			}
		}
		else if(m_type == PVA)
		{
			PVAHeader h;

			if(Read(h) && h.fpts)
			{
				rt = h.pts;

				break;
			}
		}
	}

	if(pos >= 0) 
	{
		Seek(pos);
	}

	if(rt >= 0) 
	{
		rt -= m_time.start;
	}

	return rt;
}

HRESULT CMPEGSplitterFile::SearchStreams(__int64 start, __int64 stop)
{
	Seek(start);

	stop = min(stop, GetLength());

	while(GetPos() < stop)
	{
		BYTE b;

		if(m_type == PS || m_type == ES)
		{
			if(NextMpegStartCode(b))
			{
				if(b == 0xba) // program stream header
				{
					PSHeader h;

					if(Read(h))
					{
						// TODO: should not we use scr instead of pts for initalizing m_time/m_pos?

						int i = 0;
					}
				}
				else if(b == 0xbb) // program stream system header
				{
					PSSystemHeader h;

					if(Read(h))
					{
					}
				}
				else if(b >= 0xbd && b < 0xf0) // pes packet
				{
					PESHeader h;

					if(Read(h, b))
					{
						if(h.type == Mpeg2 && h.scrambling) 
						{
							ASSERT(0); 
							
							return E_FAIL;
						}

						if(h.fpts)
						{
							if(m_time.start == _I64_MAX) 
							{
								m_time.start = h.pts; 
								m_pos.start = GetPos();
							}

							if(m_time.start < h.pts && m_time.end < h.pts) 
							{
								m_time.end = h.pts; 
								m_pos.end = GetPos();
							}

							/*
							int bitrate = 10000000i64 * (m_pos.end - m_pos.start) / (m_time.end - m_time.start); 
							if(m_bitrate == 0) m_bitrate = bitrate;
							TRACE(_T("bitrate = %d (%d), (h.pts = %I64d)\n"), bitrate, bitrate - m_bitrate, h.pts);
							m_bitrate = rate;
							*/
						}

						__int64 next = GetPos() + h.len;
						
						AddStream(b, h.len);
						
						Seek(next);
					}
				}
			}
		}
		else if(m_type == PVA)
		{
			PVAHeader h;

			if(Read(h))
			{
				__int64 pos = GetPos();

				if(h.fpts)
				{
					if(m_time.start == _I64_MAX) 
					{
						m_time.start = h.pts; 
						m_pos.start = pos;
					}

					if(m_time.start < h.pts && m_time.end < h.pts) 
					{
						m_time.end = h.pts; 
						m_pos.end = pos;
					}
				}

				if(h.streamid == 1) 
				{
					AddStream(0xe0, h.length);
				}
				else if(h.streamid == 2) 
				{
					AddStream(0xc0, h.length);
				}

				Seek(pos + h.length);
			}
		}
	}

	return S_OK;
}

DWORD CMPEGSplitterFile::AddStream(BYTE pesid, DWORD len)
{
	int type;

	return AddStream(pesid, len, type);
}

DWORD CMPEGSplitterFile::AddStream(BYTE pesid, DWORD len, int& type)
{
	__int64 pos = GetPos();

	Stream s;

	s.id.pes = pesid;

	type = Unknown;

	if(pesid >= 0xe0 && pesid < 0xf0) // mpeg video
	{
		if(type == Unknown)
		{
			Seek(pos);

			CMPEGSplitterFile::MpegSequenceHeader h;

			if(Read(h, len, &s.mt))
			{
				type = Video;
			}
		}

		if(type == Unknown)
		{
			Seek(pos);

			CMPEGSplitterFile::H264Header h;

			if(Read(h, len, &s.mt))
			{
				type = Video;
			}
		}
	}
	else if(pesid >= 0xc0 && pesid < 0xe0) // mpeg audio
	{
		if(type == Unknown)
		{
			Seek(pos);

			CMPEGSplitterFile::MpegAudioHeader h;

			if(Read(h, len, false, &s.mt))
			{
				type = Audio;
			}
		}

		if(type == Unknown)
		{
			Seek(pos);

			CMPEGSplitterFile::LATMHeader h;

			if(Read(h, len, &s.mt))
			{
				type = Audio;
			}
		}

		if(type == Unknown)
		{
			Seek(pos);

			CMPEGSplitterFile::AACHeader h;

			if(Read(h, len, &s.mt))
			{
				type = Audio;
			}
		}
	}
	else if(pesid == 0xbd || pesid == 0xfd) // private stream 1
	{
			BYTE b = (BYTE)BitRead(8, true);
			WORD w = (WORD)BitRead(16, true);
			DWORD dw = (DWORD)BitRead(32, true);

			if(b >= 0x80 && b < 0x88 || w == 0x0b77) // ac3
			{
				s.id.ps1 = (b >= 0x80 && b < 0x88) ? (BYTE)(BitRead(32) >> 24) : 0x80;

				pos = GetPos();
		
				CMPEGSplitterFile::AC3Header h;

				if(Read(h, len, &s.mt))
				{
					type = Audio;
				}
			}
			else if(b >= 0x88 && b < 0x90 || dw == 0x7ffe8001) // dts
			{
				s.id.ps1 = (b >= 0x88 && b < 0x90) ? (BYTE)(BitRead(32) >> 24) : 0x88;

				pos = GetPos();

				CMPEGSplitterFile::DTSHeader h;

				if(Read(h, len, &s.mt))
				{
					type = Audio;
				}
			}
			else if(b >= 0xa0 && b < 0xa8) // lpcm
			{
				s.id.ps1 = (b >= 0xa0 && b < 0xa8) ? (BYTE)(BitRead(32) >> 24) : 0xa0;

				pos = GetPos();
				
				CMPEGSplitterFile::LPCMHeader h;

				if(Read(h, &s.mt)) 
				{
					type = Audio;

					pos = GetPos(); // skip lpcm header
				}
			}
			else if(b >= 0x20 && b < 0x40) // DVD subpic
			{
				s.id.ps1 = (BYTE)BitRead(8);

				pos = GetPos();

				CMPEGSplitterFile::DVDSubpictureHeader h;

				if(Read(h, &s.mt))
				{
					type = Subpic;
				}
			}
			else if(b >= 0x70 && b < 0x80) // SVCD subpic
			{
				s.id.ps1 = (BYTE)BitRead(8);

				pos = GetPos();

				CMPEGSplitterFile::SVCDSubpictureHeader h;

				if(Read(h, &s.mt))
				{
					type = Subpic;
				}
			}
			else if(b >= 0x00 && b < 0x10) // CVD subpic
			{
				s.id.ps1 = (BYTE)BitRead(8);

				pos = GetPos();

				CMPEGSplitterFile::CVDSubpictureHeader h;

				if(Read(h, &s.mt))
				{
					type = Subpic;
				}
			}
			else if(w == 0xffa0 || w == 0xffa1) // ps2-mpg audio
			{
				s.id.ps1 = (BYTE)BitRead(8);
				s.id.extra = (WORD)((BitRead(8) << 8) | BitRead(16)); // 0xa?00 | track id

				pos = GetPos();

				CMPEGSplitterFile::PS2AudioHeader h;

				if(Read(h, &s.mt))
				{
					type = Audio;
				}
			}
			else if(w == 0xff90) // ps2-mpg ac3 or subtitles
			{
				s.id.ps1 = (BYTE)BitRead(8);
				s.id.extra = (WORD)((BitRead(8) << 8) | BitRead(16)); // 0x9000 | track id

				w = (WORD)BitRead(16, true);

				pos = GetPos();

				if(w == 0x0b77)
				{
					CMPEGSplitterFile::AC3Header h;

					if(Read(h, len, &s.mt))
					{
						type = Audio;
					}
				}
				else if(w == 0x0000) // usually zero...
				{
					CMPEGSplitterFile::PS2SubpictureHeader h;

					if(Read(h, &s.mt))
					{
						type = Subpic;
					}
				}
			}
	}
	else if(pesid == 0xbe) // padding
	{
	}
	else if(pesid == 0xbf) // private stream 2
	{
	}

	if(type != Unknown)
	{
		std::list<Stream>& streams = m_streams[type];

		if(std::find(streams.begin(), streams.end(), s) == streams.end())
		{
			streams.push_back(s);
		}
	}
	else
	{
		for(int t = 0; t < Unknown; t++)
		{
			std::list<Stream>& streams = m_streams[t];

			auto i = std::find(streams.begin(), streams.end(), s);

			if(i != streams.end())
			{
				type = t;

				break;
			}
		}
	}

	Seek(pos);

	return s;
}

