#include "stdafx.h"
#include "VMR9AllocatorPresenter.h"
#include "DirectShow.h"

CVMR9AllocatorPresenter::CVMR9AllocatorPresenter(const VideoPresenterParam& params, HRESULT& hr)
	: CUnknown(_T("CVMR9AllocatorPresenter"), NULL)
	, m_params(params)
	, m_buffers(0)
	, m_fps(25)
{
	memset(&m_dst, 0, sizeof(m_dst));
	memset(&m_info, 0, sizeof(m_info));

	hr = m_params.dev && m_params.wnd ? S_OK : E_INVALIDARG;
}

CVMR9AllocatorPresenter::~CVMR9AllocatorPresenter()
{
	ReleaseSurfaces();
}

void CVMR9AllocatorPresenter::ReleaseSurfaces()
{
	for(auto i = m_surfaces.begin(); i != m_surfaces.end(); i++)
	{
		if(*i != NULL) (*i)->Release();
	}

	m_surfaces.clear();

	for(auto i = m_textures.begin(); i != m_textures.end(); i++)
	{
		if(*i != NULL) (*i)->Release();
	}

	m_textures.clear();
}

STDMETHODIMP CVMR9AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IVMRSurfaceAllocator9)
		QI(IVMRImagePresenter9)
		QI(IVMRWindowlessControl9)
		QI(IVideoPresenter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CVMR9AllocatorPresenter::CreateInstance(const VideoPresenterParam& params, IBaseFilter** ppBF, IVideoPresenter** ppVP)
{
	CheckPointer(ppBF, E_POINTER);
	CheckPointer(ppVP, E_POINTER);

	if(*ppBF) return E_INVALIDARG;
	if(*ppVP) return E_INVALIDARG;

	HRESULT hr;

	CComPtr<IBaseFilter> pBF;

	if(FAILED(hr = pBF.CoCreateInstance(CLSID_VideoMixingRenderer9)))
	{
		return E_FAIL;
	}

	CComQIPtr<IVMRFilterConfig9> pConfig = pBF;

	if(pConfig == NULL) 
	{
		return E_FAIL;
	}

	if(0)
	{
		if(FAILED(hr = pConfig->SetNumberOfStreams(1))) 
		{
			return E_FAIL;
		}

		if(0)
		{
			if(CComQIPtr<IVMRMixerControl9> pMC = pBF)
			{
				DWORD dwPrefs;
				pMC->GetMixingPrefs(&dwPrefs);  
				dwPrefs &= ~MixerPref9_RenderTargetMask; 
				dwPrefs |= MixerPref9_RenderTargetYUV;
				hr = pMC->SetMixingPrefs(dwPrefs);
			}
		}
	}

	//

	if(FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Renderless)))
	{
		return E_FAIL;
	}

	CComQIPtr<IVMRSurfaceAllocatorNotify9> pSAN = pBF;

	CComQIPtr<IVMRSurfaceAllocator9> pAP = new CVMR9AllocatorPresenter(params, hr);

	if(FAILED(hr))
	{
		return E_FAIL;
	}

	if(FAILED(hr = pSAN->AdviseSurfaceAllocator(0, pAP))
	|| FAILED(hr = pAP->AdviseNotify(pSAN)))
	{
		return E_FAIL;
	}

	*ppBF = pBF.Detach();
	*ppVP = CComQIPtr<IVideoPresenter>(pAP).Detach();

	return S_OK;
}

// IVMRSurfaceAllocator9

STDMETHODIMP CVMR9AllocatorPresenter::InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo* lpAllocInfo, DWORD* lpNumBuffers)
{
	CAutoLock cAutoLock(this);

	CheckPointer(lpAllocInfo, E_POINTER);
	CheckPointer(lpNumBuffers, E_POINTER);

	CheckPointer(m_params.dev, E_UNEXPECTED);
	CheckPointer(m_notify, E_UNEXPECTED);

	TerminateDevice(dwUserID);

	int w = lpAllocInfo->dwWidth;
	int h = abs((int)lpAllocInfo->dwHeight);

	m_size.x = w;
	m_size.y = h;
	
	if(lpAllocInfo->szAspectRatio.cx > 0 && lpAllocInfo->szAspectRatio.cy > 0)
	{
		m_ar.x = lpAllocInfo->szAspectRatio.cx;
		m_ar.y = abs((int)lpAllocInfo->szAspectRatio.cy);
	}
	else
	{
		m_ar = m_size;
	}

	HRESULT hr;

	if(lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)
	{
		lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
	}

	ReleaseSurfaces();
	
	//

	m_surfaces.resize(*lpNumBuffers);

	memset(&m_surfaces[0], 0, sizeof(m_surfaces[0]) * m_surfaces.size());

	if(FAILED(hr = m_notify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &m_surfaces[0])))
	{
		return hr;
	}

	for(int i = 0; i < 2; i++)
	{
		CComPtr<IDirect3DTexture9> t;

		if(FAILED(hr = m_params.dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &t, NULL)))
		{
			return hr;
		}

		m_textures.push_back(t.Detach());
	}

	if(!(lpAllocInfo->dwFlags & VMR9AllocFlag_TextureSurface))
	{
		// tests if the colorspace is acceptable
		
		CComPtr<IDirect3DSurface9> s;

		if(FAILED(hr = m_textures.front()->GetSurfaceLevel(0, &s)))
		{
			return hr;
		}

		if(FAILED(hr = m_params.dev->StretchRect(m_surfaces[0], NULL, s, NULL, D3DTEXF_NONE)))
		{
			return E_FAIL;
		}
	}

	return hr;
}

STDMETHODIMP CVMR9AllocatorPresenter::TerminateDevice(DWORD_PTR dwUserID)
{
	CAutoLock cAutoLock(this);

	ReleaseSurfaces();

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9** lplpSurface)
{
	CAutoLock cAutoLock(this);

	CheckPointer(lplpSurface, E_POINTER);

	if(SurfaceIndex < m_surfaces.size()) 
	{
		(*lplpSurface = m_surfaces[SurfaceIndex])->AddRef();
		
		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CVMR9AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify9* lpIVMRSurfAllocNotify)
{
	CAutoLock cAutoLock(this);

	CheckPointer(lpIVMRSurfAllocNotify, E_POINTER);

	m_notify = lpIVMRSurfAllocNotify;

	return m_notify->SetD3DDevice(m_params.dev, MonitorFromWindow(m_params.wnd, MONITOR_DEFAULTTONEAREST));
}

// IVMRImagePresenter9

STDMETHODIMP CVMR9AllocatorPresenter::StartPresenting(DWORD_PTR dwUserID)
{
	CAutoLock cAutoLock(this);

	CheckPointer(m_params.dev, E_UNEXPECTED);

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::StopPresenting(DWORD_PTR dwUserID)
{
	CAutoLock cAutoLock(this);

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo* lpPresInfo)
{
	CAutoLock cAutoLock(this);

	CheckPointer(lpPresInfo, E_UNEXPECTED);
	CheckPointer(lpPresInfo->lpSurf, E_UNEXPECTED);

	CheckPointer(m_params.dev, E_UNEXPECTED);
	CheckPointer(m_notify, E_UNEXPECTED);

	if(m_textures.empty())
	{
		return E_UNEXPECTED;
	}

	m_ar.x = lpPresInfo->szAspectRatio.cx;
	m_ar.y = lpPresInfo->szAspectRatio.cy;

	HRESULT hr;

	IDirect3DTexture9* t = m_textures.back();

	CComPtr<IDirect3DSurface9> s;

	hr = t->GetSurfaceLevel(0, &s);

	hr = m_params.dev->StretchRect(lpPresInfo->lpSurf, NULL, s, NULL, D3DTEXF_NONE);

	m_textures.pop_back();
	m_textures.push_front(t);

	if(m_params.flip != NULL)
	{
		SetEvent(m_params.flip);
	}

	if(lpPresInfo->rtEnd > lpPresInfo->rtStart)
	{
		m_fps = 10000000.0 / (lpPresInfo->rtEnd - lpPresInfo->rtStart);
	}

	return S_OK;
}

// IVMRWindowlessControl9

STDMETHODIMP CVMR9AllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	CAutoLock cAutoLock(this);

	if(lpWidth) *lpWidth = m_size.x;
	if(lpHeight) *lpHeight = m_size.y;
	if(lpARWidth) *lpARWidth = m_ar.x;
	if(lpARHeight) *lpARHeight = m_ar.y;

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight)
{
	return E_NOTIMPL;
}

STDMETHODIMP CVMR9AllocatorPresenter::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect)
{
	CAutoLock cAutoLock(this);

	if(lpDSTRect) m_dst = *lpDSTRect;

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
{
	CAutoLock cAutoLock(this);

	if(lpSRCRect) *lpSRCRect = m_dst;
	if(lpDSTRect) *lpDSTRect = m_dst;

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetAspectRatioMode(DWORD* lpAspectRatioMode)
{
	if(lpAspectRatioMode) *lpAspectRatioMode = AM_ARMODE_STRETCHED;

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::SetAspectRatioMode(DWORD AspectRatioMode)
{
	return AspectRatioMode == AM_ARMODE_STRETCHED ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CVMR9AllocatorPresenter::SetVideoClippingWindow(HWND hwnd)
{
	return E_NOTIMPL;
}

STDMETHODIMP CVMR9AllocatorPresenter::RepaintVideo(HWND hwnd, HDC hdc)
{
	return E_NOTIMPL;
}

STDMETHODIMP CVMR9AllocatorPresenter::DisplayModeChanged()
{
	return E_NOTIMPL;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetCurrentImage(BYTE** lpDib)
{
	// TODO

	return E_NOTIMPL;
}

STDMETHODIMP CVMR9AllocatorPresenter::SetBorderColor(COLORREF Clr)
{
	return E_NOTIMPL;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetBorderColor(COLORREF* lpClr)
{
	if(lpClr) *lpClr = 0;

	return S_OK;
}

// IVMR9AllocatorPresenter

STDMETHODIMP CVMR9AllocatorPresenter::GetVideoTexture(IDirect3DTexture9** ppt, Vector2i* ar, Vector4i* src)
{
	CAutoLock cAutoLock(this);

	CheckPointer(ppt, E_POINTER);

	if(m_textures.empty())
	{
		return E_UNEXPECTED;
	}

	ASSERT(*ppt == NULL);

	(*ppt = m_textures.front())->AddRef();

	*ar = m_ar;
	*src = Vector4i(0, 0, m_size.x, m_size.y);

	if(m_ar.x == 0 || m_ar.y == 0)
	{
		*ar = m_size;
	}

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::SetFlipEvent(HANDLE hFlipEvent)
{
	CAutoLock cAutoLock(this);

	m_params.flip = hFlipEvent;

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetFPS(double& fps)
{
	fps = m_fps;

	return S_OK;
}
