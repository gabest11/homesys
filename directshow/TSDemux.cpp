#include "stdafx.h"
#include "TSDemux.h"
#include "DirectShow.h"
#include "AsyncReader.h"
#include "MediaTypeEx.h"
#include "FireDTV.h"
#include <initguid.h>
#include "moreuuids.h"

static unsigned __int64 s_first = 0, s_total = 0, s_start;

static void cpu_begin()
{
	s_start = __rdtsc();

	if(s_first == 0) 
	{
		s_first = s_start; 
		s_total = 0;
	}
}

static void cpu_end()
{
	unsigned __int64 stop = __rdtsc();

	s_total += stop - s_start;

	if(stop - s_first > 3000000000ui64) 
	{
		printf("%I64d%% %d\n", s_total * 100 / (stop - s_first), GetCurrentThreadId()); 

		s_first = stop; 
		s_total = 0;
	}
}

CTSDemuxFilter::CTSDemuxFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseFilter(NAME("CTSDemuxFilter"), lpunk, this, __uuidof(this))
	, m_freq(0)
	, m_sid(0)
	, m_tmpsid(0)
	, m_pat(true)
	, m_sdt(true)
	, m_sdt_count(0)
	, m_record(WriteProgram)
	, m_streaming(false)
	, m_size(0)
	, m_received(0)
	, m_received_error(0)
	, m_firesat_pmt_sent(false)
	, m_ci_pmt_sent(0)
	, m_scs_cookie(0)
	, m_seekbybitrate(false)
{
	if(phr) *phr = S_OK;

	m_input = new CTSDemuxInputPin(this, phr);
	m_writer = new CTSDemuxWriterOutputPin(this, phr);

	m_time.start = -1;
	m_time.pcr = -1;
	m_time.pts = -1;
	m_time.first = 0;
	m_time.last = 0;
	m_time.newstart = 0;
	
	m_dummy.video.pid = 0;
	m_dummy.audio.pid = 0;
}

CTSDemuxFilter::~CTSDemuxFilter()
{
	SetSmartCardServer(NULL);
	
	ResetPrograms();

	delete m_input;

	m_input = NULL;

	delete m_writer;

	m_writer = NULL;

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}

	for(auto i = m_cache.begin(); i != m_cache.end(); i++)
	{
		delete i->second;
	}
}

STDMETHODIMP CTSDemuxFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		m_reader != NULL && QI(IMediaSeeking)
		QI(ITSDemuxFilter)
		QI(ISmartCardClient)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

int CTSDemuxFilter::GetPinCount() 
{
	return 2 + m_outputs.size();
}

CBasePin* CTSDemuxFilter::GetPin(int n) 
{
	if(n == 0) 
	{
		return m_input; 
	}

	n--;

	if(n >= 0 && n < m_outputs.size()) 
	{
		return m_outputs[n]; 
	}

	n -= m_outputs.size();

	if(n == 0)
	{
		return m_writer;
	}

	n--;

	return NULL;
}

STDMETHODIMP CTSDemuxFilter::Pause()
{
	CAutoLock cAutoLock(this);

	if(m_State == State_Stopped)
	{
		m_received = 0;
		m_received_error = 0;

		m_streaming = true;

		if(m_reader != NULL)
		{
			CAMThread::Create();
		}
	}

	return __super::Pause();
}

STDMETHODIMP CTSDemuxFilter::Stop()
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		m_streaming = false;

		DeliverBeginFlush();

		CallWorker(CMD_EXIT);

		// CAMThread::Close();

		CAutoLock cAutoLock(&m_csReceive);

		DeliverEndFlush();

		ResetBuffer();
	}

	return __super::Stop();
}

HRESULT CTSDemuxFilter::CheckInput(const CMediaType* pmt)
{
	if(pmt->majortype == MEDIATYPE_Stream)
	{
		if(pmt->subtype == KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT || pmt->subtype == MEDIASUBTYPE_MPEG2_TRANSPORT)
		{
			return S_OK;
		}
		else if(pmt->subtype == MEDIASUBTYPE_NULL && m_reader != NULL)
		{
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTSDemuxFilter::CheckInput(IPin* pPin)
{
	CComQIPtr<IAsyncReader> reader = pPin;

	if(reader == NULL)
	{
		return S_OK;
	}

	ResetPrograms();

	int pcr = 0;

	__int64 total = 0;
	__int64 available = 0;

	reader->Length(&total, &available);

	std::vector<BYTE> buff(65536);

	for(__int64 start = 0; start < available; start += buff.size())
	{
		int len = (int)std::min<__int64>(available - start, buff.size());

		if(FAILED(reader->SyncRead(start, len, buff.data())))
		{
			break;
		}

		if(start == 0) // check for transport stream
		{
			CBaseSplitterFile f(buff.data(), len);

			CBaseSplitterFile::TSHeader h;

			if(!f.Read(h))
			{
				break;
			}
		}

		OnReceive(buff.data(), len);

		std::list<TSDemuxProgram*> programs;

		if(SUCCEEDED(GetPrograms(programs, m_sid, false, false)))
		{
			if(m_sid) break;

			for(auto i = programs.begin(); i != programs.end(); i++)
			{
				const TSDemuxProgram* p = *i;

				if(p->streams.size() > p->GetScrambledCount())
				{
					m_sid = p->sid;
					pcr = p->pcr;

					break;
				}
			}

			if(m_sid) break;

			int n = 0;

			for(auto i = programs.begin(); i != programs.end(); i++)
			{
				const TSDemuxProgram* p = *i;

				if(!p->streams.empty() && p->streams.size() <= p->GetScrambledCount())
				{
					n++;
				}
			}

			if(n > 0 && n == programs.size())
			{
				printf("Program encrypted (sid = %d)\n", programs.front()->sid);

				return E_ACCESSDENIED;
			}
		}
	}

	ResetBuffer();

	if(pcr == 0) 
	{
		return E_FAIL;
	}

	m_time.first = 0;
	m_time.last = 0;

	for(__int64 start = 0; start < available; start += buff.size())
	{
		int len = (int)std::min<__int64>(available - start, buff.size());

		if(FAILED(reader->SyncRead(start, len, buff.data())))
		{
			break;
		}

		OnReceive(buff.data(), len);

		if(m_time.pts >= 0)
		{
			m_time.first = m_time.pts;

			break;
		}
	}

	ResetBuffer();

	for(__int64 start = std::max<__int64>(0, available - buff.size()); start >= 0; start -= buff.size())
	{
		int len = (int)std::min<__int64>(available - start, buff.size());

		if(FAILED(reader->SyncRead(start, len, buff.data())))
		{
			break;
		}

		OnReceive(buff.data(), len);

		if(m_time.pcr >= 0)
		{
			m_time.last = m_time.pcr;

			break;
		}
	}

	ResetBuffer();

	m_seekbybitrate = false;

	if(1)//m_time.last <= m_time.first)
	{
		//printf("TSDemux: duration may be wrong!\n");

		m_seekbybitrate = true;

		__int64 start = 0;
		__int64 end = std::min<__int64>(available, 4 << 20);

		__int64 first = 0;
		__int64 last = 0;

		while(start < end)
		{
			int len = (int)std::min<__int64>(available - start, buff.size());

			if(FAILED(reader->SyncRead(start, len, buff.data())))
			{
				break;
			}

			OnReceive(buff.data(), len);

			start += buff.size();

			if(m_time.pcr > 0)
			{
				if(first == 0)
				{
					first = m_time.pcr;
				}
				else
				{
					if(m_time.pcr > first)
					{
						last = m_time.pcr;
					}
					else
					{
						break;
					}
				}
			}
		}

		if(start > 0)
		{
			__int64 dt = last - first;

			if(dt > 0)
			{
				m_time.last = m_time.first + dt * available / start;
			}
		}

		ResetBuffer();
	}

	m_time.newstart = m_time.first;

	m_reader = reader;

	return S_OK;
}

HRESULT CTSDemuxFilter::CompleteInput(IPin* pPin)
{
	IBaseFilter* pBF = DirectShow::GetFilter(pPin);

	while(pBF != NULL)
	{
		if(CComQIPtr<IKsPropertySet> ksps = pBF)
		{
			if(SUCCEEDED(ksps->Set(__uuidof(Firesat), FIRESAT_TEST_INTERFACE, NULL, NULL, NULL, NULL)))
			{
				m_ksps = ksps;

				break;
			}
		}

		pBF = DirectShow::GetUpStreamFilter(pBF);
	}

	if(m_reader)
	{
		SetProgram(m_sid, false);
	}

	return S_OK;
}

HRESULT CTSDemuxFilter::BreakInput()
{
	m_sid = 0;

	m_reader = NULL;
	m_ksps = NULL;
	m_ci = NULL; // ?

	ResetPrograms();

	m_cached_streams.clear();

	return S_OK;
}

HRESULT CTSDemuxFilter::Receive(const BYTE* p, int len)
{
	CAutoLock cAutoLock(&m_csReceive);
	
	if(m_streaming)
	{
		OnReceive(p, len);
	}

	return S_OK;
}

bool CTSDemuxFilter::WritePID(int pid)
{
	if(m_record == WriteNone)
	{
		return false;
	}

	if(m_record == WriteFullTS || pid < 0x20 || pid == 0x1ffb)
	{
		return true;
	}

	CAutoLock cAutoLock(&m_csPrograms);

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		TSDemuxProgram* p = i->second;

		if(p->sid == m_sid)
		{
			if(p->pid == pid || p->pcr == pid)
			{
				return true;
			}

			if(p->streams.find(pid) != p->streams.end())
			{
				return true;
			}

			if(m_emms.find(pid) != m_emms.end())
			{
				return true;
			}

			if(p->GetSystemId(pid) != 0)
			{
				return true;
			}

			break;
		}
	}

	return false;
}

DWORD CTSDemuxFilter::ThreadProc()
{
	HRESULT hr;

	__int64 total = 0;
	__int64 available = 0;

	hr = m_reader->Length(&total, &available);

	std::vector<BYTE> buff(65536);

	m_endflush.Set();

	for(DWORD cmd = -1; ; cmd = GetRequest())
	{
		if(cmd == CMD_EXIT)
		{
			m_hThread = NULL;

			Reply(S_OK);

			break;
		}

		__int64 pos = Seek();

		if(cmd != -1)
		{
			Reply(S_OK);
		}

		m_endflush.Wait();

		DeliverNewSegment(m_time.start - m_time.first);

		while(pos < available && !CheckRequest(NULL))
		{
			int len = (int)std::min<__int64>(available, buff.size());

			hr = m_reader->SyncRead(pos, len, buff.data());

			pos += len;
			total -= len;

			Receive(buff.data(), len);
		}
	
		if(!CheckRequest(NULL))
		{
			DeliverEndOfStream();
		}
	}

	m_hThread = 0;

	return 0;
}

__int64 CTSDemuxFilter::Seek()
{
	m_time.start = m_time.newstart;
	m_time.pcr = m_time.newstart;
	m_time.pts = m_time.newstart;
	m_time.offset = 0;

	int pcr = 0;

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		TSDemuxProgram* p = i->second;

		if(p->sid == m_sid)
		{
			pcr = p->pcr;

			break;
		}
	}

	__int64 ret = 0; // TODO

	HRESULT hr = S_OK;

	CBaseSplitterFile f(m_reader, hr);

	if(SUCCEEDED(hr) && pcr > 0 && m_time.last > m_time.first)
	{
		CBaseSplitterFile::TSHeader h;

		__int64 minpos = 0, maxpos = f.GetLength();

		__int64 t = m_time.start - 20000000; // TODO: find keyframe

		if(m_seekbybitrate)
		{
			double d = (double)(t - m_time.first) / (m_time.last - m_time.first) * maxpos;

			f.Seek((__int64)d);

			while(f.GetPos() < maxpos)
			{
				if(!f.Read(h))
				{
					continue;
				}

				if(h.pid == pcr && h.fpcr)
				{
					m_time.newstart = h.pcr;
					m_time.start = h.pcr;
					m_time.pcr = h.pcr;
					m_time.pts = h.pcr;

					minpos = h.start;

					break;
				}
			}
		}
		else
		{
			for(int i = 0; i < 10 && (maxpos - minpos) >= 1024 * 1024; i++)
			{
				f.Seek((minpos + maxpos) / 2);

				while(f.GetPos() < maxpos)
				{
					if(!f.Read(h))
					{
						continue;
					}

					if(h.pid == pcr && h.fpcr)
					{
						if(h.pcr > t)
						{
							maxpos = std::max<__int64>(h.start - 65536, minpos);
						}
						else
						{
							minpos = h.start;
						}

						break;
					}

					f.Seek(h.next);
				}
			}

			f.Seek(minpos);

			while(f.GetRemaining())
			{
				if(!f.Read(h))
				{
					continue;
				}

				if(h.pid == pcr && h.fpcr)
				{
					if(h.pcr > t)
					{
						minpos = h.start;

						break;
					}
				}

				f.Seek(h.next);
			}
		}

		ret = minpos;
	}

	return ret;
}

void CTSDemuxFilter::DeliverNewSegment(REFERENCE_TIME pts)
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++) 
	{
		(*i)->DeliverNewSegment(pts);
	};
}

void CTSDemuxFilter::DeliverEndOfStream()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++) 
	{
		(*i)->DeliverEndOfStream();
	};
}

void CTSDemuxFilter::DeliverBeginFlush()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++) 
	{
		(*i)->DeliverBeginFlush();
	};
}

void CTSDemuxFilter::DeliverEndFlush()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++) 
	{
		(*i)->DeliverEndFlush();
	};

	m_size = 0;

	for(auto i = m_payload.begin(); i != m_payload.end(); i++)
	{
		delete i->second;
	}

	m_payload.clear();

	m_endflush.Set();
}

void CTSDemuxFilter::ResetBuffer()
{
	m_time.start = -1;
	m_time.pcr = -1;
	m_time.pts = -1;

	for(auto i = m_payload.begin(); i != m_payload.end(); i++)
	{
		delete i->second;
	}

	m_payload.clear();

	m_buff.clear();
	m_size = 0;
}

void CTSDemuxFilter::OnReceive(const BYTE* buff, int len)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_received += len;
	m_received_error = 0;

	int size = m_size + len;
	
	if(m_size > 0)
	{
		if(size > m_buff.size())
		{
			m_buff.resize(size);
		}

		memcpy(&m_buff[m_size], buff, len);

		buff = m_buff.data();
	}

	CBaseSplitterFile f(buff, size);

	int tslen = 0;

	if(!f.NextTSPacket(tslen))
	{
		m_size = 0;

		return;
	}

	int packets_total = 0;
	int packets_correct = 0;

	CBaseSplitterFile::TSHeader h;

	for(int pos = 0, limit = size - tslen; pos <= limit; f.Seek(pos))
	{
		packets_total++;

		pos += tslen;

		if(!f.Read(h) || h.error)
		{
			continue;
		}

		packets_correct++;

		pos = (int)h.next;

		if(h.pid == 0x1fff)
		{
			continue;
		}

		if(h.fpcr)
		{
			OnPCR(h.pid, h.pcr);
		}

		TSDemuxPayload* payload = NULL;

		auto pair = m_payload.find(h.pid);

		if(pair != m_payload.end())
		{
			payload = pair->second;
		}
		else
		{
			payload = new TSDemuxPayload(h.pid);

			m_payload[h.pid] = payload;
		}

		OnPayload(&buff[h.start], payload, h, h.scrambling || payload->m_scrambled_size > 0);
	}

	m_size = f.GetRemaining();

	if(m_size > 0)
	{
		if(m_size > m_buff.size())
		{
			m_buff.resize(m_size);
		}

		memcpy(&m_buff[0], &buff[(int)f.GetPos()], m_size);
	}

	if(packets_total > 0)
	{
		m_received_error = (float)(packets_total - packets_correct) / packets_total;
	}

	//printf("m_received = %I64d, m_received_error = %f\n", m_received, m_received_error);
}

void CTSDemuxFilter::OnPayload(const BYTE* buff, TSDemuxPayload* payload, const CBaseSplitterFile::TSHeader& h, bool decrypt)
{
	if(decrypt)
	{
		CSA* csa = NULL;

		bool flush = false;

		{
			CAutoLock cAutoLock(&m_csPrograms);

			for(auto i = m_programs.begin(); i != m_programs.end() && csa == NULL; i++)
			{
				TSDemuxProgram* p = i->second;

				if(m_sid == 0 || m_sid == p->sid || m_tmpsid == p->sid)
				{
					auto j = p->streams.find(payload->m_pid);

					if(j != p->streams.end())
					{
						TSDemuxStream* s = j->second;

						if(s->gotkey)
						{
							csa = &s->csa;

							break;
						}
						else if(p->gotkey)
						{
							csa = &p->csa;

							break;
						}
					}
				}
			}

			if(csa != NULL)
			{
				const int n = 188 * 32; // TODO: 188 * csa->get_internal_parallelism()

				if(payload->m_scrambled.empty())
				{
					payload->m_scrambled.resize(n);
				}

				memcpy(&payload->m_scrambled[payload->m_scrambled_size], buff, 188);

				payload->m_scrambled_size += 188;

				if(payload->m_scrambled_size == n)
				{
					csa->Decrypt(payload->m_scrambled.data(), payload->m_scrambled_size / 188);

					payload->m_scrambled_size = 0;

					flush = true;
				}
			}
		}

		if(flush)
		{
			CBaseSplitterFile f(payload->m_scrambled.data(), payload->m_scrambled.size());

			while(f.GetRemaining() >= 188)
			{
				CBaseSplitterFile::TSHeader h;

				if(f.Read(h, false))
				{
					OnPayload(&payload->m_scrambled[h.start], payload, h, false);
				}

				f.Seek(h.next);
			}
		}

		if(csa != NULL)
		{
			return;
		}
	}

	payload->m_scrambled_size = 0;

	if(m_writer->IsConnected() && WritePID(h.pid))
	{
		CComPtr<IMediaSample> sample;

		if(SUCCEEDED(m_writer->GetDeliveryBuffer(&sample, NULL, NULL, 0)))
		{
			BYTE* dst = NULL;
						
			sample->GetPointer(&dst);
			memcpy(dst, buff, 188);
			sample->SetActualDataLength(188);

			m_writer->Deliver(sample);
		}
	}

	if(payload->m_size > 0)
	{
		CBaseSplitterFile f(&payload->m_buff[0], payload->m_size);

		m_packet.payload = payload->m_packet;

		auto i = m_programs.find(h.pid);

		if(i != m_programs.end())
		{
			OnPMT(f, i->second);
		}
		else
		{
			auto i = m_streams.find(h.pid);

			if(i != m_streams.end())
			{
				TSDemuxStream* s = i->second;

				if(h.payloadstart)
				{
					if(s->payloadstart < INT_MAX)
					{
						s->payloadstart++;
					}
				}

				if(h.scrambling)
				{
					s->scrambled = true;

					return;
				}

				OnPES(f, s, h.payloadstart, h.pid);

				if(!h.payloadstart)
				{
					if(f.GetPos() > 0 && f.GetPos() < f.GetLength())
					{
						payload->m_size = f.GetRemaining();

						f.ByteRead(&payload->m_buff[0], payload->m_size);
					}
				}
			}
			else
			{
				switch(h.pid)
				{
				case 0:
					OnPAT(f);
					break;
				case 1:
					OnCAT(f);
					break;
				case 0x10:
					OnNIT(f);
					break;
				case 0x11:
					OnSDT(f);
					break;
				case 0x12:
					OnEIT(f);
					break;
				case 0x1ffb:
					OnVCT(f);
					break;

				default:

					OnEMM(f, h.pid);
					OnECM(f, h.pid);

					break;
				}
			}
		}
	}

	if(h.payloadstart)
	{
		payload->m_size = 0;
		payload->m_packet = m_packet.total;
	}

	payload->Append(&buff[188 - h.bytes], h.bytes);

	m_packet.total++;
}

void CTSDemuxFilter::OnPAT(CBaseSplitterFile& f)
{
	__int64 end;

	if(CheckSection(f, end) != 0)
	{
		return;
	}

	CAutoLock cAutoLock(&m_csPrograms);

	std::set<int> missing;

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		missing.insert(i->first);
	}

	while(f.GetPos() < end)
	{
		WORD sid = (WORD)f.BitRead(16);
		BYTE res = (BYTE)f.BitRead(3);
		WORD pid = (WORD)f.BitRead(13);

		if(sid == 0) continue;

		auto i = m_programs.find(pid);

		if(i != m_programs.end())
		{
			missing.erase(pid);

			if(i->second->sid != sid)
			{
				delete i->second;

				m_programs.erase(i);

				i = m_programs.end();
			}
		}

		if(i == m_programs.end())
		{
			TSDemuxProgram* p = new TSDemuxProgram();

			p->sid = sid;
			p->pid = pid;
			p->name = Util::Format(L"SID=%d PMT=%d", sid, pid);

			m_programs[pid] = p;
		}
	}

	for(auto i = missing.begin(); i != missing.end(); i++)
	{
		auto j = m_programs.find(*i);

		delete j->second;

		m_programs.erase(j);
	}

	if(!m_pat.Check())
	{
		m_pat.Set();

		m_packet.pat = m_packet.payload;

		wprintf(L"PAT");

		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			wprintf(L" %d", i->second->pid);
		}

		wprintf(L" @ %I64d\n", m_packet.total);
	}
	else
	{
		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			TSDemuxProgram* p = i->second;
		
			if(p->packet == 0 && p->missing < 1)
			{
				if(++p->missing == 1)
				{
					wprintf(L"PMT [%d] missing!\n", p->pid);
				}
			}
		}
	}
}

void CTSDemuxFilter::OnCAT(CBaseSplitterFile& f)
{
	__int64 end;

	if(CheckSection(f, end) != 1)
	{
		return;
	}

	if(m_scs != NULL) // TODO: lock
	{
		int pos = f.GetPos();
		int len = (int)f.GetLength() - 1;

		std::vector<BYTE>* buff = new std::vector<BYTE>(len);

		f.Seek(1);
		f.ByteRead(buff->data(), buff->size());
		f.Seek(pos);

		OnSmartCardPacket(ISmartCardServer::CAT, buff, 1, 0, true);
	}

	CAutoLock cAutoLock(&m_csPrograms);

	m_emms.clear();

	while(f.GetPos() < end)
	{
		int tag = (int)f.BitRead(8);
		int len = (int)f.BitRead(8);

		__int64 next = std::min<__int64>(f.GetPos() + len, f.GetLength());

		if(tag == 9)
		{
			WORD CA_system_id = (WORD)f.BitRead(16);
			f.BitRead(3); // reserved
			int pid = (int)f.BitRead(13); // emm pid

			m_emms[pid] = CA_system_id;

			// TODO: 4 byte crc?
		}

		f.Seek(next);
	}
}

void CTSDemuxFilter::OnEMM(CBaseSplitterFile& f, int pid)
{
	std::vector<BYTE>* buff = NULL;
	WORD system_id = 0;
	
	if(m_scs != NULL) // TODO: lock
	{
		CAutoLock cAutoLock(&m_csPrograms);

		auto i = m_emms.find(pid);

		if(i != m_emms.end())
		{
			int pos = f.GetPos();
			int len = (int)f.GetLength() - (pos + 1);

			buff = new std::vector<BYTE>(len);
			system_id = i->second;

			f.Seek(1);
			f.ByteRead(buff->data(), buff->size());
			f.Seek(pos);
		}
	}

	OnSmartCardPacket(ISmartCardServer::EMM, buff, pid, system_id, true);
}

void CTSDemuxFilter::OnECM(CBaseSplitterFile& f, int pid)
{
	std::vector<BYTE>* buff = NULL;
	WORD system_id = 0;
	bool required = m_sid == 0;
	
	if(m_scs != NULL) // TODO: lock
	{
		CAutoLock cAutoLock(&m_csPrograms);

		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			const TSDemuxProgram* p = i->second;

			WORD id = p->GetSystemId(pid);

			if(id != 0)
			{
				if(system_id == 0)
				{
					system_id = id;
				}

				if(m_sid == p->sid || m_tmpsid == p->sid)
				{
					required = true;

					break;
				}
			}
		}

		if(system_id != 0)
		{
			int pos = f.GetPos();
			int len = (int)f.GetLength() - (pos + 1);
					
			buff = new std::vector<BYTE>(len);

			f.Seek(1);
			f.ByteRead(buff->data(), buff->size());
			f.Seek(pos);
		}
	}

	OnSmartCardPacket(ISmartCardServer::ECM, buff, pid, system_id, required);
}

void CTSDemuxFilter::OnPMT(CBaseSplitterFile& f, TSDemuxProgram* p)
{
	__int64 start = f.GetPos();
	__int64 end;

	if(CheckSection(f, end) != 2)
	{
		return;
	}

	CAutoLock cAutoLock(&m_csPrograms);

	// firesat

	if(!m_firesat_pmt_sent && m_ksps != NULL && (m_tmpsid == 0 && p->sid == m_sid || m_tmpsid != 0 && p->sid == m_tmpsid))
	{
		wprintf(L"PMT => firesat [%d]\n", p->pid);

		__int64 tmp = f.GetPos();

		if(tmp >= 8)
		{
			f.Seek(tmp - 8);

			int len = std::min<int>((int)(end - (tmp - 8)), 1024);

			FIRESAT_CA_DATA instance;
			FIRESAT_CA_DATA data;

			memset(&instance, 0, sizeof(instance));
			memset(&data, 0, sizeof(data));

			data.uSlot = 0;
			data.uTag = 2;
			data.bMore = FALSE;
			data.uData[0] = 3; // List Management = ONLY
			data.uData[1] = 1; // pmt_cmd = OK DESCRAMBLING
			data.uLength = 2 + len;

			f.ByteRead(&data.uData[2], len);

			// ???
			BYTE b = data.uData[3]; data.uData[3] = data.uData[4]; data.uData[4] = b;
			b = data.uData[5]; data.uData[5] = data.uData[6]; data.uData[6] = b;
			
			m_ksps->Set(__uuidof(Firesat), FIRESAT_HOST2CA, &instance, sizeof(instance), &data, sizeof(data));

			m_firesat_pmt_sent = true;
		}

		f.Seek(tmp);
	}

	// ci

	if(m_ci != NULL && (m_tmpsid == 0 && p->sid == m_sid || m_tmpsid != 0 && p->sid == m_tmpsid))
	{
		if(m_ci_pmt_sent == 0)
		{
			wprintf(L"PMT => Common Interface [%d]\n", p->pid);

			__int64 tmp = f.GetPos();

			if(tmp >= 8)
			{
				f.Seek(tmp - 8);

				int len = std::min<int>((int)(end - (tmp - 8)), 1024);

				BYTE* buff = new BYTE[len];

				f.ByteRead(buff, len);

				HRESULT hr = m_ci->SendPMT(buff, len);

				delete [] buff;
			}

			f.Seek(tmp);
		}

		m_ci_pmt_sent = ++m_ci_pmt_sent & 7;
	}

	//

	std::set<int> missing;

	for(auto i = p->streams.begin(); i != p->streams.end(); i++)
	{
		missing.insert(i->second->pid);
	}

	start = f.GetPos();

	f.BitRead(3); // reserved
	p->pcr = (int)f.BitRead(13);
	f.BitRead(4); // reserved
	int program_info_length = (int)f.BitRead(12);

	p->ecms.clear();

	{
		__int64 next = std::min<__int64>(end, f.GetPos() + program_info_length);

		while(f.GetPos() < next)
		{
			int tag = (int)f.BitRead(8);
			int len = (int)f.BitRead(8);

			__int64 next2 = std::min<__int64>(next, f.GetPos() + len);

			if(tag == 9)
			{
				WORD CA_system_id = (WORD)f.BitRead(16);
				f.BitRead(3); // reserved
				int pid = (int)f.BitRead(13); // ecm pid

				p->ecms[pid] = CA_system_id;
			}

			f.Seek(next2);
		}
		
		f.Seek(next);
	}

	while(f.GetPos() < end)
	{
		int type = (int)f.BitRead(8);
		f.BitRead(3); // reserved
		int pid = (int)f.BitRead(13);
		f.BitRead(4); // reserved
		int ES_info_length = (int)f.BitRead(12);

		__int64 next = std::min<__int64>(f.GetPos() + ES_info_length, f.GetLength());

		std::map<int, WORD> ecms;

		int type_found = 0;

		switch(type)
		{
		case 1: // mpeg-1 video
		case 2: // mpeg-2 video
		case 27: // h264
		case 36: // h265
		case 0xea: // vc1
			type_found = type;
			break;

		case 3: // mpeg-1 audio
		case 4: // mpeg-2 audio
		case 15: // aac (?)
		case 17: // latm
		case 0x80: // ?
		case 0x81: // ac3
			type_found = type;
			break;

		case 6:
			break;

		default:
			break;
		}

		while(f.GetPos() < next)
		{
			int tag = (int)f.BitRead(8);
			int len = (int)f.BitRead(8);

			__int64 next2 = std::min<__int64>(next, f.GetPos() + len);

			if(type == 6)
			{
				if(tag == 0x6a)
				{
					type_found = 0x81;
				}
				else if(tag == 0x56)
				{
					type_found = 0x100; // teletext
				}
			}
			
			if(tag == 9)
			{
				WORD CA_system_id = (WORD)f.BitRead(16);
				f.BitRead(3); // reserved
				int ecm_pid = (int)f.BitRead(13);

				ecms[ecm_pid] = CA_system_id;
			}

			f.Seek(next2);
		}

		/*
			http://forum.handbrake.fr/viewtopic.php?f=4&t=7461

			To my best knowledge this is the right way to demux TS and m2ts files:

			0x01: MPEG2 video
			0x02: MPEG2 video
			0x03: MP2 audio (MPEG-1 Audio Layer II)
			0x04: MP2 audio (MPEG-2 Audio Layer II)
			0x06: private data (can be AC3, DTS or something else)
			0x0F: AAC audio (MPEG-2 Part 7 Audio)
			0x11: AAC audio (MPEG-4 Part 3 Audio)
			0x1B: h264 video
			0x80: MPEG2 video or PCM audio
			0x81: AC3 audio
			0x82: DTS audio
			0x83: TrueHD/AC3 interweaved audio
			0x84: E-AC3 audio
			0x85: DTS-HD High Resolution Audio
			0x86: DTS-HD Master Audio
			0x87: E-AC3 audio
			0xA1: secondary E-AC3 audio
			0xA2: secondary DTS audio
			0xEA: VC-1 video

			There are two problematic situations:

			(1) 0x80 can be either MPEG2 video or LPCM audio. In the M2TS
			container 0x80 should always be LPCM audio. In the TS container
			0x80 is normally MPEG2 video. BUT - if people convert an M2TS
			stream to TS, 0x80 can be PCM. Also if people convert a TS stream
			to M2TS, 0x80 could be MPEG2 video. The most reliable way to
			figure out what it really is is checking the descriptors. If there's a
			descriptor 0x05 with the format_descriptor "HDMV" then the track
			originates from a Blu-Ray and thus is PCM.

			(2) 0x06 can be AC3, DTS or something else. You need to check
			the descriptors to find out what it really is. If there's a descriptor
			0x05 with the format_identifier of "DTS1", "DTS2", "DTS3" or
			"AC-3" then you know what the track is. If there's a descriptor
			0x05 with the format_identifier "BSSD" then it's an old style (non
			Blu-Ray) LPCM track. I don't know how to handle such a track.
			Have no sample for that. Finally, if there's no descriptor 0x05
			which tells you which format this track is, you can look for
			descriptors 0x6a (DVB AC3), 0x73 (DVB DTS) or 0x81 (ATSC
			AC3). If any of these 3 descriptors is present, again you know
			what the 0x06 track contains.
		*/

		f.Seek(next);

		if(type_found)
		{
			TSDemuxStream* s = NULL;

			auto i = m_streams.find(pid);

			if(i != m_streams.end())
			{
				s = i->second;

				if(s->type != type_found) 
				{
					s->type = type_found;

					s->mt.InitMediaType();
				}

				missing.erase(pid);
			}
			else
			{
				s = new TSDemuxStream();

				s->pid = pid;
				s->type = type_found;

				m_streams[pid] = s;
			}

			/*
			if(s->type == 27)
			{
				s->mt.majortype = MEDIATYPE_Video;
				s->mt.subtype = MEDIASUBTYPE_H264;
				s->mt.formattype = FORMAT_VideoInfo2;
				VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)s->mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
				memset(vih, 0, sizeof(VIDEOINFOHEADER2));
				vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
				vih->bmiHeader.biCompression = '462H';
				vih->bmiHeader.biWidth = 640;
				vih->bmiHeader.biHeight = 480;
				vih->dwPictAspectRatioX = 16;
				vih->dwPictAspectRatioY = 9;
			}
			else
			*/
			if(s->type == 0x100)
			{
				s->mt.majortype = MEDIATYPE_TeletextPacket;
			}

			if(!s->mt.IsValid())
			{
				auto i = m_cached_streams.find(((__int64)m_freq << 16) | s->pid);
				
				if(i != m_cached_streams.end())
				{
					s->mt = i->second;
				}
			}

			if(p->streams.find(pid) == p->streams.end())
			{
				p->streams[pid] = s;
			}

			s->ecms.insert(ecms.begin(), ecms.end());
		}
	}

	for(auto i = missing.begin(); i != missing.end(); i++)
	{
		p->streams.erase(*i);
	}
/*
	if(changed)
	{
		// TODO: delete unused elements of m_stream
	}
*/
	p->missing = -10;

	if(p->packet == 0)
	{
		p->packet = m_packet.payload;

		wprintf(L"PMT [%d]", p->pid);

		for(auto i = p->streams.begin(); i != p->streams.end(); i++)
		{
			wprintf(L" %d", i->first);
		}

		wprintf(L" @ %I64d\n", m_packet.total);

		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			i->second->missing = -10;
		}
	}
}

void CTSDemuxFilter::OnNIT(CBaseSplitterFile& f)
{
	__int64 end;

	if(CheckSection(f, end) != 0x40)
	{
		return;
	}

	CAutoLock cAutoLock(&m_csPrograms);
/*
	f.BitRead(4); // reserved
	int network_descriptors_length = (int)f.BitRead(12);

	while(f.GetRemaining() > 0 && network_descriptors_length > 0)
	{
		int tag = f.BitRead(8);
		int len = f.BitRead(8);

		network_descriptors_length -= len + 2;

		__int64 next = f.GetPos() + len;

		CStringA str;

		switch(tag)
		{
		case 0x40:
			f.ByteRead((BYTE*)str.GetBufferSetLength(len), len);
			break;
		}

		f.Seek(next);
	}

	f.BitRead(4); // reserved
	int transport_stream_loop_length = (int)f.BitRead(12);

	while(f.GetRemaining() > 0 && transport_stream_loop_length > 0)
	{
		int tsid = (int)f.BitRead(16);
		int onid = (int)f.BitRead(16);
		f.BitRead(4); // reserved
		int transport_descriptors_length = f.BitRead(12);

		transport_stream_loop_length -= transport_descriptors_length + 6;

		__int64 next = f.GetPos() + transport_stream_loop_length;

		while(f.GetRemaining() > 0 && transport_descriptors_length > 0)
		{
			break;
		}

		f.Seek(next);
	}
*/
}

void CTSDemuxFilter::OnSDT(CBaseSplitterFile& f)
{
	__int64 end;

	if(CheckSection(f, end) != 0x42)
	{
		return;
	}

	CAutoLock cAutoLock(&m_csPrograms);

	if(m_programs.empty())
	{
		return;
	}

	f.BitRead(16); // onid
	f.BitRead(8); // ?

	while(f.GetPos() < end)
	{
		int sid = (int)f.BitRead(16);
		f.BitRead(11); // ?
		bool scrambled = f.BitRead() != 0;
		int len = (int)f.BitRead(12);

		int type = 0;

		char* provider = NULL;
		char* name = NULL;

		int provider_len = 0;
		int name_len = 0;

		__int64 next = min(f.GetPos() + len, f.GetLength());

		while(f.GetPos() < next)
		{
			int tag = (int)f.BitRead(8);
			int len = (int)f.BitRead(8);

			__int64 next = min(f.GetPos() + len, f.GetLength());

			if(tag == 0x48)
			{
				if(provider == NULL && name == NULL)
				{
					type = f.BitRead(8);

					provider_len = (int)f.BitRead(8);
					provider = new char[provider_len + 1];
					f.ByteRead((BYTE*)provider, provider_len);
					provider[provider_len] = 0;

					name_len = (int)f.BitRead(8);
					name = new char[name_len + 1];
					f.ByteRead((BYTE*)name, name_len);
					name[name_len] = 0;
				}
			}

			f.Seek(next);
		}

		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			TSDemuxProgram* p = i->second;

			if(p->sid == sid)
			{
				p->scrambled = scrambled;
				p->type = type;
				p->provider = DirectShow::StripDVBName(provider, provider_len);
				p->name = DirectShow::StripDVBName(name, name_len);

				// wprintf(L"%s %d (%s) %d\n", p->name.c_str(), name_len, p->provider.c_str(), provider_len);
			}
		}

		delete [] provider;
		delete [] name;
	}

	if(!m_sdt.Check() && ++m_sdt_count == 2)
	{
		m_sdt.Set();

		wprintf(L"SDT @ %I64d\n", m_packet.payload);
	}
}

void CTSDemuxFilter::OnEIT(CBaseSplitterFile& f)
{
	__int64 end;

	int table_id = CheckSection(f, end);

	if(table_id < 0x4e || table_id >= 0x70) 
	{
		return;
	}

	CAutoLock cAutoLock(&m_csPrograms);

	// TODO
}

void CTSDemuxFilter::OnVCT(CBaseSplitterFile& f)
{
	__int64 end;

	if(CheckSection(f, end) != 0xc8) 
	{
		return;
	}

	CAutoLock cAutoLock(&m_csPrograms);

	// TODO
}

static BYTE* NextMpegStartCode(BYTE* buff, BYTE& code, int len, BYTE id)
{
	DWORD dw = 0xffffffff;

	do
	{
		if(len-- == 0) 
		{
			return NULL;
		}

		dw = (dw << 8) | *buff++;
	}
	while((dw & 0xffffff00) != 0x00000100 || id != 0 && id != (BYTE)(dw & 0xff));

	code = (BYTE)(dw & 0xff);

	return buff;
}

void CTSDemuxFilter::OnPES(CBaseSplitterFile& f, TSDemuxStream* s, bool complete, int pid)
{
	__int64 next = 0;

	while(f.BitRead(24) == 0x000001)
	{
		BYTE id = f.BitRead(8);

		CBaseSplitterFile::PESHeader h;

		if(!f.Read(h, id))
		{
			// ASSERT(0);

			break;
		}

		// ASSERT(h.fpts);

		int len = h.len;

		__int64 start = f.GetPos();

		if(len == 0)
		{
			BYTE id2;

			__int64 remaining = f.GetRemaining();

			if(remaining > 184)
			{
				f.Seek(f.GetLength() - 184);

				remaining = 184;
			}
/*
			if(!f.NextMpegStartCode(id2, remaining, id))
*/
			BYTE buff[184];

			f.ByteRead(buff, remaining);
			
			BYTE* p = NextMpegStartCode(buff, id2, remaining, id);

			if(p == NULL)
			{
				if(complete)
				{
					f.Seek(f.GetLength());
				}
				else
				{
					break;
				}
			}
			else
			{
				f.Seek(f.GetLength() - 184 + (p - buff));
			}

			len = f.GetPos() - start;

			f.Seek(start);
		}

		if(start + len > f.GetLength())
		{
			break;
		}

		next = start + len;

		if(!s->mt.IsValid() && s->DetectMediaType(f, len, id)) // h.fpts && ?
		{
			s->packet = m_packet.payload;

			m_cached_streams[((__int64)m_freq << 16) | s->pid] = s->mt;

			wprintf(L"TYP [%d] @ %I64d %s\n", s->pid, s->packet, CMediaTypeEx(s->mt).ToString().c_str());
		}

		if(h.fpts)
		{
			for(auto i = m_programs.begin(); i != m_programs.end(); i++)
			{
				TSDemuxProgram* p = i->second;

				if(p->sid == m_sid)
				{
					auto j = p->streams.find(s->pid);

					if(j != p->streams.end())
					{
						if(j->second->type == 0x100)
						{
							h.pts = m_time.pcr; // pts of teletext is not reliable, replace it with pcr
						}

						m_time.pts = h.pts;
					}

					break;
				}
			}
		}

		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			CTSDemuxOutputPin* pin = *i;

			if(pin->GetPID() == s->pid)
			{
				TSDemuxQueueItem* item = new TSDemuxQueueItem();

				item->type = TSDemuxQueueItem::Sample;

				item->buff.resize(len);

				f.ByteRead(&item->buff[0], len);

				if(h.fpts && m_time.start >= 0)
				{
					item->pts = h.pts - m_time.start + m_time.offset;
				}

				pin->Deliver(item);
			}
		}

		f.Seek(next);
	}

	f.Seek(next);
}

void CTSDemuxFilter::OnPCR(int pid, REFERENCE_TIME pcr)
{
	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		TSDemuxProgram* p = i->second;

		if(p->sid == m_sid && p->pcr == pid && p->packet)
		{
			if(m_time.start < 0)
			{
				m_time.start = pcr;
				m_time.offset = 0;

				DeliverNewSegment(0); // pcr?
			}
			else if(m_time.newstart == 0 && (m_time.pcr > pcr || m_time.pcr + 10000000 < pcr))
			{
				m_time.offset += m_time.pcr - pcr;

				for(auto i = p->streams.begin(); i != p->streams.end(); i++)
				{
					TSDemuxStream* s = i->second;

					auto pair = m_payload.find(s->pid);

					if(pair != m_payload.end())
					{
						pair->second->m_size = 0;
					}
				}
			}

			m_time.pcr = pcr;

			break;
		}
	}
}

void CTSDemuxFilter::OnSmartCardPacket(int type, std::vector<BYTE>* buff, WORD pid, WORD system_id, bool required)
{
	if(buff == NULL)
	{
		return;
	}

	// TODO: lock

	if(m_scs != NULL)
	{
		// filter dups first

		DWORD id = ((DWORD)system_id << 16) | (type << 13) | pid;

		std::vector<BYTE>* cbuff = NULL;

		auto i = m_cache.find(id);

		if(i != m_cache.end())
		{
			cbuff = i->second;

			if(cbuff->size() == buff->size() && memcmp(cbuff->data(), buff->data(), buff->size()) == 0)
			{
				delete buff;

				return;
			}

			if(cbuff->size() != buff->size())
			{
				cbuff->resize(buff->size());
			}
		}
		else
		{
			cbuff = new std::vector<BYTE>(buff->size());

			m_cache[id] = cbuff;
		}

		memcpy(cbuff->data(), buff->data(), buff->size());

		m_scs->Process(m_scs_cookie, type, buff->data(), buff->size(), pid, system_id, required);
	}

	delete buff;
}

int CTSDemuxFilter::CheckSection(CBaseSplitterFile& f, __int64& end)
{
	__int64 start = f.GetPos();

	CBaseSplitterFile::TSSectionHeader h;

	if(!f.Read(h))
	{
		f.Seek(start);

		return -1;
	}

	end = start + h.section_length;

	if(end > f.GetLength())
	{
		f.Seek(start);

		return -1;
	}

	return h.table_id;
}

bool CTSDemuxFilter::IsProgramReady(int sid)
{
	CAutoLock cAutoLock(&m_csPrograms);

	if(sid != 0)
	{
		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			const TSDemuxProgram* p = i->second;

			if(sid == p->sid)
			{
				if(p->packet == 0) 
				{
					wprintf(L"PMT [%d] ???\n", p->pid);

					return false;
				}

				return true;
			}
		}
	}
	else
	{
		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			const TSDemuxProgram* p = i->second;

			if(p->missing > 0) // || p->scrambled
			{
				continue;
			}

			if(p->packet == 0) 
			{
				wprintf(L"PMT [%d] ???\n", p->pid);

				return false;
			}
		}

		return true;
	}

	return false;
}

bool CTSDemuxFilter::IsProgramDecryptable(int sid)
{
	CAutoLock cAutoLock(&m_csPrograms);

	if(m_ksps)
	{
		if(SUCCEEDED(m_ksps->Set(__uuidof(Firesat), FIRESAT_TEST_INTERFACE, NULL, NULL, NULL, NULL)))
		{
			return true; // TODO: "firesat"->HasCard(...)
		}
	}

	if(m_ci && m_ci->HasCard() == S_OK)
	{
		return true;
	}

	int streams = 0;

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		const TSDemuxProgram* p = i->second;

		if(sid == p->sid)
		{
			for(auto i = p->streams.begin(); i != p->streams.end(); i++)
			{
				TSDemuxStream* s = i->second;

				if(s->scrambled)
				{
					if(m_scs != NULL)
					{
						if(SUCCEEDED(m_scs->CheckSystemId(0))) streams++;

						/*

						// TODO: test me

						const std::map<int, WORD>& ecms = !s->ecms.empty() ? s->ecms : p->ecms;

						for(auto i = ecms.begin(); i != ecms.end(); i++)
						{
							if(SUCCEEDED(m_scs->CheckSystemId(i->second)))
							{
								streams++;

								break;
							}
						}

						*/
					}
				}
				else
				{
					streams++;
				}
			}

			break;
		}
	}

	return streams > 0;
}

bool CTSDemuxFilter::IsTypeReady(int sid)
{
	CAutoLock cAutoLock(&m_csPrograms);

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		const TSDemuxProgram* p = i->second;

		if(sid == p->sid)
		{
			if(p->packet == 0) 
			{
				wprintf(L"PMT [%d] ???\n", p->pid);

				return false;
			}

			for(auto i = p->streams.begin(); i != p->streams.end(); i++)
			{
				TSDemuxStream* s = i->second;

				if(!s->mt.IsValid()) // && s->payloadstart < 10 // && !s->scrambled
				{
					wprintf(L"TYP [%d] ??? (%d %d)\n", s->pid, p->pid, s->payloadstart);

					return false;
				}
			}

			return true;
		}
	}

	return false;
}

// IMediaSeeking (TODO)

STDMETHODIMP CTSDemuxFilter::GetCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL)
	{
		return E_POINTER;
	}
	
	*pCapabilities = 
		AM_SEEKING_CanGetStopPos |
		AM_SEEKING_CanGetDuration |
		AM_SEEKING_CanSeekAbsolute;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::CheckCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL)
	{
		return E_POINTER;
	}
	
	if(*pCapabilities == 0)
	{
		return S_OK;
	}
	
	DWORD caps;

	GetCapabilities(&caps);

	if((caps & *pCapabilities) == 0)
	{
		return E_FAIL;
	}

	if(caps == *pCapabilities)
	{
		return S_OK;
	}

	return S_FALSE;
}

STDMETHODIMP CTSDemuxFilter::IsFormatSupported(const GUID* pFormat)
{
	if(pFormat == NULL)
	{
		return E_POINTER;
	}

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CTSDemuxFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CTSDemuxFilter::GetTimeFormat(GUID* pFormat)
{
	if(pFormat == NULL)
	{
		return E_POINTER;
	}

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CTSDemuxFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CTSDemuxFilter::GetDuration(LONGLONG* pDuration)
{
	if(pDuration == NULL)
	{
		return E_POINTER;
	}

	*pDuration = m_time.last - m_time.first;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::GetStopPosition(LONGLONG* pStop)
{
	return GetDuration(pStop);
}

STDMETHODIMP CTSDemuxFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	if(pCurrent == NULL)
	{
		return E_POINTER;
	}

	*pCurrent = m_time.pcr - m_time.first;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CTSDemuxFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if((dwCurrentFlags & AM_SEEKING_PositioningBitsMask) != AM_SEEKING_AbsolutePositioning)
	{
		return E_INVALIDARG;
	}

	m_time.newstart = *pCurrent + m_time.first;

	printf("seek start\n");

	if(ThreadExists())
	{
		DeliverBeginFlush();

		CallWorker(CMD_SEEK);

		DeliverEndFlush();
	}

	printf("seek end\n");

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent == NULL || pStop == NULL)
	{
		return E_POINTER;
	}

	*pCurrent = m_time.pcr - m_time.first;
	*pStop = m_time.last - m_time.first;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest == NULL || pLatest == NULL)
	{
		return E_POINTER;
	}

	*pEarliest = 0;
	*pLatest = 	m_time.last - m_time.first;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::SetRate(double dRate)
{
	return E_NOTIMPL;
}

STDMETHODIMP CTSDemuxFilter::GetRate(double* pdRate)
{
	if(pdRate == NULL)
	{
		return E_POINTER;
	}

	*pdRate = 1.0;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::GetPreroll(LONGLONG* pllPreroll)
{
	if(pllPreroll == NULL)
	{
		return E_POINTER;
	}

	*pllPreroll = 0;

	return S_OK;
}

// ITSDemuxFilter

STDMETHODIMP CTSDemuxFilter::SetFreq(int freq)
{
	ASSERT(m_State == State_Stopped);

	CAutoLock cAutoLock2(&m_csPrograms);

	if(m_freq != freq)
	{
		m_freq = freq;

		ResetPrograms();
	}

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::ResetPrograms()
{
	CAutoLock cAutoLock(&m_csReceive);

	CAutoLock cAutoLock2(&m_csPrograms);

	m_pat.Reset();
	m_sdt.Reset();

	m_sdt_count = 0;

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		delete i->second;
	}

	for(auto i = m_streams.begin(); i != m_streams.end(); i++)
	{
		delete i->second;
	}

	m_emms.clear();
	m_programs.clear();
	m_streams.clear();

	m_packet.total = 0;
	m_packet.pat = 0;
	
	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::GetPrograms(std::list<TSDemuxProgram*>& programs, int sid, bool sdt, bool wait)
{
	int pat_timeout = 0;
	int sdt_timeout = 0;
	int pmt_timeout = 0;
	int typ_timeout = 0;

	if(wait)
	{
		pat_timeout = 1000;
		sdt_timeout = 20000;
		pmt_timeout = 2000;
		typ_timeout = 10000;
	}

	programs.clear();

	{
		CAutoLock cAutoLock(&m_csPrograms);

		for(auto i = m_streams.begin(); i != m_streams.end(); i++)
		{
			i->second->payloadstart = 0;
		}
	}

	// wprintf(L"GetPrograms\n");

	m_tmpsid = sid;

	m_firesat_pmt_sent = false;
	m_ci_pmt_sent = 0;

	clock_t start = clock();

	for(int i = 0; i < 20; i++)
	{
		if(m_packet.total > 0)
		{
			break;
		}

		Sleep(50);
	}

	bool ready = false;

	if(m_pat.Wait(pat_timeout))
	{
		clock_t limit = clock() + pmt_timeout;

		do
		{
			if(IsProgramReady(sid))
			{
				if(sid > 0 && IsProgramDecryptable(sid))
				{
					limit = clock() + typ_timeout;

					do
					{
						if(IsTypeReady(sid))
						{
							ready = true;

							break;
						}

						if(typ_timeout > 0)
						{
							Sleep(50);
						}
					}
					while(clock() < limit);
				}
				else
				{
					ready = true;
				}

				break;
			}

			if(pmt_timeout > 0)
			{
				Sleep(50);
			}
		}
		while(clock() < limit);

		if(!ready && pmt_timeout > 0)
		{
			wprintf(L"PMT/TYP timeout\n");
		}

		if(sdt)
		{
			if(!m_sdt.Wait(sdt_timeout))
			{
				wprintf(L"SDT late\n");
			}
		}
	}
	else
	{
		wprintf(L"PAT timeout\n");
	}

	sdt = !!m_sdt.Check();

	HRESULT hr = E_FAIL;

	if(ready)
	{
		CAutoLock cAutoLock(&m_csPrograms);

		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			TSDemuxProgram* p = i->second;

			if(sdt && p->name.empty() || p->streams.empty())
			{
				continue;
			}

			programs.push_back(p);
		}

		wprintf(L"GetPrograms: %dms %I64d\n", clock() - start, m_packet.total);

		hr = S_OK;
	}

	m_tmpsid = 0;

	return hr;
}

STDMETHODIMP CTSDemuxFilter::SetProgram(int sid, bool dummy)
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped) 
	{
		return VFW_E_NOT_STOPPED;
	}

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		CTSDemuxOutputPin* pin = *i;

		if(IPin* pPinTo = pin->GetConnected())
		{
			m_pGraph->Disconnect(pPinTo);
			m_pGraph->Disconnect(pin);
		}

		delete pin;
	}

	m_outputs.clear();

	CAutoLock cAutoLock2(&m_csReceive);

	m_time.start = -1;
	m_time.pcr = -1;
	m_time.pts = -1;

	m_sid = sid;

	m_firesat_pmt_sent = false;
	m_ci_pmt_sent = 0;

	bool video = false;
	bool audio = false;

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		TSDemuxProgram* p = i->second;

		if(p->sid == sid)
		{
			for(auto i = p->streams.begin(); i != p->streams.end(); i++)
			{
				const TSDemuxStream* s = i->second;

				if(s->mt.IsValid() && (s->type != 0x100 || s->payloadstart > 0))
				{
					HRESULT hr;

					m_outputs.push_back(new CTSDemuxOutputPin(this, *s, &hr, L"Output"));

					if(s->mt.majortype == MEDIATYPE_Video) video = true;
					else if(s->mt.majortype == MEDIATYPE_Audio) audio = true;
				}
			}
		}
	}

	if(!video && !audio)
	{
		if(dummy) // HACK: just to be able to start timeshifting later...
		{
			HRESULT hr;

			m_dummy.video.mt.InitMediaType();
			m_dummy.video.mt.majortype = MEDIATYPE_Video;
			m_dummy.video.mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
			m_dummy.video.mt.formattype = FORMAT_MPEG2Video;
			m_dummy.video.mt.bFixedSizeSamples = TRUE;
			m_dummy.video.mt.bTemporalCompression = 0;

			MPEG2VIDEOINFO* vf = (MPEG2VIDEOINFO*)m_dummy.video.mt.AllocFormatBuffer(sizeof(MPEG2VIDEOINFO));

			memset(vf, 0, sizeof(*vf));

			vf->hdr.bmiHeader.biSize = sizeof(vf->hdr.bmiHeader);
			vf->hdr.bmiHeader.biWidth = 720; 
			vf->hdr.bmiHeader.biHeight = 576;
			vf->hdr.rcSource.right = vf->hdr.rcTarget.right = vf->hdr.bmiHeader.biWidth;
			vf->hdr.rcSource.bottom = vf->hdr.rcTarget.bottom = vf->hdr.bmiHeader.biHeight;
			vf->hdr.dwBitRate = 4000000;
			vf->hdr.AvgTimePerFrame = 400000; // 25fps
			vf->hdr.dwPictAspectRatioX = 4;
			vf->hdr.dwPictAspectRatioY = 3;
			vf->dwProfile = 2;
			vf->dwLevel = 2;

			m_outputs.push_back(new CTSDemuxOutputPin(this, m_dummy.video, &hr));

			m_dummy.audio.mt.InitMediaType();
			m_dummy.audio.mt.majortype = MEDIATYPE_Audio;
			m_dummy.audio.mt.subtype = MEDIASUBTYPE_MPEG2_AUDIO;
			m_dummy.audio.mt.formattype = FORMAT_WaveFormatEx;
			m_dummy.audio.mt.bFixedSizeSamples = TRUE;
			m_dummy.audio.mt.bTemporalCompression = 0;

			MPEG1WAVEFORMAT* af = (MPEG1WAVEFORMAT*)m_dummy.audio.mt.AllocFormatBuffer(sizeof(MPEG1WAVEFORMAT));
			
			memset(af, 0, sizeof(*af));
			
			af->wfx.wFormatTag = WAVE_FORMAT_MPEG;
			af->wfx.nChannels = 2;
			af->wfx.nSamplesPerSec = 48000;
			af->wfx.nAvgBytesPerSec = 32000;
			af->wfx.nBlockAlign = 768;
			af->wfx.cbSize = sizeof(*af) - sizeof(af->wfx);
			af->fwHeadLayer = ACM_MPEG_LAYER2;
			af->fwHeadMode = ACM_MPEG_STEREO;
			af->wHeadEmphasis = 1;

			m_outputs.push_back(new CTSDemuxOutputPin(this, m_dummy.audio, &hr));

			return S_FALSE;
		}

		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::HasProgram(int sid, int freq)
{
	CAutoLock cAutoLock(this);

	if(freq == 0 || freq == m_freq)
	{
		for(auto i = m_programs.begin(); i != m_programs.end(); i++)
		{
			TSDemuxProgram* p = i->second;

			if(p->sid == sid)
			{
				for(auto i = p->streams.begin(); i != p->streams.end(); i++)
				{
					const TSDemuxStream* s = i->second;

					if(!s->mt.IsValid())
					{
						return E_FAIL;
					}
				}

				return S_OK;
			}
		}
	}

	return E_FAIL;
}

STDMETHODIMP CTSDemuxFilter::IsScrambled(int sid)
{
	CAutoLock cAutoLock(&m_csPrograms);

	if(sid == 0)
	{
		sid = m_sid;
	}

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		const TSDemuxProgram* p = i->second;
		
		if(p->sid == sid)
		{
			if(!p->ecms.empty()) return S_OK;

			for(auto i = p->streams.begin(); i != p->streams.end(); i++)
			{
				const TSDemuxStream* s = i->second;

				if(!s->ecms.empty()) return S_OK;
			}

			return S_FALSE;
		}
	}

	return E_FAIL;
}

STDMETHODIMP CTSDemuxFilter::SetWriterOutput(TSWriterOutputType type) 
{
	m_record = type;

	return S_OK;
}

STDMETHODIMP_(__int64) CTSDemuxFilter::GetReceivedBytes()
{
	CAutoLock cAutoLock(&m_csReceive);

	return m_received;
}

STDMETHODIMP_(float) CTSDemuxFilter::GetReceivedErrorRate()
{
	CAutoLock cAutoLock(&m_csReceive);

	return m_received_error;
}

STDMETHODIMP CTSDemuxFilter::SetKsPropertySet(IKsPropertySet* ksps)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_ksps = ksps;

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::SetSmartCardServer(ISmartCardServer* scs)
{
	// TODO: lock

	if(m_scs != NULL)
	{
		m_scs->Unregister(m_scs_cookie);

		m_scs_cookie = 0;
	}

	m_scs = scs;

	if(m_scs != NULL)
	{
		m_scs->Register(this, &m_scs_cookie);
	}

	return S_OK;
}

STDMETHODIMP CTSDemuxFilter::SetCommonInterface(ICommonInterface* ci)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_ci = ci;

	return S_OK;
}

// ISmartCardClient

STDMETHODIMP CTSDemuxFilter::OnDCW(WORD pid, const BYTE* cw)
{
	CAutoLock cAutoLock(&m_csPrograms);

	for(auto i = m_programs.begin(); i != m_programs.end(); i++)
	{
		TSDemuxProgram* p = i->second;

		if(p->ecms.find(pid) != p->ecms.end())
		{
			p->csa.SetKey(cw);

			if(!p->gotkey)
			{
				p->gotkey = true;

				for(auto i = p->streams.begin(); i != p->streams.end(); i++)
				{
					i->second->payloadstart = 0;
				}
			}

			wprintf(L"PROGRAM KEY %d\n", p->pid);
		}
	}

	for(auto i = m_streams.begin(); i != m_streams.end(); i++)
	{
		TSDemuxStream* s = i->second;

		auto j = s->ecms.find(pid);

		if(j != s->ecms.end())
		{
			s->csa.SetKey(cw);

			if(!s->gotkey)
			{
				s->gotkey = true;

				s->payloadstart = 0;
			}

			wprintf(L"STREAM KEY %d\n", s->pid);
		}
	}

	return S_OK;
}

//

CTSDemuxFilter::CTSDemuxInputPin::CTSDemuxInputPin(CTSDemuxFilter* pFilter, HRESULT* phr)
	: CBaseInputPin(_T("Input"), pFilter, pFilter, phr, L"Input")
	, m_queue(100)
{
}

HRESULT CTSDemuxFilter::CTSDemuxInputPin::CheckMediaType(const CMediaType* pmt)
{
	return ((CTSDemuxFilter*)m_pFilter)->CheckInput(pmt);
}

HRESULT CTSDemuxFilter::CTSDemuxInputPin::CheckConnect(IPin* pPin)
{
	return SUCCEEDED(((CTSDemuxFilter*)m_pFilter)->CheckInput(pPin)) ? __super::CheckConnect(pPin) : E_FAIL;
}

HRESULT CTSDemuxFilter::CTSDemuxInputPin::CompleteConnect(IPin* pPin)
{
	return SUCCEEDED(((CTSDemuxFilter*)m_pFilter)->CompleteInput(pPin)) ? __super::CompleteConnect(pPin) : E_FAIL;
}

HRESULT CTSDemuxFilter::CTSDemuxInputPin::BreakConnect()
{
	((CTSDemuxFilter*)m_pFilter)->BreakInput();

	return __super::BreakConnect();
}

STDMETHODIMP CTSDemuxFilter::CTSDemuxInputPin::Receive(IMediaSample* pSample)
{
	HRESULT hr;

	hr = __super::Receive(pSample);

	if(FAILED(hr)) return hr;

	const AM_SAMPLE2_PROPERTIES* pProps = SampleProps();

	if(pProps->dwStreamId != AM_STREAM_MEDIA)
	{
		return S_OK;
	}

	AM_MEDIA_TYPE* pmt;
	
	if(SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
	}

	BYTE* p = NULL;

	if(FAILED(hr = pSample->GetPointer(&p)))
	{
		return hr;
	}

	long len = pSample->GetActualDataLength();

	std::vector<BYTE>* buff = new std::vector<BYTE>(len);

	memcpy(buff->data(), p, len);

	// if(m_queue.GetCount() > 1) wprintf(L"%d / %d, %d, %02x\n", m_queue.GetCount(), m_queue.GetMaxCount(), len, len >= 4 ? p[0] : -1);

	m_queue.Enqueue(buff);

	return S_OK;
}

DWORD CTSDemuxFilter::CTSDemuxInputPin::ThreadProc()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	HANDLE handles[] = {m_exit, m_queue.GetEnqueueEvent()};

	while(m_hThread != NULL)
	{
		switch(::WaitForMultipleObjects(countof(handles), handles, FALSE, INFINITE))
		{
		default:

			ASSERT(0);
			return 0;

		case WAIT_OBJECT_0 + 0: 

			return 0;

		case WAIT_OBJECT_0 + 1:

			while(m_queue.GetCount() > 0)
			{
				std::vector<BYTE>* buff = m_queue.Dequeue();

				((CTSDemuxFilter*)m_pFilter)->Receive(buff->data(), buff->size());

				delete buff;
			}

			break;
		}
	}

	return 0;
}

HRESULT CTSDemuxFilter::CTSDemuxInputPin::Active()
{
	m_exit.Reset();

	CAMThread::Create();

	return __super::Active();
}

HRESULT CTSDemuxFilter::CTSDemuxInputPin::Inactive()
{
	m_exit.Set();

	CAMThread::Close();

	while(m_queue.GetCount() > 0)
	{
		delete m_queue.Dequeue();
	}

	return __super::Inactive();
}

// CTSDemuxOutputPin

CTSDemuxFilter::CTSDemuxOutputPin::CTSDemuxOutputPin(CTSDemuxFilter* pFilter, const TSDemuxStream& stream, HRESULT* phr, LPCWSTR name)
	: CBaseOutputPin(_T("CTSDemuxOutputPin"), pFilter, pFilter, phr, name)
	, m_queue(stream.mt.majortype != MEDIATYPE_Audio ? 1000 : 100)
	, m_flushing(false)
	, m_size(0)
	, m_pts(0)
{
	if(phr) *phr = S_OK;

	m_stream.mt = stream.mt;
	m_stream.type = stream.type;
	m_stream.pid = stream.pid;
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::CheckMediaType(const CMediaType* pmt)
{
	for(int i = 0; ; i++)
	{
		CMediaType mt;

		if(S_OK != GetMediaType(i, &mt))
		{
			break;
		}

		if(mt == *pmt)
		{
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(m_stream.mt.subtype == MEDIASUBTYPE_H264)
	{
		*pmt = m_stream.mt;

		if(iPosition == 0) 
		{
			pmt->subtype = MEDIASUBTYPE_H264_TRANSPORT;
		}
		else if(iPosition > 1)
		{
			return VFW_S_NO_MORE_ITEMS;
		}
/*
		else if(iPosition == 2)
		{
			pmt->subtype = MEDIASUBTYPE_MPEG2_VIDEO;
			pmt->formattype = FORMAT_MPEG2Video;

			MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)pmt->ReallocFormatBuffer(sizeof(MPEG2VIDEOINFO));
			
			vih->cbSequenceHeader = 0;
			vih->dwSequenceHeader[0] = 0;
			vih->dwFlags = 0;
			vih->dwLevel = 0;
			vih->dwProfile = 0;
			vih->dwStartTimeCode = 0;
			
			vih->hdr.bmiHeader.biCompression = '462H';
		}
		else if(iPosition > 2)
		{
			return VFW_S_NO_MORE_ITEMS;
		}
*/
		return S_OK;
	}

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_stream.mt;

	return S_OK;
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1 << 20;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	return NOERROR;
}

DWORD CTSDemuxFilter::CTSDemuxOutputPin::ThreadProc()
{
	m_discontinuity = false;

	bool enabled = false;
	bool discontinuity = false;

	HANDLE handles[] = {m_exit, m_queue.GetEnqueueEvent()};

	while(m_hThread != NULL)
	{
		switch(::WaitForMultipleObjects(countof(handles), handles, FALSE, INFINITE))
		{
		default:

			ASSERT(0);
			return 0;

		case WAIT_OBJECT_0 + 0: 

			return 0;

		case WAIT_OBJECT_0 + 1:

			while(m_queue.GetCount() > 0)
			{
				// if(m_queue.GetCount() > 1) wprintf(L"%d / %d [%08x]\n", m_queue.GetCount(), m_queue.GetMaxCount(), m_mt.subtype.Data1);

				TSDemuxQueueItem* item = m_queue.Dequeue();

				if(!m_flushing)
				{
					if(item->type == TSDemuxQueueItem::Sample)
					{
						BYTE* dst = &item->buff[0];
						/*
						wprintf(L"PES [%d] %d %I64d [%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x .. ]\n",
							m_stream.pid, m_stream.type, item->pts != _I64_MIN ? item->pts / 10000 : 0, item->buff.size(),
							dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7], dst[8], dst[9]);
						*/
						if(enabled && item->pts > -5000000) 
						{
							if(discontinuity)
							{
								item->discontinuity = true;
								discontinuity = false;
							}

							DeliverSample(item);
						}
					}
					else if(item->type == TSDemuxQueueItem::NewSegment)
					{
						enabled = true;
						discontinuity = true;

						__super::DeliverNewSegment(item->pts, _I64_MAX, 1.0f);
					}
					else if(item->type == TSDemuxQueueItem::EndOfStream)
					{
						__super::DeliverEndOfStream();
					}
				}

				delete item;
			}

			break;
		}
	}

	return 0;
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::Active()
{
	m_exit.Reset();

	CAMThread::Create();

	return __super::Active();
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::Inactive()
{
	DeliverBeginFlush();

	m_exit.Set();

	CAMThread::Close();

	DeliverEndFlush();

	m_buff.clear();
	m_size = 0;

	return __super::Inactive();
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::Deliver(TSDemuxQueueItem* item)
{
	if(m_flushing || !IsConnected()) {delete item; return S_FALSE;}

	m_queue.Enqueue(item);

	return S_OK;
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::DeliverNewSegment(REFERENCE_TIME pts)
{
	if(m_flushing || !IsConnected()) return S_FALSE;

	TSDemuxQueueItem* item = new TSDemuxQueueItem();

	item->type = TSDemuxQueueItem::NewSegment;
	item->pts = pts;

	m_queue.Enqueue(item);

	return S_OK;
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::DeliverEndOfStream()
{
	if(m_flushing || !IsConnected()) return S_FALSE;

	TSDemuxQueueItem* item = new TSDemuxQueueItem();

	item->type = TSDemuxQueueItem::EndOfStream;

	m_queue.Enqueue(item);

	return S_OK;
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::DeliverBeginFlush()
{
	if(!IsConnected()) return S_FALSE;

	m_flushing = true;

	return __super::DeliverBeginFlush();
}

HRESULT CTSDemuxFilter::CTSDemuxOutputPin::DeliverEndFlush()
{
	if(!IsConnected()) return S_FALSE;

	m_queue.GetDequeueEvent().Wait();

	m_size = 0;

	m_flushing = false;

	return __super::DeliverEndFlush();
}

void CTSDemuxFilter::CTSDemuxOutputPin::DeliverSample(TSDemuxQueueItem* item)
{
	ASSERT(item->pts != _I64_MIN);

	//

	int limit;
	REFERENCE_TIME pts;

	if(m_size == 0)
	{
		limit = item->buff.size();
		pts = item->pts;
	}
	else
	{
		limit = m_size;
		pts = m_pts;
	}

	// append

	int size = m_size + item->buff.size();

	if(size > m_buff.size())
	{
		m_buff.resize(size);
	}

	memcpy(&m_buff[m_size], &item->buff[0], item->buff.size());

	m_size = size;

	if(item->discontinuity)
	{
		m_discontinuity = true;
	}

	//

	CBaseSplitterFile f(&m_buff[0], m_size);

	int len;
	REFERENCE_TIME dur;

	__int64 next = f.GetPos();

	while(next < limit && NextFrame(f, len, dur) && f.GetPos() < limit)
	{
		next = f.GetPos() + len;

		CComPtr<IMediaSample> sample;

		if(SUCCEEDED(GetDeliveryBuffer(&sample, NULL, NULL, 0)))
		{
			BYTE* dst = NULL;

			if(SUCCEEDED(sample->GetPointer(&dst)))
			{
				if(len > sample->GetSize())
				{
					wprintf(L"!!! buffer size too small !!! %d > %d\n", len, sample->GetSize());

					len = sample->GetSize();
				}

				f.ByteRead(dst, len);

				sample->SetActualDataLength(len);

				sample->SetTime(NULL, NULL);
				sample->SetMediaTime(NULL, NULL);
				sample->SetSyncPoint(FALSE);
				sample->SetPreroll(FALSE);
				sample->SetDiscontinuity(m_discontinuity ? TRUE : FALSE);

				m_discontinuity = false;

				if(pts != _I64_MIN)
				{
					REFERENCE_TIME start = pts;
					REFERENCE_TIME stop = pts + (dur > 0 ? dur : 1);

					sample->SetTime(&start, &stop);
					sample->SetSyncPoint(TRUE);
				}
				/*
				wprintf(L"PES*[%d] %d %I64d [%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x .. ]\n",
					m_stream.pid, m_stream.type, pts != _I64_MIN ? pts / 10000 : 0, len,
					dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7], dst[8], dst[9]);
				*/
				__super::Deliver(sample);
			}
		}

		if(pts != _I64_MIN)
		{
			pts = dur > 0 ? pts + dur : _I64_MIN;
		}

		f.Seek(next);
	}

	f.Seek(next);

	m_size = f.GetRemaining();

	if(m_size > 0)
	{
		memcpy(&m_buff[0], &m_buff[f.GetPos()], m_size);
	}

	m_pts = item->pts;
}

bool CTSDemuxFilter::CTSDemuxOutputPin::NextFrame(CBaseSplitterFile& f, int& len, REFERENCE_TIME& dur)
{
	__int64 start = f.GetPos();

	if(m_mt.subtype == MEDIASUBTYPE_H264 || m_mt.subtype == MEDIASUBTYPE_H264_TRANSPORT)
	{
		BYTE b;
		__int64 at;

		if(f.NextNALU(b, at) && (b & 0x1f) == 9 && at == start)
		{
			while(f.NextNALU(b, at))
			{
				if((b & 0x1f) == 9)
				{
					f.Seek(start);

					len = at - start;
					dur = 1;
	
					return true;
				}
			}
		}
	}
	else if(m_mt.subtype == MEDIASUBTYPE_AAC)
	{
		CBaseSplitterFile::AACHeader h;

		if(f.Read(h, f.GetRemaining()) && h.bytes <= f.GetRemaining())
		{
			len = h.bytes;
			dur = h.rtDuration;

			return true;
		}

		return false;
	}
	else if(m_stream.type == 0x80 && m_mt.subtype == MEDIASUBTYPE_PCM) // 0x80 can also be mpeg2 video (dvb)
	{
		CBaseSplitterFile::BPCMHeader h;

		if(f.Read(h))
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.Format();

			len = f.GetRemaining();
			dur = 10000000i64 * len / wfe->nAvgBytesPerSec;

			// reorder BE to LE

			if(h.bps == 1) // 16 bps
			{
				BYTE* p = &m_buff[(int)f.GetPos()];

				for(int i = 0, j = len; i < j; i += 2)
				{
					// TODO: sse
					BYTE b = p[i];
					p[i] = p[i + 1];
					p[i + 1] = b;
				}
			}
			else
			{
				ASSERT(0); // TODO
			}

			f.ClearCache(); // since we've manipulated the mem-reader's buffer directly

			return true;
		}

		return false;
	}

	f.Seek(start);

	len = (int)f.GetRemaining();
	dur = 0;

	return true;
}

bool TSDemuxStream::DetectMediaType(CBaseSplitterFile& f, int len, BYTE pes_id)
{
	__int64 start = f.GetPos();

	if(pes_id >= 0xe0 && pes_id < 0xf0) // video
	{
		{
			CBaseSplitterFile::MpegSequenceHeader h;

			bool ret = f.Read(h, len, &mt);

			f.Seek(start);

			if(ret) return true;
		}

		{
			CBaseSplitterFile::H264Header h;

			bool ret = f.Read(h, len, &mt);
			
			f.Seek(start);

			if(ret) return true;
		}

		{
			CBaseSplitterFile::H265Header h;

			bool ret = f.Read(h, len, &mt);
			
			f.Seek(start);

			if(ret) return true;
		}
	}
	else if(pes_id >= 0xc0 && pes_id < 0xe0) // audio
	{
		{
			CBaseSplitterFile::LATMHeader h;

			bool ret = f.Read(h, len, &mt);

			f.Seek(start);

			if(ret) return true;
		}

		{
			CBaseSplitterFile::MpegAudioHeader h;

			bool ret = f.Read(h, len, false, &mt);

			f.Seek(start);

			if(ret) return true;
		}

		{
			CBaseSplitterFile::AACHeader h;

			bool ret = f.Read(h, len, &mt);

			f.Seek(start);

			if(ret) return true;
		}
	}
	else if(pes_id == 0xbd || pes_id == 0xfd) // private stream 1
	{
		if(type == 0xea)
		{
			CBaseSplitterFile::VC1Header h;

			bool ret = f.Read(h, len, &mt);

			f.Seek(start);

			if(ret) return true;
		}
		else
		{
			{
				CBaseSplitterFile::AC3Header h;

				bool ret = f.Read(h, len, &mt);

				f.Seek(start);

				if(ret) return true;
			}

			{
				CBaseSplitterFile::DTSHeader h;

				bool ret = f.Read(h, len, &mt);

				f.Seek(start);

				if(ret) return true;
			}
		
			if(type == 0x80)
			{
				CBaseSplitterFile::BPCMHeader h;

				bool ret = f.Read(h, &mt);

				f.Seek(start);

				if(ret) return true;
			}
		}
	}

	return false;
}

size_t TSDemuxProgram::GetScrambledCount() const
{
	return std::count_if(streams.begin(), streams.end(), [] (const std::pair<int, TSDemuxStream*>& p) -> bool 
	{
		return p.second->scrambled;
	});
}

WORD TSDemuxProgram::GetSystemId(int ecm_pid) const
{
	{
		auto i = ecms.find(ecm_pid);

		if(i != ecms.end())
		{
			return i->second;
		}
	}

	for(auto i = streams.begin(); i != streams.end(); i++)
	{
		TSDemuxStream* s = i->second;

		auto j = s->ecms.find(ecm_pid);

		if(j != s->ecms.end())
		{
			return j->second;
		}
	}

	return 0;
}

void TSDemuxPayload::Append(const BYTE* buff, int len)
{
	if(len > 0)
	{
		int size = m_size + len;

		if(size > m_buff.size())
		{
			m_buff.resize(size);
		}

		memcpy(&m_buff[m_size], buff, len);

		m_size = size;
	}
}

//

// CTSDemuxOutputPin

#include "TSWriter.h"

CTSDemuxFilter::CTSDemuxWriterOutputPin::CTSDemuxWriterOutputPin(CTSDemuxFilter* pFilter, HRESULT* phr, LPCWSTR name)
	: CBaseOutputPin(_T("CTSDemuxWriterOutputPin"), pFilter, pFilter, phr, name)
{
	if(phr) *phr = S_OK;
}

HRESULT CTSDemuxFilter::CTSDemuxWriterOutputPin::CheckConnect(IPin* pPin)
{
	CComQIPtr<ITSWriterFilter> pTSW = DirectShow::GetFilter(pPin);

	if(pTSW == NULL)
	{
		return E_FAIL;
	}

	return __super::CheckConnect(pPin);
}

HRESULT CTSDemuxFilter::CTSDemuxWriterOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_MPEG2_TRANSPORT ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTSDemuxFilter::CTSDemuxWriterOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->InitMediaType();

	pmt->majortype = MEDIATYPE_Stream;
	pmt->subtype = MEDIASUBTYPE_MPEG2_TRANSPORT;

	return S_OK;
}

HRESULT CTSDemuxFilter::CTSDemuxWriterOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 2;
	pProperties->cbBuffer = 188;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	return NOERROR;
}
