#include "stdafx.h"
#include "DXVADecoderAllocator.h"
#include "DXVADecoderFilter.h"
#include <mfapi.h>
#include <mferror.h>

CDXVA2Sample::CDXVA2Sample(CDXVADecoderAllocator* pAlloc, HRESULT *phr)
	: CMediaSample(L"CDXVA2Sample", pAlloc, phr, NULL, 0)
	, m_dwSurfaceId(0)
{ 
}

// Note: CMediaSample does not derive from CUnknown, so we cannot use the
//       DECLARE_IUNKNOWN macro that is used by most of the filter classes.

STDMETHODIMP CDXVA2Sample::QueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	if(riid == __uuidof(IMFGetService))
	{
		return GetInterface((IMFGetService*)this, ppv);
	}
	else if(riid == __uuidof(IDXVA2Sample))
	{
		return GetInterface((IDXVA2Sample*)this, ppv);
	}

	return __super::QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CDXVA2Sample::AddRef()
{
	return __super::AddRef();
}

STDMETHODIMP_(ULONG) CDXVA2Sample::Release()
{
	// Return a temporary variable for thread safety.

	ULONG cRef = __super::Release();

	return cRef;
}

// IMFGetService

STDMETHODIMP CDXVA2Sample::GetService(REFGUID guidService, REFIID riid, LPVOID *ppv)
{
	if(guidService != MR_BUFFER_SERVICE)
	{
		return MF_E_UNSUPPORTED_SERVICE;
	}
	else if(m_pSurface == NULL)
	{
		return E_NOINTERFACE;
	}
	else
	{
		return m_pSurface->QueryInterface(riid, ppv);
	}
}

void CDXVA2Sample::SetSurface(DWORD surfaceId, IDirect3DSurface9* pSurf)
{
	// Sets the pointer to the Direct3D surface. 

	m_pSurface = pSurf;
	m_dwSurfaceId = surfaceId;
}

STDMETHODIMP_(int) CDXVA2Sample::GetDXSurfaceId()
{
	return m_dwSurfaceId;
}

//

CDXVADecoderAllocator::CDXVADecoderAllocator(CDXVADecoderFilter* pVideoDecFilter, HRESULT* phr)
	: CBaseAllocator(L"CDXVADecoderAllocator", NULL, phr)
	, m_pVideoDecFilter(pVideoDecFilter)
	, m_ppRTSurfaceArray(NULL)
{
}

CDXVADecoderAllocator::~CDXVADecoderAllocator()
{
	Free();
}

HRESULT CDXVADecoderAllocator::Alloc()
{
	CheckPointer(m_pVideoDecFilter->m_pDeviceManager, E_UNEXPECTED);

	HRESULT hr;

	CComPtr<IDirectXVideoAccelerationService> pDXVA2Service;

	hr = m_pVideoDecFilter->m_pDeviceManager->GetVideoService(m_pVideoDecFilter->m_hDevice, IID_IDirectXVideoAccelerationService, (void**)&pDXVA2Service);
	
	CheckPointer(pDXVA2Service, E_UNEXPECTED);
	
	CAutoLock lock(this);

    hr = __super::Alloc();

	if(SUCCEEDED(hr))
    {
        Free();

		m_nSurfaceArrayCount = m_lCount;

        m_ppRTSurfaceArray = new IDirect3DSurface9*[m_lCount];
        
		if(m_ppRTSurfaceArray == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memset(m_ppRTSurfaceArray, 0, sizeof(IDirect3DSurface9*) * m_lCount);
        }
    }

	D3DFORMAT m_dwFormat = m_pVideoDecFilter->m_VideoDesc.Format;

	if(SUCCEEDED(hr))
    {
        hr = pDXVA2Service->CreateSurface(
            m_pVideoDecFilter->PictWidthRounded(),
            m_pVideoDecFilter->PictHeightRounded(),
            m_lCount - 1,
            (D3DFORMAT)m_dwFormat,
            D3DPOOL_DEFAULT,
            0,
            DXVA2_VideoDecoderRenderTarget,
            m_ppRTSurfaceArray,
            NULL);
    }

    if(SUCCEEDED(hr))
    {
		m_lAllocated = m_lCount;

		// Important : create samples in reverse order !
		
		for(int i = m_lCount - 1; i >= 0; i--)
        {
            CDXVA2Sample* pSample = new CDXVA2Sample(this, &hr);

			if(pSample == NULL)
			{
				hr = E_OUTOFMEMORY;

				break;
			}

			if(FAILED(hr))
            {
                break;
            }

            // Assign the Direct3D surface pointer and the index.
            
			pSample->SetSurface(i, m_ppRTSurfaceArray[i]);

            // Add to the sample list.
            
			m_lFree.push_front(pSample);
        }

		hr = m_pVideoDecFilter->CreateDXVA2Decoder(m_lCount, m_ppRTSurfaceArray);
		
		if(FAILED(hr)) 
		{
			Free();
		}
	}

    if(SUCCEEDED(hr))
    {
        m_bChanged = FALSE;
    }

    return hr;
}

void CDXVADecoderAllocator::Free()
{
	for(auto i = m_lFree.begin(); i != m_lFree.end(); i++)
	{
		delete *i;
	}

	m_lFree.clear();

    if(m_ppRTSurfaceArray)
    {
        for(UINT i = 0; i < m_nSurfaceArrayCount; i++)
        {
            if(m_ppRTSurfaceArray[i] != NULL)
			{
				m_ppRTSurfaceArray[i]->Release();
			}
		}

        delete [] m_ppRTSurfaceArray;
		
		m_ppRTSurfaceArray = NULL;
    }

    m_lAllocated = 0;
	m_nSurfaceArrayCount = 0;
}

