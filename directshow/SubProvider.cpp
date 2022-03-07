#include "StdAfx.h"
#include "SubProvider.h"

// CSubPic

CSubPic::CSubPic(const SubPicDesc& desc)
	: CUnknown(L"CSubPic", NULL)
{
	m_start = desc.start;
	m_stop = desc.stop;
	m_rect = Vector4(desc.bbox) / Vector4(desc.size.x, desc.size.y).xyxy();
}

STDMETHODIMP CSubPic::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ISubPic)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP_(REFERENCE_TIME) CSubPic::GetStart()
{
	return m_start;
}

STDMETHODIMP_(REFERENCE_TIME) CSubPic::GetStop()
{
	return m_stop;
}

STDMETHODIMP CSubPic::GetRect(Vector4i& r)
{
	r = r.xyxy() + Vector4i(m_rect * Vector4(r.rsize()).zwzw());

	return S_OK;
}

// CSubProvider

CSubProvider::CSubProvider()
	: CUnknown(NAME("CSubProvider"), NULL)
{
}

CSubProvider::~CSubProvider()
{
}

STDMETHODIMP CSubProvider::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ISubProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubProvider

STDMETHODIMP CSubProvider::Lock()
{
	CCritSec::Lock();

	return S_OK;
}

STDMETHODIMP CSubProvider::Unlock()
{
	CCritSec::Unlock();

	return S_OK;
}

// CSubPicQueue

CSubPicQueue::CSubPicQueue(const Vector2i& maxsize, int maxsubpic) 
	: CUnknown(NAME("ISubPicQueueImpl"), NULL)
	, m_maxsubpic(std::max<int>(maxsubpic, 0))
	, m_start(0)
	, m_break(false)
	, m_now(0)
	, m_fps(25.0f)
{
	m_target.size = Vector2i(0, 0);
	m_target.rect = Vector4i::zero();

	m_desc.w = maxsize.x;
	m_desc.h = maxsize.y;
	m_desc.pitch = ((maxsize.x + 3) & ~3) << 2;
	m_desc.bits = (BYTE*)_aligned_malloc(m_desc.pitch * m_desc.h, 16);

	memset(m_desc.bits, 0, m_desc.pitch * m_desc.h);
	
	if(m_maxsubpic > 0)
	{
		for(int i = 0; i < sizeof(m_events) / sizeof(m_events[0]); i++) 
		{
			m_events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		}

		CAMThread::Create();
	}
}

CSubPicQueue::~CSubPicQueue()
{
	if(m_maxsubpic > 0)
	{
		m_break = true;

		SetEvent(m_events[CMD_EXIT]);

		CAMThread::Close();
		
		for(int i = 0; i < sizeof(m_events) / sizeof(m_events[0]); i++) 
		{
			CloseHandle(m_events[i]);
		}
	}

	_aligned_free(m_desc.bits);

	m_desc.bits = NULL;
}

STDMETHODIMP CSubPicQueue::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(ISubPicQueue)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicQueue

STDMETHODIMP CSubPicQueue::SetSubProvider(ISubProvider* provider)
{
	CAutoLock cAutoLock(this);

//	if(m_provider != provider)
	{
		m_provider = provider;

		Invalidate();
	}

	return S_OK;
}

STDMETHODIMP CSubPicQueue::GetSubProvider(ISubProvider** provider)
{
	CheckPointer(provider, E_POINTER);

	*provider = NULL;

	CAutoLock cAutoLock(this);

	if(m_provider != NULL)
	{
		(*provider = m_provider)->AddRef();

		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CSubPicQueue::SetTime(REFERENCE_TIME now)
{
	CAutoLock cAutoLock(this);

	m_now = now;

	if(m_maxsubpic > 0)
	{
		SetEvent(m_events[CMD_TIME]);
	}

	return S_OK;
}

STDMETHODIMP CSubPicQueue::SetFPS(float fps)
{
	CAutoLock cAutoLock(this);

	m_fps = fps;

	if(m_maxsubpic > 0)
	{
		SetEvent(m_events[CMD_TIME]);
	}

	return S_OK;
}

STDMETHODIMP CSubPicQueue::SetTarget(const Vector2i& s, const Vector4i& r)
{
	CAutoLock cAutoLock(this);

	bool changed = m_target.size != s || !m_target.rect.eq(r);

	m_target.size = s; 
	m_target.rect = r; 

	if(changed)
	{
		Invalidate(0);
	}

	return S_OK;
}

STDMETHODIMP CSubPicQueue::Invalidate(REFERENCE_TIME rt)
{
	CAutoLock cAutoLock(this);

	if(m_maxsubpic > 0)
	{
		m_invalidate = rt;
	
		m_break = true;

		SetEvent(m_events[CMD_TIME]);
	}
	else
	{
		m_subpics.clear();
	}

	return S_OK;
}

STDMETHODIMP_(bool) CSubPicQueue::LookupSubPic(REFERENCE_TIME now, ISubPic** ppSubPic)
{
	*ppSubPic = NULL;

	{
		CAutoLock cAutoLock(this);

		for(auto i = m_subpics.begin(); i != m_subpics.end(); i++)
		{
			CComPtr<ISubPic> sp = *i;

			if(sp->GetStart() <= now && now < sp->GetStop())
			{
				*ppSubPic = sp.Detach();

				return true;
			}
		}
	}

	if(m_maxsubpic == 0)
	{
		CComPtr<ISubProvider> provider;

		if(SUCCEEDED(GetSubProvider(&provider)) && provider != NULL && SUCCEEDED(provider->Lock()))
		{
			CComPtr<ISubPic> sp;
			
			float fps = m_fps;

			if(void* pos = provider->GetStartPosition(now, fps))
			{
				REFERENCE_TIME start, stop;

				provider->GetTime(pos, fps, start, stop);

				if(provider->IsAnimated(pos))
				{
					start = now;
					stop = now + 1;
				}

				if(start <= now && now < stop)
				{
					Render(provider, start, stop, fps, &sp);
				}
			}

			provider->Unlock();

			CAutoLock cAutoLock(this);

			m_subpics.clear();

			if(sp != NULL)
			{
				m_subpics.push_back(sp);

				*ppSubPic = sp.Detach();
			}
		}
	}

	return *ppSubPic != NULL;
}

STDMETHODIMP CSubPicQueue::GetStats(int& count, REFERENCE_TIME& now, REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	CAutoLock cAutoLock(this);

	count = m_subpics.size();
	now = m_now;
	start = m_start;
	stop = !m_subpics.empty() ? m_subpics.back()->GetStop() : start;

	return S_OK;
}

STDMETHODIMP CSubPicQueue::GetStats(int index, REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	CAutoLock cAutoLock(this);

	start = stop = -1;

	for(auto i = m_subpics.begin(); i != m_subpics.end(); i++)
	{
		if(index-- == 0)
		{
			CComPtr<ISubPic> sp = *i;

			start = sp->GetStart();
			stop = sp->GetStop();

			return S_OK;
		}
	}

	return E_INVALIDARG;
}

//

bool CSubPicQueue::Render(ISubProvider* provider, const REFERENCE_TIME& start, const REFERENCE_TIME& stop, float fps, ISubPic** ppsp)
{
	*ppsp = NULL;

	m_desc.start = start;
	m_desc.stop = stop;

	m_desc.size = m_target.size;
	m_desc.rect = m_target.rect;

	if(m_desc.size.x > m_desc.w)
	{
		m_desc.size.y = MulDiv(m_desc.size.y, m_desc.w, m_desc.size.x);
		m_desc.size.x = m_desc.w;
	}

	if(m_desc.size.y > m_desc.h)
	{
		m_desc.size.x = MulDiv(m_desc.size.x, m_desc.h, m_desc.size.y);
		m_desc.size.y = m_desc.h;
	}

	if(m_desc.size != m_target.size)
	{
		m_desc.rect.top = MulDiv(m_desc.rect.top, m_desc.size.x, m_target.size.x);
		m_desc.rect.bottom = MulDiv(m_desc.rect.bottom, m_desc.size.x, m_target.size.x);
		m_desc.rect.left = MulDiv(m_desc.rect.left, m_desc.size.y, m_target.size.y);
		m_desc.rect.right = MulDiv(m_desc.rect.right, m_desc.size.y, m_target.size.y);
	}

	if(!m_desc.bbox.rempty())
	{
		BYTE* p = m_desc.bits + m_desc.pitch * m_desc.bbox.top + m_desc.bbox.left * 4;

		int w = m_desc.bbox.width();
		int h = m_desc.bbox.height();

		for(int j = 0; j < h; j++, p += m_desc.pitch)
		{
			memset(p, 0, w * 4);
		}
		
		m_desc.bbox = Vector4i::zero();
	}

	provider->Render(m_desc, fps);

	if(!m_desc.bbox.rempty())
	{
		// pitch mod16

		m_desc.bbox.left &= ~3;
		m_desc.bbox.right = (m_desc.bbox.right + 3) & ~3;

		CComPtr<ISubPic> sp;
		
		if(CreateSubPic(m_desc, &sp) && sp != NULL)
		{
			*ppsp = sp.Detach();
		}
	}

	return *ppsp != NULL;
}

REFERENCE_TIME CSubPicQueue::UpdateQueue()
{
	CAutoLock cAutoLock(this);

	REFERENCE_TIME now = m_now;

	if(now < m_start)
	{
		m_subpics.clear();
	}
	else
	{
		while(!m_subpics.empty() && now >= m_subpics.front()->GetStop())
		{
			m_subpics.pop_front();
		}
	}

	m_start = now;

	if(!m_subpics.empty())
	{
		now = m_subpics.back()->GetStop();
	}

	return now;
}

DWORD CSubPicQueue::ThreadProc()
{	
	SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST/*THREAD_PRIORITY_BELOW_NORMAL*/);

	while((WaitForMultipleObjects(sizeof(m_events) / sizeof(m_events[0]), m_events, FALSE, INFINITE) - WAIT_OBJECT_0) == CMD_TIME)
	{
		REFERENCE_TIME now = UpdateQueue();
		float fps = m_fps;

		CComPtr<ISubProvider> provider;

		if(SUCCEEDED(GetSubProvider(&provider)) && provider != NULL && SUCCEEDED(provider->Lock()))
		{
			if(m_subpics.size() < m_maxsubpic) // saves us from calling GetStartPosition when the buffer is already full (which is most of the time)
			{
				for(void* pos = provider->GetStartPosition(now, fps); 
					pos != NULL && !m_break && m_subpics.size() < m_maxsubpic; 
					pos = provider->GetNext(pos))
				{
					REFERENCE_TIME start = 0;
					REFERENCE_TIME stop = 0;

					provider->GetTime(pos, fps, start, stop);

					if(m_now >= start)
					{
						// m_fBufferUnderrun = true;

						if(m_now >= stop) 
						{
							continue;
						}
					}

					if(start >= m_now + 60 * 10000000i64) // we are already one minute ahead, this should be enough
					{
						break;
					}

					if(now < stop)
					{
						CComPtr<ISubPic> sp;

						if(Render(provider, start, stop, fps, &sp))
						{
							CAutoLock cAutoLock(this);

							m_subpics.push_back(sp);
						}
					}
				}
			}

			provider->Unlock();
		}

		if(m_break)
		{
			CAutoLock cAutoLock(this);

			REFERENCE_TIME rt = m_invalidate;

			while(!m_subpics.empty() && m_subpics.back()->GetStop() >= rt)
			{
				m_subpics.pop_back();
			}

			m_break = false;
		}
	}

	return 0;
}

// CDX9SubPicQueue

CDX9SubPicQueue::CDX9SubPicQueue(IDirect3DDevice9* dev, const Vector2i& maxsize, int maxsubpic)
	: CSubPicQueue(maxsize, maxsubpic)
	, m_dev(dev)
{
}

bool CDX9SubPicQueue::CreateSubPic(const SubPicDesc& desc, ISubPic** ppsp)
{
	CComPtr<IDirect3DTexture9> texture;

	int w = desc.bbox.width();
	int h = desc.bbox.height();

	if(FAILED(m_dev->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL)))
	{
		return false;
	}

	BYTE* RESTRICT src = desc.bits + desc.bbox.top * desc.pitch + desc.bbox.left * 4;

	D3DLOCKED_RECT lr;

	if(FAILED(texture->LockRect(0, &lr, NULL, 0)))
	{
		return false;
	}

	BYTE* RESTRICT dst = (BYTE*)lr.pBits;

	Vector4i mask_r = Vector4i::x000000ff();
	Vector4i mask_b = mask_r << 16;
	Vector4i mask_ag = Vector4i::xff00();

	for(int i = 0; i < h; i++)
	{
		Vector4i* RESTRICT s = (Vector4i*)src;
		Vector4i* RESTRICT d = (Vector4i*)dst;

		for(int j = 0, k = w >> 2; j < k; j++)
		{
			d[j] = (s[j] & mask_ag) | ((s[j] & mask_b) >> 16) | ((s[j] & mask_r) << 16); // TODO: ssse3 pshufb
		}

		src += desc.pitch;
		dst += lr.Pitch;
	}

	texture->UnlockRect(0);

	// ::D3DXSaveTextureToFile(L"C:\\1.dds", ::D3DXIMAGE_FILEFORMAT::D3DXIFF_DDS, texture, NULL);

	(*ppsp = new CDX9SubPic(desc, texture))->AddRef();

	return true;
}

CDX9SubPicQueue::CDX9SubPic::CDX9SubPic(const SubPicDesc& desc, IDirect3DTexture9* texture)
	: CSubPic(desc)
	, m_texture(texture)
{
}

STDMETHODIMP CDX9SubPicQueue::CDX9SubPic::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		riid == __uuidof(IDirect3DTexture9) ? ::GetInterface(m_texture.p, ppv) : 
		__super::NonDelegatingQueryInterface(riid, ppv);
}
