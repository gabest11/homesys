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
#include "MPEGSplitter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// CMPEGSplitterFilter

CMPEGSplitterFilter::CMPEGSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseSplitterFilter(NAME("CMPEGSplitterFilter"), pUnk, phr, clsid)
	, m_file(NULL)
{
}

STDMETHODIMP CMPEGSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMPEGSplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	delete m_file;

	m_file = new CMPEGSplitterFile(pAsyncReader, hr);

	if(FAILED(hr)) 
	{
		delete m_file; 

		m_file = NULL;
		
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	for(int j = 0; j < countof(m_file->m_streams); j++)
	{
		for(auto i = m_file->m_streams[j].begin(); i != m_file->m_streams[j].end(); i++)
		{
			CMPEGSplitterFile::Stream& s = *i;

			std::vector<CMediaType> mts;

			mts.push_back(s.mt);

			std::wstring name = Util::Format(L"%s %d", CMPEGSplitterFile::ToString(j), m_pOutputs.size() + 1);

			CBaseSplitterOutputPin* pin = new CMPEGSplitterOutputPin(mts, name.c_str(), this, &hr);

			if(S_OK == AddOutputPin(s, pin))
			{
				if(j == CMPEGSplitterFile::Video)
				{
					break;
				}
			}
		}
	}

	if(m_file->m_bitrate > 0)
	{
		m_rtNewStop = m_rtStop = m_rtDuration = 10000000i64 * m_file->GetLength() * 8 / m_file->m_bitrate;
	}

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool CMPEGSplitterFilter::DemuxInit()
{
	m_start = 0;

	return true;
}

void CMPEGSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	REFERENCE_TIME preroll = 10000000;
	
	if(rt <= preroll || m_rtDuration <= 0)
	{
		m_file->Seek(0);

		return;
	}

	__int64 len = m_file->GetLength();
	__int64 seekpos = (__int64)(1.0 * rt / m_rtDuration * len);
	__int64 minseekpos = _I64_MAX;

	REFERENCE_TIME rtmax = rt - preroll;
	REFERENCE_TIME rtmin = rtmax - 5000000;

	if(m_start == 0)
	{
		for(int j = 0; j < CMPEGSplitterFile::Subpic; j++)
		{
			for(auto i = m_file->m_streams[j].begin(); i != m_file->m_streams[j].end(); i++)
			{
				DWORD id = *i;

				CBaseSplitterOutputPin* pin = GetOutputPin(id);

				if(pin == NULL || !pin->IsConnected())
				{
					continue;
				}

				m_file->Seek(seekpos);

				REFERENCE_TIME pdt = _I64_MIN;

				for(int j = 0; j < 10; j++)
				{
					REFERENCE_TIME rt = m_file->NextPTS(id);

					if(rt < 0) break;

					REFERENCE_TIME dt = rt - rtmax;

					if(dt > 0 && dt == pdt) 
					{
						dt = 10000000i64;
					}

					if(rtmin <= rt && rt <= rtmax || pdt > 0 && dt < 0)
					{
						minseekpos = min(minseekpos, m_file->GetPos());

						break;
					}

					m_file->Seek(m_file->GetPos() - (__int64)(1.0 * dt / m_rtDuration * len));
	
					pdt = dt;
				}
			}
		}
	}

	if(minseekpos != _I64_MAX)
	{
		m_file->Seek(minseekpos);
	}
	else
	{
		// this file is probably screwed up, try plan B, seek by bitrate

		rt -= preroll;

		m_file->Seek((__int64)(1.0 * rt / m_rtDuration * len));

		for(int i = 0; i < CMPEGSplitterFile::Unknown; i++)
		{
			if(!m_file->m_streams[i].empty())
			{
				m_start = m_file->m_time.start + m_file->NextPTS(m_file->m_streams[i].front()) - rt;

				break;
			}
		}		
	}
}

bool CMPEGSplitterFilter::DemuxLoop()
{
	REFERENCE_TIME rtStartOffset = m_start ? m_start : m_file->m_time.start;

	HRESULT hr = S_OK;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_file->GetRemaining() > 0)
	{
		DemuxNextPacket(rtStartOffset);
	}

	return true;
}

bool CMPEGSplitterFilter::DemuxNextPacket(REFERENCE_TIME rtStartOffset)
{
	HRESULT hr;

	if(m_file->m_type == CMPEGSplitterFile::PS || m_file->m_type == CMPEGSplitterFile::ES)
	{
		BYTE b;

		if(!m_file->NextMpegStartCode(b))
		{
			return false;
		}

		if(b == 0xba) // program stream header
		{
			CMPEGSplitterFile::PSHeader h;

			if(!m_file->Read(h)) 
			{
				return false;
			}
		}
		else if(b == 0xbb) // program stream system header
		{
			CMPEGSplitterFile::PSSystemHeader h;

			if(!m_file->Read(h)) 
			{
				return false;
			}
		}
		else if(b >= 0xbd && b < 0xf0) // pes packet
		{
			CMPEGSplitterFile::PESHeader h;

			if(!m_file->Read(h, b) || !h.len) 
			{
				return false;
			}

			if(h.type == CMPEGSplitterFile::Mpeg2 && h.scrambling) 
			{
				ASSERT(0); 
				
				return false;
			}

			__int64 next = m_file->GetPos() + h.len;

			DWORD id = m_file->AddStream(b, h.len);

			if(GetOutputPin(id) != NULL)
			{
				CPacket* p = new CPacket(next - m_file->GetPos());

				p->id = id;

				if(h.fpts)
				{
					p->start = h.pts - rtStartOffset;
					p->stop = p->start + 1;

					p->flags |= CPacket::TimeValid | CPacket::SyncPoint; // syncpoint should be only set for I frames if this is video
				}
				else
				{
					p->flags |= CPacket::Appendable;
				}

				m_file->ByteRead(&p->buff[0], p->buff.size());

				hr = DeliverPacket(p);
			}

			m_file->Seek(next);
		}
	}
	else if(m_file->m_type == CMPEGSplitterFile::PVA)
	{
		CMPEGSplitterFile::PVAHeader h;

		if(!m_file->Read(h))
		{
			return false;
		}

		__int64 next = m_file->GetPos() + h.length;

		if(GetOutputPin(h.streamid))
		{
			CPacket* p = new CPacket(h.length);

			p->id = h.streamid;

			if(h.fpts)
			{
				p->start = h.pts - rtStartOffset;
				p->stop = p->start + 1;

				p->flags |= CPacket::TimeValid | CPacket::SyncPoint; // syncpoint should be only set for I frames if this is video
			}
			else
			{
				p->flags |= CPacket::Appendable;
			}

			m_file->ByteRead(&p->buff[0], p->buff.size());

			hr = DeliverPacket(p);
		}

		m_file->Seek(next);
	}

	return true;
}

// CMPEGSourceFilter

CMPEGSourceFilter::CMPEGSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CMPEGSplitterFilter(pUnk, phr, clsid)
{
	delete m_pInput;

	m_pInput = NULL;
}

// CMPEGSplitterOutputPin

CMPEGSplitterOutputPin::CMPEGSplitterOutputPin(const std::vector<CMediaType>& mts, LPCWSTR pName, CBaseSplitterFilter* pFilter, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, phr)
	, m_packet(NULL)
{
}

CMPEGSplitterOutputPin::~CMPEGSplitterOutputPin()
{
	delete m_packet;
}

HRESULT CMPEGSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	{
		CAutoLock cAutoLock(this);

		m_time.current = _I64_MIN;
		m_time.offset = 0;
	}

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CMPEGSplitterOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(this);

		delete m_packet;

		m_packet = NULL;
	}

	return __super::DeliverEndFlush();
}

HRESULT CMPEGSplitterOutputPin::DeliverPacket(CPacket* p)
{
	CAutoLock cAutoLock(this);

	if(p->flags & CPacket::TimeValid)
	{
		REFERENCE_TIME rt = p->start + m_time.offset;

		if(m_time.current != _I64_MIN && abs(rt - m_time.current) > 50000000)
		{
			m_time.offset += m_time.current - rt;
		}

		p->start += m_time.offset;
		p->stop += m_time.offset;

		m_time.current = p->start;
	}

	if(m_mt.subtype == MEDIASUBTYPE_AAC) // special code for aac, the currently available decoders only like whole frame samples
	{
		// TODO: cleanup

		if(m_packet != NULL && m_packet->buff.size() == 1 && m_packet->buff[0] == 0xff)
		{
			if(p->buff.empty() || (p->buff[0] & 0xf6) != 0xf0)
			{
				delete m_packet;

				m_packet = NULL;
			}
		}

		if(m_packet == NULL)
		{
			BYTE* s = &p->buff[0];
			BYTE* e = s + p->buff.size();

			for(; s < e; s++)
			{
				if(s[0] == 0xff && ((s[1] & 0xf6) == 0xf0 || s == e - 1))
				{
					memmove(&p->buff[0], s, e - s);
					p->buff.resize(e - s);
					m_packet = p;
					break;
				}
			}
		}
		else
		{
			m_packet->buff.insert(m_packet->buff.end(), p->buff.begin(), p->buff.end());
		}

		while(m_packet != NULL && m_packet->buff.size() > 9)
		{
			BYTE* s = &m_packet->buff[0];
			BYTE* e = s + m_packet->buff.size();

			//bool layer0 = ((s[3] >> 1) & 3) == 0;
			int len = ((s[3] & 3) << 11) | (s[4] << 3) | (s[5] >> 5);
			bool crc = (s[1] & 1) == 0;
			
			s += 7; 
			len -= 7;
			
			if(crc) 
			{
				s += 2;
				len -= 2;
			}

			if(e - s < len)
			{
				break;
			}

			if(len <= 0 || e - s >= len + 2 && (s[len] != 0xff || (s[len + 1] & 0xf6) != 0xf0))
			{
				delete m_packet;
				
				m_packet = NULL;

				break;
			}

			CPacket* p = new CPacket();
			
			p->id = m_packet->id;
			p->start = m_packet->start;
			p->stop = m_packet->stop;
			p->mt = m_packet->mt;
			p->flags = m_packet->flags; 
			p->SetData(s, len);

			if(p->flags & CPacket::TimeValid) 
			{
				p->flags |= CPacket::SyncPoint;
			}

			m_packet->flags &= ~CPacket::TimeValid;
			m_packet->flags &= ~CPacket::Discontinuity;
			m_packet->mt.InitMediaType();

			s += len;
			memmove(&m_packet->buff[0], s, e - s);
			m_packet->buff.resize(e - s);

			HRESULT hr;
			
			hr = __super::DeliverPacket(p);

			if(hr != S_OK) 
			{
				return hr;
			}
		}

		if(m_packet != NULL && p != NULL)
		{
			m_packet->flags |= p->flags & (CPacket::SyncPoint | CPacket::Discontinuity);

			if((m_packet->flags & CPacket::TimeValid) == 0)
			{
				m_packet->start = p->start;
				m_packet->stop = p->stop;

				m_packet->flags |= CPacket::TimeValid;
			}

			m_packet->mt = p->mt;
		}

		return S_OK;
	}
	else
	{
		delete m_packet;

		m_packet = NULL;
	}

	return __super::DeliverPacket(p);
}
