#pragma once
#include "BaseVideo.h"
#include "DXVADecoder.h"

typedef enum
{
	MODE_NONE,
	MODE_DXVA1,
	MODE_DXVA2
} DXVA_MODE;

[uuid("ADC3B5B3-A8B0-4c70-A805-9FC80CDEF262")]
interface IDXVADecoderFilter : public IUnknown
{
	STDMETHOD_(GUID*, GetDXVADecoderGuid()) = 0;
};

struct AVCodec;
struct AVCodecContext;
struct AVFrame;

[uuid("108BAC12-FBAF-497b-9670-BC6F6FBAE2C4")]
class CDXVADecoderFilter 
	: public CBaseVideoFilter
	, public IDXVADecoderFilter
{
	friend class CDXVADecoderAllocator;

	// === FFMpeg variables

	AVCodec* m_pAVCodec;
	AVCodecContext* m_pAVCtx;
	AVFrame* m_pFrame;
	int m_nCodecNb;
	REFERENCE_TIME m_rtAvrTimePerFrame;
	int m_nWidth;				// Frame width give to input pin
	int m_nHeight;				// Frame height give to input pin
	Vector2i m_sar;

	// === DXVA common variables

	VIDEO_OUTPUT_FORMATS* m_pVideoOutputFormat;
	int m_nVideoOutputCount;
	DXVA_MODE m_nDXVAMode;
	CDXVADecoder* m_pDXVADecoder;
	GUID m_DXVADecoderGUID;

	int m_nPCIVendor;
	int m_nPCIDevice;
	LARGE_INTEGER m_VideoDriverVersion;
	std::string m_strDeviceDescription;

	// === DXVA1 variables

	DDPIXELFORMAT m_PixelFormat;

	// === DXVA2 variables

	CComPtr<IDirect3DDeviceManager9> m_pDeviceManager;
	CComPtr<IDirectXVideoDecoderService> m_pDecoderService;
	DXVA2_ConfigPictureDecode m_DXVA2Config;
	HANDLE m_hDevice;
	DXVA2_VideoDesc m_VideoDesc;

	// === Private functions

	void FreeInputResources();
	void FreeOutputResources();
	int FindCodec(const CMediaType* mtIn);
	void AllocExtradata(AVCodecContext* pAVCtx, const CMediaType* mt);
	void DetectVideoCard(HWND hWnd);
	UINT GetAdapter(IDirect3D9* pD3D, HWND hWnd);

public:
	CDXVADecoderFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CDXVADecoderFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
	
	CTransformOutputPin* GetOutputPin() { return m_pOutput; }
	void UpdateFrameTime(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	
	void GetOutputSize(Vector2i& size, Vector2i& ar, Vector2i& realsize);
	void GetOutputFormats(int& count, VIDEO_OUTPUT_FORMATS** formats);
	bool CanResetAllocator() {return false;} // FIXME

	// === Overriden DirectShow functions

	HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType* pmt);
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT	Transform(IMediaSample* pIn);
	HRESULT	CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin);
    HRESULT	GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
	HRESULT	NewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate);
	HRESULT	BreakConnect(PIN_DIRECTION dir);

	// === IDXVADecoderFilter

	STDMETHOD_(GUID*, GetDXVADecoderGuid());

	// === DXVA common functions

	BOOL IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered);
	BOOL IsSupportedDecoderMode(const GUID& mode);
	void BuildDXVAOutputFormat();
	int GetPicEntryNumber();
	int	PictWidth();
	int	PictHeight();
	int	PictWidthRounded();
	int	PictHeightRounded();
	bool UseDXVA2()	{ return m_nDXVAMode == MODE_DXVA2; };
	void FlushDXVADecoder()	{ if (m_pDXVADecoder) m_pDXVADecoder->Flush(); }
	AVCodecContext* GetAVCtx() { return m_pAVCtx; };
	AVFrame* GetFrame() { return m_pFrame; }
	int GetPCIVendor() { return m_nPCIVendor; };
	REFERENCE_TIME GetAvrTimePerFrame() { return m_rtAvrTimePerFrame; };
	void UpdateAspectRatio();

	// === DXVA1 functions

	DDPIXELFORMAT* GetPixelFormat() { return &m_PixelFormat; }
	HRESULT FindDXVA1DecoderConfiguration(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DDPIXELFORMAT* pPixelFormat);
	HRESULT CheckDXVA1Decoder(const GUID* pGuid);
	void SetDXVA1Params(const GUID* pGuid, DDPIXELFORMAT* pPixelFormat);
	WORD GetDXVA1RestrictedMode();
	HRESULT CreateDXVA1Decoder(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* pDecoderGuid, DWORD dwSurfaceCount);

	// === DXVA2 functions

	DXVA2_ConfigPictureDecode* GetDXVA2Config() { return &m_DXVA2Config; };
	HRESULT ConfigureDXVA2(IPin *pPin);
	HRESULT	SetEVRForDXVA2(IPin *pPin);
	HRESULT	FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService* pDecoderService, const GUID& guidDecoder, DXVA2_ConfigPictureDecode* pSelectedConfig, BOOL* pbFoundDXVA2Configuration);
	HRESULT CreateDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets);
};
