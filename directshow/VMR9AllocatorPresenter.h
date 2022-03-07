#pragma once

#include "IVideoPresenter.h"
#include <vmr9.h>

[uuid("AD8AA335-3D1E-4FC8-879B-361418AE0024")]
class CVMR9AllocatorPresenter
	: public CUnknown
	, public CCritSec
	, public IVMRSurfaceAllocator9
	, public IVMRImagePresenter9
	, public IVMRWindowlessControl9
	, public IVideoPresenter
{
protected:
	VideoPresenterParam m_params; 
	VMR9AllocationInfo m_info;
	CComPtr<IVMRSurfaceAllocatorNotify9> m_notify;
	std::vector<IDirect3DSurface9*> m_surfaces;
	std::list<IDirect3DTexture9*> m_textures;
	DWORD m_buffers;
	Vector2i m_size;
	Vector2i m_ar;
	RECT m_dst;
	double m_fps;

	void ReleaseSurfaces();

public:
	CVMR9AllocatorPresenter(const VideoPresenterParam& params, HRESULT& hr);
	virtual ~CVMR9AllocatorPresenter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	static HRESULT CreateInstance(const VideoPresenterParam& p, IBaseFilter** ppBF, IVideoPresenter** ppVP);

	// IVMRSurfaceAllocator9

	STDMETHODIMP InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo* lpAllocInfo, DWORD* lpNumBuffers);
	STDMETHODIMP TerminateDevice(DWORD_PTR dwID);
	STDMETHODIMP GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9** lplpSurface);
	STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify9* lpIVMRSurfAllocNotify);

	// IVMRImagePresenter9

	STDMETHODIMP StartPresenting(DWORD_PTR dwUserID);
	STDMETHODIMP StopPresenting(DWORD_PTR dwUserID);
	STDMETHODIMP PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo* lpPresInfo);

	// IVMRWindowlessControl9

	STDMETHODIMP GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight);
	STDMETHODIMP GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight);
	STDMETHODIMP GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight);
	STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
	STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect);
	STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode);
	STDMETHODIMP SetAspectRatioMode(DWORD AspectRatioMode);
	STDMETHODIMP SetVideoClippingWindow(HWND hwnd);
	STDMETHODIMP RepaintVideo(HWND hwnd, HDC hdc);
	STDMETHODIMP DisplayModeChanged();
	STDMETHODIMP GetCurrentImage(BYTE** lpDib);
	STDMETHODIMP SetBorderColor(COLORREF Clr);
	STDMETHODIMP GetBorderColor(COLORREF* lpClr);

	// IVMR9AllocatorPresenter

	STDMETHODIMP GetVideoTexture(IDirect3DTexture9** ppt, Vector2i* ar, Vector4i* src);
	STDMETHODIMP SetFlipEvent(HANDLE hFlipEvent);
	STDMETHODIMP GetFPS(double& fps);
};
