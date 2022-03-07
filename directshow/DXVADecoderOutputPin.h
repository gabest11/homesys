#pragma once

#include <videoacc.h>
#include "BaseVideo.h"

class CDXVADecoderFilter;
class CDXVADecoderAllocator;

class CDXVADecoderOutputPin : public CBaseVideoOutputPin, public IAMVideoAcceleratorNotify
{
	CDXVADecoderFilter*	m_pVideoDecFilter;
	CDXVADecoderAllocator*	m_pDXVA2Allocator;
	DWORD m_dwDXVA1SurfaceCount;
	GUID m_GuidDecoderDXVA1;
	DDPIXELFORMAT m_ddUncompPixelFormat;

	HRESULT Active();
	HRESULT Inactive();

public:
	CDXVADecoderOutputPin(CDXVADecoderFilter* pFilter, HRESULT* phr, LPCWSTR pName);
	virtual ~CDXVADecoderOutputPin();

	HRESULT InitAllocator(IMemAllocator** ppAlloc);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT Deliver(IMediaSample* pSample);

	// IAMVideoAcceleratorNotify

	STDMETHODIMP GetUncompSurfacesInfo(const GUID* pGuid, AMVAUncompBufferInfo* pUncompBufferInfo);        
	STDMETHODIMP SetUncompSurfacesInfo(DWORD dwActualUncompSurfacesAllocated);        
	STDMETHODIMP GetCreateVideoAcceleratorData(const GUID* pGuid, DWORD* pdwSizeMiscData, void** ppMiscData);
};
