#include "stdafx.h"
#include "DXVADecoderOutputPin.h"
#include "DXVADecoderAllocator.h"
#include "DXVADecoderFilter.h"

CDXVADecoderOutputPin::CDXVADecoderOutputPin(CDXVADecoderFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CBaseVideoOutputPin(L"CDXVADecoderOutputPin", pFilter, phr, pName)
	, m_pVideoDecFilter(pFilter)
	, m_pDXVA2Allocator(NULL)
	, m_dwDXVA1SurfaceCount(0)
	, m_GuidDecoderDXVA1(GUID_NULL)
{
	memset(&m_ddUncompPixelFormat, 0, sizeof(m_ddUncompPixelFormat));
}

CDXVADecoderOutputPin::~CDXVADecoderOutputPin()
{
}

STDMETHODIMP CDXVADecoderOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAMVideoAcceleratorNotify)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDXVADecoderOutputPin::Active()
{
	return __super::Active();
}

HRESULT CDXVADecoderOutputPin::Inactive()
{
	m_pVideoDecFilter->FlushDXVADecoder();

	return __super::Inactive();
}

HRESULT CDXVADecoderOutputPin::InitAllocator(IMemAllocator** ppAlloc)
{
	if(m_pVideoDecFilter->UseDXVA2())
	{
		HRESULT hr = S_FALSE;
		
		m_pDXVA2Allocator = new CDXVADecoderAllocator(m_pVideoDecFilter, &hr);
		
		if(!m_pDXVA2Allocator)
		{
			return E_OUTOFMEMORY;
		}

		if(FAILED(hr))
		{
			delete m_pDXVA2Allocator;
		
			return hr;
		}

		return m_pDXVA2Allocator->QueryInterface(__uuidof(IMemAllocator), (void **)ppAlloc);
	}

	return __super::InitAllocator(ppAlloc);
}

HRESULT CDXVADecoderOutputPin::Deliver(IMediaSample* pSample)
{
	REFERENCE_TIME start, stop;

	if(0) // SUCCEEDED(pSample->GetTime(&start, &stop)))
	{
		printf("[d] %I64d\n", start / 10000);
	}

	return __super::Deliver(pSample);
}

// IAMVideoAcceleratorNotify

STDMETHODIMP CDXVADecoderOutputPin::GetUncompSurfacesInfo(const GUID* pGuid, AMVAUncompBufferInfo* pUncompBufferInfo)
{
	HRESULT hr = E_INVALIDARG;
	
	if(SUCCEEDED(m_pVideoDecFilter->CheckDXVA1Decoder(pGuid)))
	{
		if(CComQIPtr<IAMVideoAccelerator> pAMVideoAccelerator = GetConnected())
		{
			pUncompBufferInfo->dwMaxNumSurfaces = m_pVideoDecFilter->GetPicEntryNumber();
			pUncompBufferInfo->dwMinNumSurfaces = m_pVideoDecFilter->GetPicEntryNumber();

			hr = m_pVideoDecFilter->FindDXVA1DecoderConfiguration(pAMVideoAccelerator, pGuid, &pUncompBufferInfo->ddUncompPixelFormat);

			if(SUCCEEDED(hr))
			{
				memcpy(&m_ddUncompPixelFormat, &pUncompBufferInfo->ddUncompPixelFormat, sizeof(DDPIXELFORMAT));

				m_GuidDecoderDXVA1 = *pGuid;
			}
		}
	}

	return hr;
}

STDMETHODIMP CDXVADecoderOutputPin::SetUncompSurfacesInfo(DWORD dwActualUncompSurfacesAllocated)      
{
	m_dwDXVA1SurfaceCount = dwActualUncompSurfacesAllocated;

	return S_OK;
}

STDMETHODIMP CDXVADecoderOutputPin::GetCreateVideoAcceleratorData(const GUID* pGuid, DWORD* pdwSizeMiscData, void** ppMiscData)
{
	HRESULT hr = E_UNEXPECTED;

	AMVAUncompDataInfo UncompInfo;
	AMVACompBufferInfo CompInfo[30];

	DWORD dwNumTypesCompBuffers	= sizeof(CompInfo) / sizeof(CompInfo[0]);
	DXVA_ConnectMode* pConnectMode;

	if(CComQIPtr<IAMVideoAccelerator> pAMVideoAccelerator = GetConnected())
	{
		memcpy(&UncompInfo.ddUncompPixelFormat, &m_ddUncompPixelFormat, sizeof(DDPIXELFORMAT));

		UncompInfo.dwUncompWidth = m_pVideoDecFilter->PictWidthRounded();
		UncompInfo.dwUncompHeight = m_pVideoDecFilter->PictHeightRounded();

		hr = pAMVideoAccelerator->GetCompBufferInfo(&m_GuidDecoderDXVA1, &UncompInfo, &dwNumTypesCompBuffers, CompInfo);

		if(SUCCEEDED(hr))
		{
			hr = m_pVideoDecFilter->CreateDXVA1Decoder(pAMVideoAccelerator, pGuid, m_dwDXVA1SurfaceCount);

			if(SUCCEEDED (hr))
			{
				m_pVideoDecFilter->SetDXVA1Params(&m_GuidDecoderDXVA1, &m_ddUncompPixelFormat);

				pConnectMode = (DXVA_ConnectMode*)CoTaskMemAlloc(sizeof(DXVA_ConnectMode));
				pConnectMode->guidMode = m_GuidDecoderDXVA1;
				pConnectMode->wRestrictedMode = m_pVideoDecFilter->GetDXVA1RestrictedMode();

				*pdwSizeMiscData = sizeof(DXVA_ConnectMode);
				*ppMiscData = pConnectMode;
			}
		}
	}

	return hr;
}
