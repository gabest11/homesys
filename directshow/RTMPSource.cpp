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
#include "RTMPSource.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include <initguid.h>
#include "moreuuids.h"

// CRTMPSourceFilter

CRTMPSourceFilter::CRTMPSourceFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CRTMPSourceFilter"), lpunk, this, __uuidof(this))
	, m_video(NULL)
	, m_audio(NULL)
	, m_current(0)
	, m_duration(0)
	, m_waitkf(true)
{
}

CRTMPSourceFilter::~CRTMPSourceFilter()
{
	for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++) 
	{
		delete *i;
	}

	DeleteOutputs();
}

void CRTMPSourceFilter::DeleteOutputs()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}

	m_outputs.clear();

	m_video = NULL;
	m_audio = NULL;
}

STDMETHODIMP CRTMPSourceFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

int CRTMPSourceFilter::GetPinCount()
{
	return m_outputs.size();
}

CBasePin* CRTMPSourceFilter::GetPin(int n)
{
	return n >= 0 && n < m_outputs.size() ? m_outputs[n] : NULL;
}

STDMETHODIMP CRTMPSourceFilter::Run(REFERENCE_TIME tStart)
{
	//CAMThread::CallWorker(CMD_START);

	return __super::Run(tStart);
}

STDMETHODIMP CRTMPSourceFilter::Pause()
{
	if(m_State == State_Running)
	{
		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverBeginFlush();
		}

		CAMThread::CallWorker(CMD_FLUSH);
	}

	if(m_State == State_Stopped)
	{
		CAMThread::Create();

		CAMThread::CallWorker(CMD_START);
	}

	return __super::Pause();
}

STDMETHODIMP CRTMPSourceFilter::Stop()
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
	}

	return __super::Stop();
}

static FILE* rtmp_log = NULL; //_wfopen(L"c:\\rtmp.log", L"w");

DWORD CRTMPSourceFilter::ThreadProc()
{
	Util::Socket::Startup();

	RTMPSocket* s = NULL;

	Util::Socket::Url url(std::string(m_url.begin(), m_url.end()).c_str());

	LARGE_INTEGER timestamp;
	__int64 offset = 0;

	timestamp.QuadPart = 0;

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

			timestamp.QuadPart = 0;
			offset = 0;

			break;

		case CMD_START:

			Reply(S_OK);

			for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++) 
			{
				delete *i;
			}

			m_h264queue.clear();

			m_waitkf = true;

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				(*i)->DeliverNewSegment(0, _I64_MAX, 1.0f);
			}

if(rtmp_log) {fprintf(rtmp_log, "open %d\n", (int)(m_start / 10000)); fflush(rtmp_log);}

			if(s == NULL)
			{
				s = new RTMPSocket();

				if(!s->Open(url, (double)m_start / 10000000, &m_playpath))
				{
					NotifyEvent(EC_ERRORABORT, 0, 0);
				}
			}
			else
			{
				if(m_duration > 0) 
				{
					s->Seek((double)m_start / 10000000);
				}
			}

			/*
			delete s;

			s = new RTMPSocket();

			if(!s->Open(url, (double)m_start / 10000000, &m_playpath))
			{
				NotifyEvent(EC_ERRORABORT, 0, 0);
			}
			*/

			while(!CheckRequest(NULL))
			{
				RTMPPacket p;

				if(!s->GetNextMediaPacket(p))
				{
					for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
					{
						CPacket* p = NULL;

						(*i)->Deliver(p);
					}

					// NotifyEvent(EC_ERRORABORT, 0, 0);

					break;
				}

				if(p.m_body == NULL)
				{
					continue;
				}

				CBaseSplitterFile f((BYTE*)p.m_body, p.m_nBodySize);

if(rtmp_log) {fprintf(rtmp_log, "packet %d %d\n", p.m_headerType, p.m_packetType); fflush(rtmp_log);}

				if(p.m_packetType == 22)
				{
					bool first = true;

					CBaseSplitterFile::FLVPacket t;

					while(f.Read(t, first))
					{
						first = false;

						__int64 pos = f.GetPos();
						__int64 next = pos + t.size;

						CRTMPSourceOutputPin* pin = NULL;

						if(t.type == CBaseSplitterFile::FLVPacket::Video)
						{
							if(m_video != NULL)
							{
								pin = m_video;
							}
						}
						else if(t.type == CBaseSplitterFile::FLVPacket::Audio)
						{
							if(m_audio != NULL)
							{
								pin = m_audio;
							}
						}

						if(pin != NULL)
						{
if(rtmp_log) fprintf(rtmp_log, "tsi: %d %d (%d)\n", t.timestamp, timestamp.HighPart, t.type);

							LARGE_INTEGER ts;

							ts.LowPart = t.timestamp;
							ts.HighPart = timestamp.HighPart;

							if((ts.LowPart & 0x80000000) == 0 && (timestamp.LowPart & 0x80000000) != 0) // rollover
							{
if(rtmp_log) fprintf(rtmp_log, "overflow\n");
								ts.HighPart++;
							}

							if(ts.QuadPart > timestamp.QuadPart + 1000) // skip ahead
							{
if(rtmp_log) fprintf(rtmp_log, "offset += %I64d\n", ts.QuadPart - timestamp.QuadPart);

								offset += ts.QuadPart - timestamp.QuadPart;
							}

							timestamp = ts;

if(rtmp_log) {fprintf(rtmp_log, "tso: %I64d (%d)\n", timestamp.QuadPart - offset, t.type); fflush(rtmp_log);}

							Deliver(pin, t.type, timestamp.QuadPart - offset, (BYTE*)&p.m_body[pos], t.size);
						}

						f.Seek(next);
					}
				}
				else if(p.m_packetType == 9 || p.m_packetType == 8)
				{
					CRTMPSourceOutputPin* pin = NULL;

					if(p.m_nInfoField1 > 100)
					{
						// absolute timestamp? ignore it...

						p.m_nInfoField1 = 0;
					}

					if(p.m_packetType == CBaseSplitterFile::FLVPacket::Video)
					{
						if(m_video != NULL)
						{
							pin = m_video;
						}

						timestamp.QuadPart += p.m_nInfoField1;
					}
					else if(p.m_packetType == CBaseSplitterFile::FLVPacket::Audio)
					{
						if(m_audio != NULL)
						{
							pin = m_audio;
						}

						timestamp.QuadPart += p.m_nInfoField1;
					}

					if(pin != NULL)
					{
if(rtmp_log) fprintf(rtmp_log, "tsi2: %I64d %d (%d)\n", timestamp.QuadPart, p.m_nInfoField1, p.m_packetType);
if(rtmp_log) {fprintf(rtmp_log, "tso2: %I64d (%d)\n", timestamp.QuadPart - offset, p.m_packetType); fflush(rtmp_log);}

						Deliver(pin, p.m_packetType, timestamp.QuadPart - offset, (BYTE*)p.m_body, p.m_nBodySize);
					}
				}
			}

			break;
		}
	}

	delete s;

	Util::Socket::Cleanup();

	return 0;
}

void CRTMPSourceFilter::Deliver(CRTMPSourceOutputPin* pin, int type, __int64 ts, const BYTE* ptr, int size)
{
	CBaseSplitterFile f(ptr, size);

	CBaseSplitterFile::FLVVideoPacket vt;
	CBaseSplitterFile::FLVAudioPacket at;

	if(type == CBaseSplitterFile::FLVPacket::Video)
	{
		if(!f.Read(vt, size) || vt.type == CBaseSplitterFile::FLVVideoPacket::Command)
		{
			return;
		}
	}
	else if(type == CBaseSplitterFile::FLVPacket::Audio)
	{
		if(!f.Read(at, size))
		{
			return;
		}
	}
	else
	{
		return;
	}

	BYTE AVCPacketType = 0; // f.BitRead(8)

	if(type == CBaseSplitterFile::FLVPacket::Video && vt.codec == CBaseSplitterFile::FLVVideoPacket::AVC
	|| type == CBaseSplitterFile::FLVPacket::Audio && at.format == CBaseSplitterFile::FLVAudioPacket::AAC)
	{
		AVCPacketType = f.BitRead(8);

		if(AVCPacketType > 1) // 0: sequence header, 1: nalu, 2: end of sequence
		{
			return;
		}
	}

	CPacket* p = new CPacket();

	p->id = type;

	p->start = ts * 10000;// - m_start; 
	p->stop = p->start + 1;

	p->flags |= CPacket::TimeValid;

	if(type == CBaseSplitterFile::FLVPacket::Video && vt.type == CBaseSplitterFile::FLVVideoPacket::Intra
	|| type == CBaseSplitterFile::FLVPacket::Audio)
	{
		p->flags |= CPacket::SyncPoint;
	}

	if(type == CBaseSplitterFile::FLVPacket::Video)
	{
		switch(vt.codec)
		{
		case CBaseSplitterFile::FLVVideoPacket::VP6: 
			f.BitRead(8); 
			break;
		case CBaseSplitterFile::FLVVideoPacket::VP6A: 
			f.BitRead(32); 
			break;
		case CBaseSplitterFile::FLVVideoPacket::AVC: 
			f.BitRead(24); 
			break;
		}
	}

	size = f.GetRemaining();

	if(size == 0)
	{
		return;
	}

	p->buff.resize(size);

	f.ByteRead(&p->buff[0], p->buff.size());

	m_current = p->start;

	if(type == CBaseSplitterFile::FLVPacket::Video && vt.codec == CBaseSplitterFile::FLVVideoPacket::AVC) 
	{
		if(AVCPacketType == 0)
		{
			// AVCDecoderConfigurationRecord

			m_h264seqhdr.clear();

			CBaseSplitterFile f(p->buff.data(), p->buff.size());

			BYTE version = (BYTE)f.BitRead(8);
			BYTE profile = (BYTE)f.BitRead(8);
			BYTE compat = (BYTE)f.BitRead(8);
			BYTE level = (BYTE)f.BitRead(8);

			f.BitRead(6); // reserved '111111'

			BYTE nalu_unit_len = (BYTE)f.BitRead(2) + 1; // TODO

			f.BitRead(3); // reserved '111'
			
			BYTE nsps = f.BitRead(5);

			while(nsps-- > 0)
			{
				DWORD size = (DWORD)f.BitRead(16);
				
				size_t i = m_h264seqhdr.size();

				m_h264seqhdr.resize(i + 4 + size);

				m_h264seqhdr[i + 0] = 0;
				m_h264seqhdr[i + 1] = 0;
				m_h264seqhdr[i + 2] = size >> 8;
				m_h264seqhdr[i + 3] = size & 0xff;
				
				f.ByteRead(&m_h264seqhdr[i + 4], size);
			}

			BYTE npps = f.BitRead(8);

			while(npps-- > 0)
			{
				DWORD size = (DWORD)f.BitRead(16);

				size_t i = m_h264seqhdr.size();

				m_h264seqhdr.resize(i + 4 + size);

				m_h264seqhdr[i + 0] = 0;
				m_h264seqhdr[i + 1] = 0;
				m_h264seqhdr[i + 2] = size >> 8;
				m_h264seqhdr[i + 3] = size & 0xff;
				
				f.ByteRead(&m_h264seqhdr[i + 4], size);
			}

			p->buff = m_h264seqhdr;

			// return;
		}

		// One or more NALUs (Full frames are required)

		{
			CBaseSplitterFile f(p->buff.data(), p->buff.size());

//if(rtmp_log) fprintf(rtmp_log, "---\nH264 %d (%d)\n", p->buff.size(), vt.type);

			while(f.GetRemaining() > 5)
			{
				DWORD size = (DWORD)f.BitRead(32);
				DWORD pos = (DWORD)f.GetPos();

				if(pos + size > f.GetLength()) break;

				BYTE forbidden_zero_bit = f.BitRead(1);
				BYTE nal_ref_idc = f.BitRead(2);
				BYTE nal_unit_type = f.BitRead(5);

//if(rtmp_log) fprintf(rtmp_log, "H264 pos %d, size = %d, nal_ref_idc = %d, nal_unit_type = %d\n", pos, size, nal_ref_idc, nal_unit_type);

				if(nal_unit_type == 1 || nal_unit_type == 5)
				{
					BYTE first_mb_in_slice = f.UExpGolombRead();
					BYTE slice_type = f.UExpGolombRead();
					BYTE pic_patameter_set_id = f.UExpGolombRead();
					BYTE frame_num = f.BitRead(2);

//if(rtmp_log) fprintf(rtmp_log, "H264 slice_type = %d\n", slice_type);
				}

				f.Seek(pos + size);
			}
		}

		/*
		if(m_waitkf)
		{
			if(vt.type != CBaseSplitterFile::FLVVideoPacket::Intra)
			{
				return;
			}

			m_waitkf = false;
		}
		*/

		bool b_frame = false;

		if(vt.type == CBaseSplitterFile::FLVVideoPacket::Inter)
		{
			CBaseSplitterFile f(p->buff.data(), p->buff.size());

			while(f.GetRemaining() > 5)
			{
				DWORD size = (DWORD)f.BitRead(32);
				DWORD pos = (DWORD)f.GetPos();

				if(pos + size > f.GetLength()) break;

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
					}

					break;
				}

				f.Seek(pos + size);
			}
		}

		if(b_frame)
		{
			if(!m_h264queue.empty())
			{
				CPacket* p2 = m_h264queue.front();

				REFERENCE_TIME atpf = p->start - p2->start;

				p2->start += atpf;
				p2->stop += atpf;

				p->start -= atpf;
				p->stop -= atpf;

				m_h264queue.push_back(p);
			}
		}
		else
		{
			HRESULT hr = S_OK;

			while(SUCCEEDED(hr) && !m_h264queue.empty())
			{
				CPacket* p2 = m_h264queue.front();

				m_h264queue.pop_front();
				
				/*
				if(!m_h264seqhdr.empty())
				{
					p2->buff.insert(p2->buff.begin(), m_h264seqhdr.begin(), m_h264seqhdr.end());

					m_h264seqhdr.clear();
				}
				*/

				hr = pin->Deliver(p2);
			}

			m_h264queue.push_back(p);
		}
	}
	else
	{
		pin->Deliver(p);
	}
}

// IFileSourceFilter

STDMETHODIMP CRTMPSourceFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		return VFW_E_NOT_STOPPED;
	}

	DeleteOutputs();

	m_current = 0;
	m_duration = 0;
	m_start = 0;

	m_playpath.clear();

	Util::Socket::Startup();

	RTMPSocket s;

	m_url = pszFileName;

	if(s.Open(Util::Socket::Url(std::string(m_url.begin(), m_url.end()).c_str()), 0, &m_playpath))
	{
		RTMPPacket p;

		for(int i = 0; i < 1000 && (m_video == NULL || m_audio == NULL) && s.GetNextMediaPacket(p); i++)
		{
			if(p.m_body == NULL)
			{
				continue;
			}

			CMediaType mt;

			HRESULT hr;

			CBaseSplitterFile f((BYTE*)p.m_body, p.m_nBodySize);

			CBaseSplitterFile::FLVVideoPacket vt;
			CBaseSplitterFile::FLVAudioPacket at;

			if(p.m_packetType == 22)
			{
				CBaseSplitterFile::FLVPacket t;
	
				while(f.Read(t))
				{
					__int64 next = f.GetPos() + t.size;

					if(t.type == CBaseSplitterFile::FLVPacket::Video)
					{
						if(m_video == NULL && f.Read(vt, t.size, &mt))
						{
							m_video = new CRTMPSourceOutputPin(this, mt, &hr);
						}
					}
					else if(t.type == CBaseSplitterFile::FLVPacket::Audio)
					{
						if(m_audio == NULL && f.Read(at, t.size, &mt))
						{
							m_audio = new CRTMPSourceOutputPin(this, mt, &hr);
						}
					}

					f.Seek(next);
				}
			}
			else if(p.m_packetType == 9)
			{
				if(m_video == NULL && f.Read(vt, p.m_nBodySize, &mt))
				{
					m_video = new CRTMPSourceOutputPin(this, mt, &hr);
				}
			}
			else if(p.m_packetType == 8)
			{
				if(m_audio == NULL && f.Read(at, p.m_nBodySize, &mt))
				{
					m_audio = new CRTMPSourceOutputPin(this, mt, &hr);
				}
			}
		}

		m_duration = (REFERENCE_TIME)(s.GetDuration() * 10000000);
	}

	s.Close();

	Util::Socket::Cleanup();

	if(m_video != NULL) m_outputs.push_back(m_video);
	if(m_audio != NULL) m_outputs.push_back(m_audio);

	if(m_video != NULL)
	{
		ASSERT(m_audio != NULL);
	}

	return m_outputs.size() > 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CRTMPSourceFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
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

STDMETHODIMP CRTMPSourceFilter::GetCapabilities(DWORD* pCapabilities)
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

STDMETHODIMP CRTMPSourceFilter::CheckCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL) return E_POINTER;

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;

	GetCapabilities(&caps);

	caps &= *pCapabilities;

	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CRTMPSourceFilter::IsFormatSupported(const GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CRTMPSourceFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CRTMPSourceFilter::GetTimeFormat(GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CRTMPSourceFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CRTMPSourceFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CRTMPSourceFilter::GetDuration(LONGLONG* pDuration)
{
	if(pDuration == NULL)
	{
		return E_POINTER;
	}
	
	*pDuration = m_duration;

	return S_OK;
}

STDMETHODIMP CRTMPSourceFilter::GetStopPosition(LONGLONG* pStop)
{
	if(pStop == NULL)
	{
		return E_POINTER;
	}

	*pStop = m_duration;

	return S_OK;
}

STDMETHODIMP CRTMPSourceFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRTMPSourceFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRTMPSourceFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if(ThreadExists())
	{
		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverBeginFlush();
		}

		CallWorker(CMD_FLUSH);

		m_start = *pCurrent;

		CallWorker(CMD_START);
	}

	return S_OK;
}

STDMETHODIMP CRTMPSourceFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent == NULL || pStop == NULL)
	{
		return E_POINTER;
	}

	*pCurrent = m_current;
	*pStop = m_duration;

	return S_OK;
}

STDMETHODIMP CRTMPSourceFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest == NULL || pLatest == NULL)
	{
		return E_POINTER;
	}

	*pEarliest = 0;
	*pLatest = m_duration;

	return S_OK;
}

STDMETHODIMP CRTMPSourceFilter::SetRate(double dRate)
{
	return dRate == 1.0 ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CRTMPSourceFilter::GetRate(double* pdRate)
{
	if(pdRate == NULL)
	{
		return E_POINTER;
	}

	*pdRate = 1.0;

	return S_OK;
}

STDMETHODIMP CRTMPSourceFilter::GetPreroll(LONGLONG* pllPreroll)
{
	if(pllPreroll == NULL)
	{
		return E_POINTER;
	}

	*pllPreroll = 0;

	return S_OK;
}

// CRTMPSourceOutputPin

CRTMPSourceFilter::CRTMPSourceOutputPin::CRTMPSourceOutputPin(CRTMPSourceFilter* pFilter, const CMediaType& mt, HRESULT* phr)
	: CBaseOutputPin(NAME("CRTMPSourceOutputPin"), pFilter, pFilter, phr, L"Output")
	, m_queue(1000)
	, m_flushing(false)
{
	m_mt = mt;
}

CRTMPSourceFilter::CRTMPSourceOutputPin::~CRTMPSourceOutputPin()
{
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::Active()
{
	CAMThread::Create();

	return __super::Active();
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::Inactive()
{
	m_exit.Set();

	CAMThread::Close();

	return __super::Inactive();
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::Deliver(CPacket* p)
{
	if(m_flushing) {delete p; return S_FALSE;}

	m_queue.Enqueue(p);

	return S_OK;
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::DeliverBeginFlush()
{
	m_flushing = true;

	return __super::DeliverBeginFlush();
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::DeliverEndFlush()
{
	m_queue.GetDequeueEvent().Wait();

	m_flushing = false;

	return __super::DeliverEndFlush();
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_mt == *pmt ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mt;

	return S_OK;
}

HRESULT CRTMPSourceFilter::CRTMPSourceOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1 << 20;

	return S_OK;
}

DWORD CRTMPSourceFilter::CRTMPSourceOutputPin::ThreadProc()
{
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
				CPacket* p = m_queue.Dequeue();

				if(!m_flushing)
				{
					if(p != NULL)
					{
						HRESULT hr;

						CComPtr<IMediaSample> sample;

						hr = GetDeliveryBuffer(&sample, NULL, NULL, 0);

						if(SUCCEEDED(hr) && sample->GetSize() >= p->buff.size())
						{
							BYTE* dst = NULL;
							
							hr = sample->GetPointer(&dst);

							if(SUCCEEDED(hr) && dst != NULL)
							{
								memcpy(dst, p->buff.data(), p->buff.size());

								sample->SetActualDataLength(p->buff.size());

								sample->SetTime(&p->start, &p->stop);
								sample->SetMediaTime(NULL, NULL);

								sample->SetDiscontinuity((p->flags & CPacket::Discontinuity) ? TRUE : FALSE);
								sample->SetSyncPoint((p->flags & CPacket::SyncPoint) ? TRUE : FALSE);
								sample->SetPreroll(FALSE);

								printf("[%d] %lld\n", p->id, p->start / 10000);

								__super::Deliver(sample);
							}
						}
					}
					else
					{
						__super::DeliverEndOfStream();
					}
				}

				delete p;
			}

			break;
		}
	}

	return 0;
}
