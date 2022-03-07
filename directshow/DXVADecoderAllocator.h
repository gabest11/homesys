#pragma once

#include <dxva.h>
#include <dxva2api.h>
#include <evr.h>

class CDXVADecoderFilter;
class CDXVADecoderAllocator;

[uuid("AE7EC2A2-1913-4a80-8DD6-DF1497ABA494")]
interface IDXVA2Sample : public IUnknown
{
	STDMETHOD_(int, GetDXSurfaceId()) = 0;
};

class CDXVA2Sample : public CMediaSample, public IMFGetService, public IDXVA2Sample
{
    friend class CDXVADecoderAllocator;

	CComPtr<IDirect3DSurface9>	m_pSurface;
    DWORD m_dwSurfaceId;
	
	void SetSurface(DWORD surfaceId, IDirect3DSurface9* pSurf);

public:
    CDXVA2Sample(CDXVADecoderAllocator* pAlloc, HRESULT *phr);

	// IUnknown

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFGetService

	STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID *ppv);

	// IDXVA2Sample
	
	STDMETHODIMP_(int) GetDXSurfaceId();

    // Override GetPointer because this class does not manage a system memory buffer.
    // The EVR uses the MR_BUFFER_SERVICE service to get the Direct3D surface.

	STDMETHODIMP GetPointer(BYTE** ppBuffer) {return E_NOTIMPL;}
};

class CDXVADecoderAllocator : public CBaseAllocator
{
	CDXVADecoderFilter* m_pVideoDecFilter;
	IDirect3DSurface9** m_ppRTSurfaceArray;
	UINT m_nSurfaceArrayCount;

protected:
	HRESULT Alloc();
	void Free();

public:
	CDXVADecoderAllocator(CDXVADecoderFilter* pVideoDecFilter, HRESULT* phr);
	virtual ~CDXVADecoderAllocator();

//   STDMETHODIMP GetBuffer(__deref_out IMediaSample **ppBuffer,		// Try for a circular buffer!
//                          __in_opt REFERENCE_TIME * pStartTime,
//                          __in_opt REFERENCE_TIME * pEndTime,
//                          DWORD dwFlags);
//
//    STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);
//	    CAtlList<int>				m_FreeSurface;

};
