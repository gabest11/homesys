#include "StdAfx.h"
#include "DSMSink.h"
#include "DirectShow.h"
#include "../Util/String.h"
#include <initguid.h>
#include "moreuuids.h"

// TODO: registry

// #define MAX_PIECE_TIME 3000000000i64
#define MAX_PIECE_TIME 300000000i64
// #define MAX_PIECE_COUNT 20
#define MAX_PIECE_COUNT 200

static int GetByteLength(UINT64 data, int min = 0)
{
	int i = 7;
	while(i >= min && ((BYTE*)&data)[i] == 0) i--;
	return ++i;
}

CDSMSinkFilter::CDSMSinkFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseFilter(NAME("CDSMSinkFilter"), pUnk, this, __uuidof(*this))
	, m_streaming(false)
{
	if(phr) *phr = S_OK;

	AddInput();
}

CDSMSinkFilter::~CDSMSinkFilter()
{
	std::for_each(m_pInputs.begin(), m_pInputs.end(), [] (CDSMSinkInputPin* pin) {delete pin;});
}

STDMETHODIMP CDSMSinkFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		QI(IAMFilterMiscFlags)
		QI(IFileSinkFilter)
		// QI(IMediaSeeking)
		QI(IDSMSinkFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CDSMSinkFilter::AddInput()
{
	auto i = std::find_if(m_pInputs.begin(), m_pInputs.end(), [] (CDSMSinkInputPin* pin) 
	{
		return !pin->IsConnected();
	});

	if(i == m_pInputs.end())
	{
		std::wstring name = Util::Format(L"Input %d", m_pInputs.size() + 1);

		HRESULT hr;

		CDSMSinkInputPin* pPin = new CDSMSinkInputPin(name.c_str(), this, &hr);

		m_pInputs.push_back(pPin);
	}
}

DWORD CDSMSinkFilter::ThreadProc()
{
	if(CDSMSharedData* data = m_mapping.GetData())
	{
		SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

		{
			ATL::CMutexLock lock(m_mapping.GetMutex());

			m_mapping.Reset();
		}

		m_pActivePins.clear();

		for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
		{
			CDSMSinkInputPin* pin = *i;

			if(pin->IsConnected())
			{
				m_pActivePins.push_back(pin);
			}
		}

		while(m_streaming && !m_pActivePins.empty())
		{
			if(m_State == State_Paused) {Sleep(10); continue;}

			CDSMPacket* p = GetPacket();

			if(p == NULL) {Sleep(1); continue;}

			if(p->type == CDSMPacket::Sample)
			{
				WritePacket(p);
			}
			else if(p->type == CDSMPacket::EndOfStream)
			{
				m_pActivePins.remove_if([&p] (CDSMSinkInputPin* pin) {return pin->GetId() == p->id;});
			}

			delete p;
		}

		for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
		{
			CDSMSinkInputPin* pin = *i;

			while(CDSMPacket* p = pin->GetPacket(false))
			{
				delete p;
			}
		}

		Finalize();
		
		m_pActivePins.clear();

		NotifyEvent(EC_COMPLETE, S_OK, (LONG_PTR)(IBaseFilter*)this);
	}

	return 0;
}

CDSMPacket* CDSMSinkFilter::GetPacket()
{
	int n = std::count_if(m_pActivePins.begin(), m_pActivePins.end(), [] (CDSMSinkInputPin* pin) -> bool
	{
		const CMediaType& mt = pin->CurrentMediaType();

		if(mt.majortype == MEDIATYPE_Subtitle || mt.majortype == MEDIATYPE_Text || mt.majortype == MEDIATYPE_VBI)
		{
			return false;
		}

		return !pin->IsFlushing() || pin->HasPacket();
	});

	CDSMSinkInputPin* pmin = NULL;
	REFERENCE_TIME tmin = _I64_MAX;

	for(auto i = m_pActivePins.begin(); i != m_pActivePins.end(); i++)
	{
		if(CDSMPacket* p = (*i)->GetPacket(true))
		{
			if(p->type == CDSMPacket::Sample && (p->flags & (CDSMPacket::Bogus | CDSMPacket::TimeInvalid)) != 0
			|| p->type == CDSMPacket::EndOfStream)
			{
				pmin = *i;
				n = 0;
				break;
			}
			else if(p->start < tmin)
			{
				pmin = *i;
				tmin = p->start;
			}

			n--;
		}
	}

	return pmin != NULL && n <= 0 ? pmin->GetPacket(false) : NULL;
}

void CDSMSinkFilter::WritePacket(CDSMPacket* p)
{
	if(CDSMSharedData* data = m_mapping.GetData())
	{
		REFERENCE_TIME stop = data->stop;

		if((p->flags & CDSMPacket::TimeInvalid) == 0)
		{
			stop = p->start;
		}

		int next = data->next;

		while(stop >= data->piece * next)
		{
			m_file[0].Create(m_mapping.GetName(next++), m_pInputs);
		}

		m_file[0].Write(p);

		DWORD index = data->index;

		if(p->flags & CDSMPacket::SyncPoint)
		{
			CDSMIndexData i;
			
			i.piece = data->next - 1;
			i.offset = (DWORD)m_file[0].GetWritePos();
			i.start = p->start;

			if(m_mapping.WriteIndex(p->id, i))
			{
				index++;
			}
		}

		ATL::CMutexLock lock(m_mapping.GetMutex());

		m_mapping.DeleteOld(MAX_PIECE_COUNT);
		
		data->stop = stop;
		data->next = next;
		data->size = m_file[0].GetPos();
		data->index = index;
	}

	{
		CAutoLock cAutoLock(&m_csWrite);

		m_file[1].Write(p);
	}

}

void CDSMSinkFilter::Finalize()
{
	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());

		m_file[0].Finalize();

		m_mapping.DeleteOld(0);

		data->next = -1;
	}

	{
		CAutoLock cAutoLock(&m_csWrite);

		m_file[1].Finalize();
	}
}

//

int CDSMSinkFilter::GetPinCount()
{
	return m_pInputs.size();
}

CBasePin* CDSMSinkFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(n >= 0 && n < (int)m_pInputs.size())
	{
		return m_pInputs[n];
	}

	return NULL;
}

STDMETHODIMP CDSMSinkFilter::Stop()
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		m_streaming = false;

		CAMThread::Close();

		m_mapping.Close();
	}

	return __super::Stop();
}

STDMETHODIMP CDSMSinkFilter::Pause()
{
	CAutoLock cAutoLock(this);

	if(m_State == State_Stopped)
	{
		m_mapping.Create(m_filename.c_str(), m_pInputs);

		m_streaming = true;

		CAMThread::Create();
	}

	return __super::Pause();
}

// IFileSinkFilter

STDMETHODIMP CDSMSinkFilter::SetFileName(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		return VFW_E_WRONG_STATE;
	}

	m_filename.clear();

	HRESULT hr;

	hr = m_mapping.Create(pszFileName, m_pInputs) ? S_OK : E_FAIL;

	m_mapping.Close();

	if(SUCCEEDED(hr))
	{
		m_filename = pszFileName;
	}

	return hr;
}

STDMETHODIMP CDSMSinkFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_filename.size() + 1) * sizeof(WCHAR));
	
	if(*ppszFileName == NULL)
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_filename.c_str());

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CDSMSinkFilter::GetCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL) return E_POINTER;

	*pCapabilities = AM_SEEKING_CanGetDuration | AM_SEEKING_CanGetCurrentPos;

	return S_OK;
}

STDMETHODIMP CDSMSinkFilter::CheckCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL) return E_POINTER;

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;

	GetCapabilities(&caps);

	caps &= *pCapabilities;

	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CDSMSinkFilter::IsFormatSupported(const GUID* pFormat) 
{
	if(pFormat == NULL) return E_POINTER;

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CDSMSinkFilter::QueryPreferredFormat(GUID* pFormat) 
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CDSMSinkFilter::GetTimeFormat(GUID* pFormat) 
{
	if(pFormat == NULL) return E_POINTER;

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CDSMSinkFilter::IsUsingTimeFormat(const GUID* pFormat) 
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CDSMSinkFilter::SetTimeFormat(const GUID* pFormat) 
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDSMSinkFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	
	*pDuration = 0; // TODO

	/*
	POSITION pos = m_pInputs.GetHeadPosition();
	
	while(pos) 
	{
		REFERENCE_TIME rt = m_pInputs.GetNext(pos)->GetDuration(); 
		
		if(rt > *pDuration) *pDuration = rt;
	}
	*/
	
	return S_OK;
}

STDMETHODIMP CDSMSinkFilter::GetStopPosition(LONGLONG* pStop) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMSinkFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	if(pCurrent == NULL) return E_POINTER;
	
	*pCurrent = 0; // TODO: m_current;

	return S_OK;
}

STDMETHODIMP CDSMSinkFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMSinkFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if(m_State == State_Stopped)
	{
		std::for_each(m_pInputs.begin(), m_pInputs.end(), [&] (CDSMSinkInputPin* pin)
		{
			CComQIPtr<IMediaSeeking> pMS = pin->GetConnected();

			if(pMS == NULL) 
			{
				pMS = DirectShow::GetFilter(pin->GetConnected());
			}

			if(pMS != NULL)
			{
				pMS->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
			}
		});

		return S_OK;
	}

	return VFW_E_WRONG_STATE;
}

STDMETHODIMP CDSMSinkFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMSinkFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMSinkFilter::SetRate(double dRate) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMSinkFilter::GetRate(double* pdRate) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMSinkFilter::GetPreroll(LONGLONG* pllPreroll) 
{
	return E_NOTIMPL;
}

// IDSMSinkFilter

STDMETHODIMP CDSMSinkFilter::StartRecording(LPCWSTR path)
{
	CAutoLock cAutoLock(&m_csWrite);

	m_file[1].Finalize();

	std::vector<CDSMSinkInputPin*> pins;

	for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
	{
		const CMediaType& mt = (*i)->CurrentMediaType();

		if(mt.majortype == MEDIATYPE_VBI) continue;

		pins.push_back(*i);
	}

	return m_file[1].Create(path, pins) ? S_OK : E_FAIL;
}

STDMETHODIMP CDSMSinkFilter::StopRecording()
{
	CAutoLock cAutoLock(&m_csWrite);

	m_file[1].Finalize();

	return S_OK;
}

// CDSMSinkInputPin

CDSMSinkInputPin::CDSMSinkInputPin(LPCWSTR pName, CDSMSinkFilter* pFilter, HRESULT* phr)
	: CBaseInputPin(NAME("CDSMSinkInputPin"), pFilter, pFilter, phr, pName)
	, m_queue(1000)
{
	static BYTE s_id = 0;

	m_id = s_id++;

	m_packet.in = NULL;
	m_packet.out = NULL;
}

CDSMSinkInputPin::~CDSMSinkInputPin()
{
	ASSERT(m_queue.GetCount() == 0);

	FreePackets();
}

void CDSMSinkInputPin::FreePackets()
{
	CAutoLock cAutoLock(&m_csPacket);

	while(m_queue.GetCount() > 0)
	{
		delete m_queue.Dequeue();
	}

	delete m_packet.in;
	delete m_packet.out;

	m_packet.in = NULL;
	m_packet.out = NULL;
}

STDMETHODIMP CDSMSinkInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDSMSinkInputPin::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->formattype == FORMAT_WaveFormatEx)
	{
		WORD wFormatTag = ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag;

		if((wFormatTag == WAVE_FORMAT_PCM || wFormatTag == WAVE_FORMAT_EXTENSIBLE || wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		&& pmt->subtype != FOURCCMap(wFormatTag)
		&& !(pmt->subtype == MEDIASUBTYPE_PCM && wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		&& !(pmt->subtype == MEDIASUBTYPE_PCM && wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		&& pmt->subtype != MEDIASUBTYPE_DVD_LPCM_AUDIO
		&& pmt->subtype != MEDIASUBTYPE_DOLBY_AC3
		&& pmt->subtype != MEDIASUBTYPE_DTS)
		{
			return E_INVALIDARG;
		}
	}

	return pmt->majortype == MEDIATYPE_Video
		|| pmt->majortype == MEDIATYPE_Audio && pmt->formattype != FORMAT_VorbisFormat
		|| pmt->majortype == MEDIATYPE_Text && pmt->subtype == MEDIASUBTYPE_NULL && pmt->formattype == FORMAT_None
		|| pmt->majortype == MEDIATYPE_Subtitle
		|| pmt->majortype == MEDIATYPE_VBI && pmt->subtype == MEDIASUBTYPE_TELETEXT
		? S_OK
		: E_INVALIDARG;
}

HRESULT CDSMSinkInputPin::BreakConnect()
{
	return __super::BreakConnect();
}

HRESULT CDSMSinkInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr;

	hr = __super::CompleteConnect(pReceivePin);

	if(SUCCEEDED(hr))
	{
		((CDSMSinkFilter*)m_pFilter)->AddInput();
	}

	return hr;
}

HRESULT CDSMSinkInputPin::Active()
{
	m_current = _I64_MIN;

	return __super::Active();
}

HRESULT CDSMSinkInputPin::Inactive()
{
	FreePackets();

	return __super::Inactive();
}

STDMETHODIMP CDSMSinkInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	if(m_bFlushing) return S_OK;

	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CDSMSinkInputPin::Receive(IMediaSample* pSample)
{
	CAutoLock cAutoLock(&m_csReceive);

	if(m_bFlushing) return S_OK;

	HRESULT hr = __super::Receive(pSample);

	if(FAILED(hr)) return hr;

	long len = pSample->GetActualDataLength();

	BYTE* ptr = NULL;

	if(FAILED(pSample->GetPointer(&ptr)) || !ptr)
	{
		return S_OK;
	}

	CDSMPacket* p = new CDSMPacket();

	p->id = m_id;

	p->buff.resize(len);

	memcpy(&p->buff[0], ptr, len);

	p->flags |= CDSMPacket::TimeInvalid;

	if(S_OK == pSample->IsSyncPoint() || m_mt.majortype == MEDIATYPE_Audio && !m_mt.bTemporalCompression)
	{
		p->flags |= CDSMPacket::SyncPoint;
	}

	if(S_OK == pSample->GetTime(&p->start, &p->stop))
	{
		p->flags &= ~CDSMPacket::TimeInvalid;

		p->start += m_tStart; 
		p->stop += m_tStart;

		if((p->flags & CDSMPacket::SyncPoint) && p->start < m_current)
		{
			p->flags &= ~CDSMPacket::SyncPoint;
			p->flags |= CDSMPacket::Bogus;
		}

		m_current = std::max<REFERENCE_TIME>(m_current, p->start);
	}
	else 
	{
		if(p->flags & CDSMPacket::SyncPoint)
		{
			p->flags &= ~CDSMPacket::SyncPoint;
			p->flags |= CDSMPacket::Bogus;
		}

		// TODO: accumlate mpeg video packets
	}

	if(S_OK == pSample->IsDiscontinuity())
	{
		p->flags |= CDSMPacket::Discontinuity;
	}

	if(m_mt.majortype == MEDIATYPE_Video && m_mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO) // MEDIASUBTYPE_MPEG1Video ?
	{
		CAutoLock cAutoLock(&m_csPacket);

		if(m_packet.in == NULL)
		{
			if((p->flags & CDSMPacket::TimeInvalid) == 0)
			{
				m_packet.in = p;

				return S_OK;
			}
		}
		else
		{
			ASSERT(m_packet.in != NULL);

			if((p->flags & CDSMPacket::TimeInvalid) != 0)
			{
				m_packet.in->buff.insert(m_packet.in->buff.end(), p->buff.begin(), p->buff.end()); // resize + memcpy?

				delete p;

				return S_OK;
			}
			else
			{
				CDSMPacket* tmp = p;

				p = m_packet.in;

				m_packet.in = tmp;
			}
		}
	}

	// printf("%08x q count=%d p size=%d start=%I64d flags=%02x\n", m_mt.majortype.Data1, m_queue.GetCount(), p->buff.size(), p->start / 10000, p->flags);

	m_queue.Enqueue(p);

	return S_OK;
}

STDMETHODIMP CDSMSinkInputPin::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	if(m_bFlushing) return S_OK;

	CDSMPacket* p = new CDSMPacket();

	p->id = m_id;

	p->type = CDSMPacket::EndOfStream;

	m_queue.Enqueue(p);

	return __super::EndOfStream();
}

STDMETHODIMP CDSMSinkInputPin::BeginFlush()
{
	return __super::BeginFlush();
}

STDMETHODIMP CDSMSinkInputPin::EndFlush()
{
	FreePackets();

	return __super::EndFlush();
}

CDSMPacket* CDSMSinkInputPin::GetPacket(bool peek)
{
	if(m_bFlushing) return NULL;

	if(m_packet.out == NULL)
	{
		if(m_queue.GetCount() > 0)
		{
			m_packet.out = m_queue.Dequeue();
		}
	}

	CDSMPacket* packet = m_packet.out;

	if(!peek)
	{
		m_packet.out = NULL;
	}

	return packet;
}

// CDSMFile

CDSMFile::CDSMFile()
	: m_file(INVALID_HANDLE_VALUE)
	, m_size(0)
{
	m_byte.limit = 2048;
	m_byte.buff = new BYTE[m_byte.limit];

	m_byte.len = 0;
	m_bit.len = 0;

	m_mode = None;
}

CDSMFile::~CDSMFile()
{
	Close();

	delete m_byte.buff;
}

bool CDSMFile::Create(const std::wstring& path, const std::vector<CDSMSinkInputPin*>& pins)
{
	Finalize();

	m_path = path;
	m_file = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	m_byte.len = 0;
	m_bit.len = 0;
	
	m_mode = ModeWrite;

	WritePacketHeader(DSMP_FILEINFO, 1);
	
	BitWrite(DSMF_VERSION, 8);

	std::for_each(pins.begin(), pins.end(), [this] (CDSMSinkInputPin* pin)
	{
		if(pin->IsConnected())
		{
			const CMediaType& mt = pin->CurrentMediaType();

			int id = pin->GetId();

			WritePacketHeader(DSMP_MEDIATYPE, 5 + sizeof(GUID) * 3 + mt.FormatLength());
			
			BitWrite(id, 8);
			ByteWrite(&mt.majortype, sizeof(mt.majortype));
			ByteWrite(&mt.subtype, sizeof(mt.subtype));
			BitWrite(mt.bFixedSizeSamples, 1);
			BitWrite(mt.bTemporalCompression, 1);
			BitWrite(mt.lSampleSize, 30);
			ByteWrite(&mt.formattype, sizeof(mt.formattype));
			ByteWrite(mt.Format(), mt.FormatLength());

			/*
			PacketHeader(DSMP_STREAMINFO, 1);
			
			BitWrite(id, 8);
			*/
		}
	});

	Flush();

	return true;
}

void CDSMFile::Write(CDSMPacket* p)
{
	if(m_file == INVALID_HANDLE_VALUE)
	{
		return;
	}

	REFERENCE_TIME pts = 0;
	REFERENCE_TIME dur = 0;

	int pts_len = 0;
	int dur_len = 0;

	if((p->flags & CDSMPacket::TimeInvalid) == 0)
	{
		pts = p->start;
		dur = p->stop - p->start;

		if(dur < 0) dur = 0;

		pts_len = GetByteLength(pts < 0 ? -pts : pts);
		dur_len = GetByteLength(dur);

		// TODO: IndexSyncPoint(p, GetPos());
	}

	int len = 2 + pts_len + dur_len + p->buff.size();

	WritePacketHeader(DSMP_SAMPLE, len);

	BitWrite(p->id, 8);
	BitWrite((p->flags & CDSMPacket::SyncPoint) ? 1 : 0, 1);
	BitWrite(pts < 0, 1);
	BitWrite(pts_len, 3);
	BitWrite(dur_len, 3);
	BitWrite(pts < 0 ? -pts : pts, pts_len << 3);
	BitWrite(dur, dur_len << 3);
	ByteWrite(&p->buff[0], p->buff.size());

	Flush();
}

void CDSMFile::Finalize()
{
	if(m_file != INVALID_HANDLE_VALUE)
	{
		// TODO

		Flush();
	}

	Close();
}

bool CDSMFile::Open(const std::wstring& path)
{
	m_mode = ModeRead;

	m_path = path;
	m_file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);

	if(m_file == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();

		return false;
	}

	dsmp_t type;
	UINT64 len;

	if(!ReadPacketHeader(type, len) || type != DSMP_FILEINFO)
	{
		return false;
	}

	if(BitRead(8) > DSMF_VERSION)
	{
		return false;
	}

	return true;
}

void CDSMFile::Close()
{
	m_path.clear();

	if(m_file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_file);

		m_file = INVALID_HANDLE_VALUE;
	}

	m_size = 0;
}

void CDSMFile::WritePacketHeader(dsmp_t type, UINT64 len)
{
	ASSERT(type < 32);

	int i = GetByteLength(len, 1);

	BitWrite(DSMSW, DSMSW_SIZE << 3);
	BitWrite(type, 5);
	BitWrite(i - 1, 3);
	BitWrite(len, i << 3);
}

bool CDSMFile::ReadPacketHeader(dsmp_t& type, UINT64& len)
{
	if(BitRead(32) != DSMSW)
	{
		return false;
	}

	type = (dsmp_t)BitRead(5);

	len = BitRead(((int)BitRead(3) + 1) << 3);

	return true;
}

void CDSMFile::ByteWrite(const void* data, int len)
{
	ASSERT(m_mode == ModeWrite);

	BitFlush();

	Write(data, len);
}

void CDSMFile::BitWrite(UINT64 data, int len)
{
	ASSERT(m_mode == ModeWrite);

	ASSERT(len >= 0 && len <= 64);

	if(len > 56) 
	{
		BitWrite(data >> 56, len - 56); 
		
		len = 56;
	}

	m_bit.buff <<= len;
	m_bit.buff |= data & ((1ui64 << len) - 1);
	m_bit.len += len;
	
	while(m_bit.len >= 8)
	{
		BYTE b = (BYTE)(m_bit.buff >> (m_bit.len - 8));

		Write(&b, 1);

		m_bit.len -= 8;
	}
}

void CDSMFile::StrWrite(LPCSTR data, bool bFixNewLine)
{
	ASSERT(m_mode == ModeWrite);

	std::string s = data;

	if(bFixNewLine)
	{
		Util::Replace(s, "\r", "");
		Util::Replace(s, "\n", "\r\n");
	}

	return ByteWrite(s.c_str(), s.size());
}

UINT64 CDSMFile::BitRead(int len)
{
	ASSERT(len >= 0 && len <= 64);

	while(m_bit.len < len)
	{
		BYTE b;

		Read(&b, 1);

		m_bit.buff = (m_bit.buff << 8) | b;
		m_bit.len += 8;
	}

	m_bit.len -= len;

	UINT64 ret = (m_bit.buff >> m_bit.len) & ((1ui64 << len) - 1);

	m_bit.buff &= ((1ui64 << m_bit.len) - 1);

	return ret;
}

void CDSMFile::ByteRead(void* data, int len)
{
	BitFlush();

	Read(data, len);
}

void CDSMFile::BitFlush()
{
	if(m_mode == ModeWrite)
	{
		if(m_bit.len > 0)
		{
			ASSERT(m_bit.len < 8);

			BYTE b = (BYTE)(m_bit.buff << (8 - m_bit.len));

			Write(&b, 1);

			m_bit.len = 0;
		}
	}
	else if(m_mode == ModeRead)
	{
		m_bit.len = 0;
	}
}

UINT64 CDSMFile::GetPos()
{
	LARGE_INTEGER pos;
	
	pos.QuadPart = 0;
	
	pos.LowPart = SetFilePointer(m_file, pos.LowPart, &pos.HighPart, FILE_CURRENT);

	return pos.QuadPart;
}

UINT64 CDSMFile::GetLength()
{
	LARGE_INTEGER size;
	
	size.QuadPart = 0;
	
	GetFileSizeEx(m_file, &size);

	return size.QuadPart;
}

UINT64 CDSMFile::GetWritePos()
{
	return m_size;
}

void CDSMFile::SetReadPos(UINT64 pos)
{
	BitFlush();

	LARGE_INTEGER li1;
	LARGE_INTEGER li2;

	li1.QuadPart = pos;
	li2.QuadPart = 0;

	SetFilePointerEx(m_file, li1, &li2, FILE_BEGIN);
}

void CDSMFile::Write(const void* data, int len)
{
	ASSERT(m_mode == ModeWrite);

	if(m_byte.len + len >= m_byte.limit)
	{
		DWORD written;

		WriteFile(m_file, m_byte.buff, m_byte.len, &written, NULL);
		WriteFile(m_file, data, len, &written, NULL);

		m_byte.len = 0;
	}
	else
	{
		if(len == 1)
		{
			m_byte.buff[m_byte.len++] = ((BYTE*)data)[0];
		}
		else
		{
			memcpy(&m_byte.buff[m_byte.len], data, len);

			m_byte.len += len;
		}
	}
		
	m_size += len;
}

void CDSMFile::Read(void* data, int len)
{
	// TODO: m_byte.buff

	DWORD read;

	ReadFile(m_file, data, len, &read, NULL);
}

void CDSMFile::Flush()
{
	if(m_mode == ModeWrite)
	{
		if(m_byte.len > 0)
		{
			DWORD written;

			WriteFile(m_file, m_byte.buff, m_byte.len, &written, NULL);

			m_byte.len = 0;
		}
	}
}


// CDSMFileMapping

CDSMFileMapping::CDSMFileMapping()
{
	m_readonly = true;
	m_file = INVALID_HANDLE_VALUE;
	m_index.file = INVALID_HANDLE_VALUE;
	m_index.piece = 0;
	m_index.stop = _I64_MIN;
	m_index.id = 0xffffffff;
	m_mapping = INVALID_HANDLE_VALUE;
	m_data = NULL;

	m_acl = BuildRestrictedSD(&m_sd);

	m_sa.nLength = sizeof(m_sa);
	m_sa.lpSecurityDescriptor = &m_sd;
	m_sa.bInheritHandle = FALSE;
}

CDSMFileMapping::~CDSMFileMapping()
{
	Close();

	delete [] m_acl;
}

ACL* CDSMFileMapping::BuildRestrictedSD(SECURITY_DESCRIPTOR* sd)
{
	if(InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
	{
		SID_IDENTIFIER_AUTHORITY siaNT = SECURITY_WORLD_SID_AUTHORITY; //SECURITY_NT_AUTHORITY;
		
		void* sid = NULL;

		if(AllocateAndInitializeSid(&siaNT, 1, SECURITY_WORLD_RID/*SECURITY_AUTHENTICATED_USER_RID*/, 0, 0, 0, 0, 0, 0, 0, &sid))
		{
			size_t len = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(sid);

			ACL* acl = (ACL*)new BYTE[len];

			memset(acl, 0, len);

			if(InitializeAcl(acl, len, ACL_REVISION))
			{
				if(AddAccessAllowedAce(acl, ACL_REVISION, GENERIC_ALL, sid))
				{
					if(SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
					{
						return acl;
					}
				}
			}

			delete [] (BYTE*)acl;
		}

		FreeSid(sid);
	}

	return NULL;
}

bool CDSMFileMapping::Create(const std::wstring& path, const std::vector<CDSMSinkInputPin*>& pins)
{
	Close();

	SECURITY_ATTRIBUTES* sa = &m_sa;

	m_file = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, sa, CREATE_ALWAYS, 0, 0);

	if(m_file == INVALID_HANDLE_VALUE)
	{
		wprintf(L"CDSMFileMapping::Create, error %d, %s\n", GetLastError(), path.c_str());

		Close();

		return false;
	}

	m_index.file = CreateFile((path + L":index").c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	if(m_index.file == INVALID_HANDLE_VALUE)
	{
		wprintf(L"CDSMFileMapping::Create, index error %d, %s\n", GetLastError(), (path + L":index").c_str());

		Close();

		return false;
	}

	m_name = path;

	m_readonly = false;

	std::wstring str = L"Global\\DSMSink$" + path.substr(path.find_last_of('\\') + 1);

	CDSMSharedData data;

	data.sync = (DWORD)~DSMSW;
	data.streams = 0;
	data.first = 0;
	data.next = 0;
	data.piece = MAX_PIECE_TIME;
	data.start = 0;
	data.stop = 0;

	DWORD written;

	WriteFile(m_file, &data, sizeof(data), &written, NULL);

	for(auto i = pins.begin(); i != pins.end(); i++)
	{
		CDSMSinkInputPin* pin = *i;

		if(pin->IsConnected())
		{
			data.streams++;

			DWORD written;

			BYTE id = pin->GetId();

			WriteFile(m_file, &id, sizeof(id), &written, NULL);

			const CMediaType& mt = pin->CurrentMediaType();

			WriteFile(m_file, &mt.majortype, sizeof(mt.majortype), &written, NULL);
			WriteFile(m_file, &mt.subtype, sizeof(mt.subtype), &written, NULL);
			WriteFile(m_file, &mt.bFixedSizeSamples, sizeof(mt.bFixedSizeSamples), &written, NULL);
			WriteFile(m_file, &mt.bTemporalCompression, sizeof(mt.bTemporalCompression), &written, NULL);
			WriteFile(m_file, &mt.lSampleSize, sizeof(mt.lSampleSize), &written, NULL);
			WriteFile(m_file, &mt.formattype, sizeof(mt.formattype), &written, NULL);
			WriteFile(m_file, &mt.cbFormat, sizeof(mt.cbFormat), &written, NULL);
			WriteFile(m_file, mt.pbFormat, mt.cbFormat, &written, NULL);

			if(m_index.id == 0xffffffff || mt.majortype == MEDIATYPE_Video)
			{
				m_index.id = id;
			}
		}
	}

	if(!m_mutex.Create(sa, FALSE, str.c_str()))
	{
		wprintf(L"CDSMFileMapping::Create, m_mutex.Create, error %d, %s\n", GetLastError(), str.c_str());

		Close();

		return false;
	}

	m_mapping = CreateFileMapping(m_file, NULL, PAGE_READWRITE, 0, 0, NULL);

	if(m_mapping == NULL)
	{
		wprintf(L"CDSMFileMapping::Create, CreateFileMapping, error %d\n", GetLastError());

		Close();

		return false;
	}

	m_data = (CDSMSharedData*)MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if(m_data == NULL)
	{
		wprintf(L"CDSMFileMapping::Create, MapViewOfFile, error %d\n", GetLastError());

		Close();

		return false;
	}

	*m_data = data;

	return true;
}

bool CDSMFileMapping::Open(const std::wstring& path, std::unordered_map<int, CMediaType>& mts)
{
	Close();

	m_file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);

	if(m_file == INVALID_HANDLE_VALUE)
	{
		Close();

		return false;
	}

	m_index.file = CreateFile((path + L":index").c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);

	if(m_index.file == INVALID_HANDLE_VALUE)
	{
		Close();

		return false;
	}

	m_name = path;

	m_readonly = true;

	std::wstring str = L"Global\\DSMSink$" + path.substr(path.find_last_of('\\') + 1);

	CDSMSharedData data;

	DWORD read;

	ReadFile(m_file, &data, sizeof(data), &read, NULL);

	if(data.sync != (DWORD)~DSMSW)
	{
		Close();

		return false;
	}

	for(int i = 0; i < data.streams; i++)
	{
		DWORD read;
		DWORD cbFormat = 0;

		BYTE id = 0;

		ReadFile(m_file, &id, sizeof(id), &read, NULL);

		CMediaType mt;

		ReadFile(m_file, &mt.majortype, sizeof(mt.majortype), &read, NULL);
		ReadFile(m_file, &mt.subtype, sizeof(mt.subtype), &read, NULL);
		ReadFile(m_file, &mt.bFixedSizeSamples, sizeof(mt.bFixedSizeSamples), &read, NULL);
		ReadFile(m_file, &mt.bTemporalCompression, sizeof(mt.bTemporalCompression), &read, NULL);
		ReadFile(m_file, &mt.lSampleSize, sizeof(mt.lSampleSize), &read, NULL);
		ReadFile(m_file, &mt.formattype, sizeof(mt.formattype), &read, NULL);
		ReadFile(m_file, &cbFormat, sizeof(cbFormat), &read, NULL);

		if(cbFormat > 0)
		{
			ReadFile(m_file, mt.AllocFormatBuffer(cbFormat), cbFormat, &read, NULL);
		}

		mts[id] = mt;
	}

	if(!m_mutex.Create(NULL, FALSE, str.c_str()) || data.sync != (DWORD)~DSMSW)
	{
		DWORD error = GetLastError();

		Close();

		return false;
	}

	m_mapping = CreateFileMapping(m_file, NULL, PAGE_READONLY, 0, 0, NULL);

	if(m_mapping == NULL)
	{
		Close();

		return false;
	}

	m_data = (CDSMSharedData*)MapViewOfFile(m_mapping, FILE_MAP_READ, 0, 0, 0);

	if(m_data == NULL)
	{
		Close();

		return false;
	}

	return true;
}

void CDSMFileMapping::Close()
{
	m_mutex.Close();

	if(m_data != NULL && !m_readonly)
	{
		m_data->next = -1;
	}

	if(m_mapping != INVALID_HANDLE_VALUE)
	{
		UnmapViewOfFile(m_data);

		m_data = NULL;

		CloseHandle(m_mapping);

		m_mapping = INVALID_HANDLE_VALUE;
	}

	if(m_index.file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_index.file);

		m_index.file = INVALID_HANDLE_VALUE;

		m_index.id = 0xffffffff;
	}

	if(m_file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_file);

		m_file = INVALID_HANDLE_VALUE;
	}

	if(!m_name.empty())
	{
		if(!m_readonly)
		{
			LPCWSTR name = m_name.c_str();

			DeleteFile(name);

			for(clock_t t = clock(); clock() - t < 3000; )
			{
				HANDLE h = CreateFile(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);

				if(h != INVALID_HANDLE_VALUE)
				{
					CloseHandle(h);

					DeleteFile(name);

					break;
				}

				Sleep(50);
			}
		}

		m_name.clear();
	}
}

void CDSMFileMapping::Reset()
{	
	m_data->first = 0;
	m_data->next = 0;
	m_data->start = 0;
	m_data->stop = 0;
	m_data->size = 0;
	m_data->index = 0;

	if(m_index.file != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(m_index.file, 0, NULL, FILE_BEGIN);
		SetEndOfFile(m_index.file);
		
		m_index.piece = 0;
		m_index.stop = _I64_MIN;
	}
}

void CDSMFileMapping::DeleteOld(int keep)
{
	// TODO: delete more pieces if low on space

	while(m_data->next - m_data->first > keep)
	{
		DeleteFile(GetName(m_data->first).c_str());

		m_data->first++;
		m_data->start += m_data->piece;

		// TODO: clean up front elements from index and enable WriteIndex
	}
}

bool CDSMFileMapping::WriteIndex(BYTE id, const CDSMIndexData& data)
{
	if(0)
	if(id == m_index.id && (m_index.piece < data.piece || m_index.stop < data.start))
	{
		DWORD written;

		WriteFile(m_index.file, &data, sizeof(data), &written, NULL);

		m_index.piece = data.piece;
		m_index.stop = data.start + 10000000; // 1 sec gap

		return true;
	}

	return false;
}

bool CDSMFileMapping::ReadIndex(REFERENCE_TIME rt, CDSMIndexData& data)
{
	if(m_data->index == 0)
	{
		return false;
	}

	SetFilePointer(m_index.file, 0, NULL, FILE_BEGIN);

	std::vector<CDSMIndexData> buff(m_data->index);

	DWORD read = 0;
	DWORD size = buff.size() * sizeof(CDSMIndexData);

	ReadFile(m_index.file, buff.data(), size, &read, NULL);

	if(size != read)
	{
		return false;
	}

	for(int i = buff.size() - 1; i > 0; i--)
	{
		if(buff[i].start <= rt)
		{
			data = buff[i];

			return true;
		}
	}

	return false;
}

std::wstring CDSMFileMapping::GetName(int n)
{
	return n < 0 ? m_name : Util::Format(L"%s:%d.dsm", m_name.c_str(), n);
}
