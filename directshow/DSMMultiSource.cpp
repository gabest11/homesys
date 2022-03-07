#include "StdAfx.h"
#include "DSMMultiSource.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// CDSMMultiSourceFilter

CDSMMultiSourceFilter::CDSMMultiSourceFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CDSMMultiSourceFilter"), lpunk, this, __uuidof(this))
	, m_exit(TRUE)
	, m_starvation(false)
	, m_progress(0)
{
	if(phr) *phr = S_OK;
}

CDSMMultiSourceFilter::~CDSMMultiSourceFilter()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}
}

STDMETHODIMP CDSMMultiSourceFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMediaSeeking)
		QI(IFileSourceFilter)
		QI(IAMOpenProgress)
		QI(IDSMMultiSourceFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

int CDSMMultiSourceFilter::GetPinCount()
{
	return m_outputs.size();
}

CBasePin* CDSMMultiSourceFilter::GetPin(int n)
{
	if(n >= 0 && n < m_outputs.size())
	{
		return m_outputs[n];
	}

	return NULL;
}

STDMETHODIMP CDSMMultiSourceFilter::Pause()
{
	CAutoLock cAutoLock(this);

	if(m_State == State_Stopped)
	{
		std::unordered_map<int, CMediaType> mts;

		if(!m_mapping.Open(m_path.c_str(), mts))
		{
			return E_FAIL;
		}

		if(m_mapping.GetData() == NULL)
		{
			m_mapping.Close();

			return E_FAIL;
		}

		memset(&m_time, 0, sizeof(m_time));

		m_exit.Reset();

		CAMThread::Create();
	}

	return __super::Pause();
}

STDMETHODIMP CDSMMultiSourceFilter::Stop()
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		m_exit.Set();

		DeliverBeginFlush();

		CAMThread::Close();

		DeliverEndFlush();
	}

	return __super::Stop();
}

void CDSMMultiSourceFilter::DeliverBeginFlush()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		(*i)->DeliverBeginFlush();
	}
}

void CDSMMultiSourceFilter::DeliverEndFlush()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		(*i)->DeliverEndFlush();
	}
}

void CDSMMultiSourceFilter::DeliverNewSegment()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		(*i)->DeliverNewSegment(m_time.start, _I64_MAX);
	}
}

void CDSMMultiSourceFilter::DeliverEndOfStream()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		(*i)->DeliverEndOfStream();
	}
}

bool CDSMMultiSourceFilter::Seek(REFERENCE_TIME rt)
{
	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());

		m_file.Close();

		{
			CDSMIndexData i;

			if(m_mapping.ReadIndex(rt, i))
			{
				m_index = i.piece;

				m_time.start = rt;
				m_time.offset = 0;
				m_time.filter = rt;

				if(m_file.Open(m_mapping.GetName(m_index)))
				{
					m_file.SetReadPos(i.offset);

					return true;
				}
			}
		}

		if(rt < data->start) rt = data->start;
		if(rt > data->stop) rt = data->stop;

		m_index = (int)(rt / data->piece);

		m_time.start = rt;
		m_time.offset = 0;

		if(m_index < data->first)
		{
			m_index = data->first;
			m_time.start = data->piece * data->first;
		}
		else if(m_index >= data->next)
		{
			m_index = data->next - 1;
			m_time.start = data->stop;
		}

		m_time.filter = m_time.start;

		if(m_index < data->first || m_index >= data->next || m_index < 0)
		{
			return false;
		}

		if(!m_file.Open(m_mapping.GetName(m_index)))
		{
			return false;
		}
	}

	// TODO: leave file pos at rt

	return true;
}

bool CDSMMultiSourceFilter::SeekNext()
{
	REFERENCE_TIME start = _I64_MIN;

	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());

		m_file.Close();

		int index = m_index + 1;

		if(index >= data->next)
		{
			return false;
		}

		if(index < data->first)
		{
			index = data->first;
			start = data->first * data->piece;
		}

		if(!m_file.Open(m_mapping.GetName(index)))
		{
			return false;
		}

		m_index = index;
	}

	if(start != _I64_MIN)
	{
		m_time.offset += start - m_time.filter;
	}

	return true;
}

DWORD CDSMMultiSourceFilter::ThreadProc()
{
	Seek(_I64_MAX); // TODO: resume from last stop position

	DWORD delay = 0;

	while(!m_exit.Check())
	{
		if(CheckRequest(NULL))
		{
			GetRequest();

			Seek(m_time.newstart);

			DeliverEndFlush();

			DeliverNewSegment();

			Reply(S_OK);
		}

		if(delay > 0)
		{
			Sleep(delay);

			delay = 0;
		}

		if(CDSMSharedData* data = m_mapping.GetData())
		{
			ATL::CMutexLock lock(m_mapping.GetMutex());

			if(data->next < 0)
			{
				break;
			}
		}

		{
			bool full = false;

			REFERENCE_TIME rt_max = 0;

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				CDSMMultiSourceOutputPin* pin = *i;

				if(pin->IsFull())
				{
					full = true;
				}

				REFERENCE_TIME rt = pin->GetQueueDuration();

				rt_max = std::max<REFERENCE_TIME>(rt_max, rt);
			}

			if(m_starvation)
			{
				const REFERENCE_TIME rt_limit = 10000000;

				m_progress = std::min<int>(100, (int)(rt_max * 100 / rt_limit));

				if(full || rt_max > rt_limit)
				{
					m_starvation = false;

					printf("resume\n");

					NotifyEvent(EC_USER, EC_USER_MAGIC, EC_USER_RESUME);
				}
			}

			if(full)
			{
				delay = 1;

				continue;
			}
		}

		CDSMPacket* p = NULL;

		if(CDSMSharedData* data = m_mapping.GetData())
		{
			ATL::CMutexLock lock(m_mapping.GetMutex());

			if(m_index == data->next - 1 && m_file.GetPos() >= data->size)
			{
				delay = 10;

				if(!m_starvation && m_State == State_Running)
				{
					bool starvation = true;

					for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
					{
						CDSMMultiSourceOutputPin* pin = *i;

						if(pin->GetQueueDuration() > 5000000)
						{
							starvation = false;
						}
					}

					if(starvation)
					{
						m_starvation = true;
						m_progress = 0;

						printf("suspend\n");

						NotifyEvent(EC_USER, EC_USER_MAGIC, EC_USER_PAUSE);
					}
				}

				continue;
			}

			if(m_index < data->next - 1 && m_file.GetPos() == m_file.GetLength())
			{
				delay = 10;

				SeekNext();

				continue;
			}

			dsmp_t type;
			UINT64 len;

			if(!m_file.ReadPacketHeader(type, len))
			{
				// WTF

				Seek(_I64_MAX);

				delay = 100;

				continue;
			}

			if(type == DSMP_SAMPLE)
			{
				p = new CDSMPacket();

				m_file.ByteRead(&p->id, 1);

				REFERENCE_TIME pts = 0;
				REFERENCE_TIME dur = 0;

				int pts_sign = 1;
				int pts_len = 0;
				int dur_len = 0;

				if(m_file.BitRead(1)) p->flags |= CDSMPacket::SyncPoint;
				if(m_file.BitRead(1)) pts_sign = -1;
				pts_len = m_file.BitRead(3);
				dur_len = m_file.BitRead(3);
				for(int i = 0; i < pts_len; i++) pts = (pts << 8) | m_file.BitRead(8);
				for(int i = 0; i < dur_len; i++) dur = (dur << 8) | m_file.BitRead(8);

				pts *= pts_sign;

				if(m_time.filter < pts) m_time.filter = pts;

				p->start = pts - m_time.start - m_time.offset;
				p->stop = p->start + dur;
				p->pts = pts;

				len -= 2 + pts_len + dur_len;

				p->buff.resize(len);

				m_file.ByteRead(&p->buff[0], len);
			}
			else
			{
				BYTE buff[64];

				while(len > 0)
				{
					int size = min(sizeof(buff), len);

					m_file.ByteRead(buff, size);

					len -= size;
				}
			}
		}

		if(p != NULL)
		{
			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				CDSMMultiSourceOutputPin* pin = *i;

				if(pin->GetId() == p->id)
				{
					/*
					if(pin->CurrentMediaType().majortype == MEDIATYPE_Video || pin->CurrentMediaType().majortype == MEDIATYPE_Audio)
					printf("[%p/%d] DSMSF Deliver start=%I64d valid=%d bogus=%d sync=%d size=%d [%02x %02x %02x %02x %02x %02x %02x]\n", 
						this, p->id, p->start / 10000, 
						(p->flags & CDSMPacket::TimeInvalid) ? 0 : 1, 
						(p->flags & CDSMPacket::Bogus) ? 1 : 0, 
						(p->flags & CDSMPacket::SyncPoint) ? 1 : 0,
						p->buff.size(),
						p->buff.size() > 0 ? p->buff[0] : 0,
						p->buff.size() > 1 ? p->buff[1] : 0,
						p->buff.size() > 2 ? p->buff[2] : 0,
						p->buff.size() > 3 ? p->buff[3] : 0,
						p->buff.size() > 4 ? p->buff[4] : 0,
						p->buff.size() > 5 ? p->buff[5] : 0,
						p->buff.size() > 6 ? p->buff[6] : 0);
					*/
					pin->Deliver(p);

					p = NULL;

					break;
				}
			}

			delete p;
		}

		delay = 0;
	}

	m_file.Close();

	m_mapping.Close();

	m_hThread = 0;

	DeliverEndOfStream();

	return 0;
}

// IFileSourceFilter

STDMETHODIMP CDSMMultiSourceFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt)
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		return VFW_E_NOT_STOPPED;
	}

	m_path.clear();

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}

	m_outputs.clear();

	std::unordered_map<int, CMediaType> mts;

	if(m_mapping.Open(pszFileName, mts))
	{
		for(auto i = mts.begin(); i != mts.end(); i++)
		{
			HRESULT hr;

			m_outputs.push_back(new CDSMMultiSourceOutputPin(this, i->first, i->second, &hr, L"Output"));
		}

		m_mapping.Close();
	}

	if(m_outputs.empty())
	{
		return E_FAIL;
	}

	m_path = pszFileName;
	
	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	if(ppszFileName == NULL) return E_POINTER;

	*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_path.size() + 1) * sizeof(WCHAR));
	
	if(*ppszFileName == NULL)
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_path.c_str());

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CDSMMultiSourceFilter::GetCapabilities(DWORD* pCapabilities)
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

STDMETHODIMP CDSMMultiSourceFilter::CheckCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL) return E_POINTER;

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;

	GetCapabilities(&caps);

	caps &= *pCapabilities;

	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CDSMMultiSourceFilter::IsFormatSupported(const GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CDSMMultiSourceFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CDSMMultiSourceFilter::GetTimeFormat(GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CDSMMultiSourceFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDSMMultiSourceFilter::GetDuration(LONGLONG* pDuration)
{
	if(pDuration == NULL)
	{
		return E_POINTER;
	}

	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());

		*pDuration = _I64_MAX; // data->stop;// - data->start;
	}
	else
	{
		*pDuration = 0;
	}

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::GetStopPosition(LONGLONG* pStop)
{
	if(pStop == NULL)
	{
		return E_POINTER;
	}

	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());

		*pStop = _I64_MAX; // data->stop;
	}
	else
	{
		*pStop = 0;
	}

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMMultiSourceFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDSMMultiSourceFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	REFERENCE_TIME rt = *pCurrent;

	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());
		
		rt = std::min<REFERENCE_TIME>(std::max<REFERENCE_TIME>(rt, data->start), data->stop);
	}

// printf("[%p] DSMSF Seek cur=%I64d rt=%I64d\n", this, *pCurrent / 10000, rt / 10000);

	*pCurrent = rt;

	m_time.newstart = rt;

	if(ThreadExists())
	{
		DeliverBeginFlush();

		CallWorker(0);
	}

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent == NULL || pStop == NULL)
	{
		return E_POINTER;
	}

	// TRACE(_T("%I64d %I64d %I64d\n"), m_mapping.GetData()->stop - m_time.pin, m_mapping.GetData()->stop - m_time.filter, m_time.filter - m_time.pin);

	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());

		*pCurrent = m_time.pin;
		*pStop = data->stop;
	}
	else
	{
		*pCurrent = 0;
		*pStop = 0;
	}

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest == NULL || pLatest == NULL)
	{
		return E_POINTER;
	}

	if(CDSMSharedData* data = m_mapping.GetData())
	{
		ATL::CMutexLock lock(m_mapping.GetMutex());

		*pEarliest = data->start;
		*pLatest = data->stop;
	}
	else
	{
		*pEarliest = 0;
		*pLatest = 0;
	}

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::SetRate(double dRate)
{
	return dRate == 1.0 ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDSMMultiSourceFilter::GetRate(double* pdRate)
{
	if(pdRate == NULL)
	{
		return E_POINTER;
	}

	*pdRate = 1.0;

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::GetPreroll(LONGLONG* pllPreroll)
{
	if(pllPreroll == NULL)
	{
		return E_POINTER;
	}

	*pllPreroll = 0;

	return S_OK;
}

// IAMOpenProgress

STDMETHODIMP CDSMMultiSourceFilter::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	CheckPointer(pllTotal, E_POINTER);
	CheckPointer(pllCurrent, E_POINTER);

	*pllTotal = 100;
	*pllCurrent = m_starvation ? m_progress : 100;

	return S_OK;
}

STDMETHODIMP CDSMMultiSourceFilter::AbortOperation()
{
	return S_OK;
}


// IDSMMultiSourceFilter

STDMETHODIMP_(bool) CDSMMultiSourceFilter::IsClosed()
{
	return m_mapping.GetData() == NULL;
}

// CDSMMultiSourceOutputPin

CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::CDSMMultiSourceOutputPin(CDSMMultiSourceFilter* pFilter, int id, const CMediaType& mt, HRESULT* phr, LPCWSTR name)
	: CBaseOutputPin(_T("CDSMMultiSourceOutputPin"), pFilter, pFilter, phr, name)
	, m_id(id)
	, m_queue(200)
	, m_flushing(false)
{
	m_mt = mt;
}


STDMETHODIMP CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::Active()
{
	m_queue_start = _I64_MIN;
	m_queue_stop = _I64_MIN;

	m_exit.Reset();

	CAMThread::Create();

	return __super::Active();
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::Inactive()
{
	DeliverBeginFlush();

	m_exit.Set();

	CAMThread::Close();

	DeliverEndFlush();

	return __super::Inactive();
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_mt == *pmt ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mt;

	return S_OK;
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1 << 20;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	return S_OK;
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::Deliver(CDSMPacket* item)
{
	if(m_flushing || !IsConnected()) {delete item; return S_FALSE;}

	if(item->type == CDSMPacket::Sample)
	{
		if(m_queue_start == _I64_MIN)
		{
			m_queue_start = item->start;
		}

		m_queue_stop = item->start;
	}

	m_queue.Enqueue(item);

	return S_OK;
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::DeliverNewSegment(REFERENCE_TIME start, REFERENCE_TIME stop)
{
	if(m_flushing || !IsConnected()) return S_FALSE;

	CDSMPacket* item = new CDSMPacket();

	item->id = m_id;
	item->type = CDSMPacket::NewSegment;
	item->start = start;
	item->stop = stop;

	m_queue.Enqueue(item);

	return S_OK;
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::DeliverEndOfStream()
{
	if(m_flushing || !IsConnected()) return S_FALSE;

	CDSMPacket* item = new CDSMPacket();

	item->id = m_id;
	item->type = CDSMPacket::EndOfStream;

	m_queue.Enqueue(item);

	return S_OK;
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::DeliverBeginFlush()
{
	if(!IsConnected()) return S_FALSE;

	m_flushing = true;

	return __super::DeliverBeginFlush();
}

HRESULT CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::DeliverEndFlush()
{
	if(!IsConnected()) return S_FALSE;

	m_queue.GetDequeueEvent().Wait();

	m_queue_start = _I64_MIN;
	m_queue_stop = _I64_MIN;

	m_flushing = false;

	return __super::DeliverEndFlush();
}

DWORD CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::ThreadProc()
{
	bool discontinuity = false;

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
				CDSMPacket* item = m_queue.Dequeue();

				if(!m_flushing)
				{
					if(item->type == CDSMPacket::Sample)
					{
						m_queue_start = item->start;

						if(item->start > -10000000)
						{
							if(discontinuity)
							{
								item->flags |= CDSMPacket::Discontinuity;

								discontinuity = false;
							}

							DeliverSample(item);
						}
					}
					else if(item->type == CDSMPacket::NewSegment)
					{
						discontinuity = true;

						__super::DeliverNewSegment(item->start, item->stop, 1.0f);
					}
					else if(item->type == CDSMPacket::EndOfStream)
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

void CDSMMultiSourceFilter::CDSMMultiSourceOutputPin::DeliverSample(CDSMPacket* item)
{
	CComPtr<IMediaSample> sample;

	if(FAILED(GetDeliveryBuffer(&sample, NULL, NULL, 0)))
	{
		return;
	}

	BYTE* dst = NULL;

	if(FAILED(sample->GetPointer(&dst)))
	{
		return;
	}

	int len = item->buff.size();

	if(len > sample->GetSize())
	{
		_tprintf(_T("!!! buffer size too small !!! %d > %d\n"), len, sample->GetSize());

		len = sample->GetSize();
	}

	memcpy(dst, &item->buff[0], len);

	sample->SetActualDataLength(len);

	sample->SetMediaTime(NULL, NULL);

	if((item->flags & CDSMPacket::TimeInvalid) == 0)
	{
		sample->SetTime(&item->start, &item->stop);

		sample->SetSyncPoint((item->flags & CDSMPacket::SyncPoint) ? TRUE : FALSE);

		CDSMMultiSourceFilter* f = (CDSMMultiSourceFilter*)m_pFilter;

		CAutoLock cAutoLock(&f->m_csPinTime);

		if(item->pts > f->m_time.pin) f->m_time.pin = item->pts;
	}
	else
	{
		sample->SetTime(NULL, NULL);

		sample->SetSyncPoint(FALSE);
	}

	sample->SetPreroll(FALSE); // hm

	sample->SetDiscontinuity((item->flags & CDSMPacket::Discontinuity) ? TRUE : FALSE);

	__super::Deliver(sample);
}
