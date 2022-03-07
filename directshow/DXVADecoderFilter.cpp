#include "stdafx.h"
#include "DXVADecoderFilter.h"
#include "DXVADecoderOutputPin.h"
#include "DirectShow.h"
#include "moreuuids.h"
#include "../3rdparty/ffmpeg/ffmpeg.h"
#include <evr.h>

#define MAX_SUPPORTED_MODE 5

struct DXVA_PARAMS
{
	const int PicEntryNumber;
	const UINT PreferedConfigBitstream;
	const GUID* Decoder[MAX_SUPPORTED_MODE];
	const WORD RestrictedMode[MAX_SUPPORTED_MODE];
};

struct FFMPEG_CODECS
{
	const CLSID* clsMinorType;
	const enum CodecID nFFCodec;
	const int fourcc;
	const DXVA_PARAMS* DXVAModes;

	int DXVAModeCount()		
	{
		if(DXVAModes == NULL) return 0;

		for(int i = 0; i < MAX_SUPPORTED_MODE; i++)
		{
			if(DXVAModes->Decoder[i] == NULL) 
			{
				return i;
			}
		}

		return MAX_SUPPORTED_MODE;
	}
};

static DXVA_PARAMS DXVA_Mpeg2 = {9, 1, {&DXVA2_ModeMPEG2_VLD, NULL}, {DXVA_RESTRICTED_MODE_UNRESTRICTED, 0}}; // Restricted mode for DXVA1?
static DXVA_PARAMS DXVA_H264 = {16, 2, {&DXVA2_ModeH264_E, &DXVA2_ModeH264_F, /*&DXVA_Intel_H264_ClearVideo, */NULL}, {DXVA_RESTRICTED_MODE_H264_E, 0}};
static DXVA_PARAMS DXVA_H264_VISTA = {18, 2, {&DXVA2_ModeH264_E, &DXVA2_ModeH264_F, /*&DXVA_Intel_H264_ClearVideo, */NULL}, {DXVA_RESTRICTED_MODE_H264_E, 0}};
static DXVA_PARAMS DXVA_VC1 = {14, 1, {&DXVA2_ModeVC1_D, /*&DXVA_Intel_VC1_ClearVideo, NULL*/}, {DXVA_RESTRICTED_MODE_VC1_D, 0}};

static FFMPEG_CODECS ffCodecs[] =
{
	{ &MEDIASUBTYPE_MPEG2_VIDEO, CODEC_ID_MPEG2VIDEO,  MAKEFOURCC('M','P','G','2'), &DXVA_Mpeg2 },
	{ &MEDIASUBTYPE_H264, CODEC_ID_H264, MAKEFOURCC('H','2','6','4'), &DXVA_H264 },
	{ &MEDIASUBTYPE_h264, CODEC_ID_H264, MAKEFOURCC('h','2','6','4'), &DXVA_H264 },
	{ &MEDIASUBTYPE_X264, CODEC_ID_H264, MAKEFOURCC('X','2','6','4'), &DXVA_H264 },
	{ &MEDIASUBTYPE_x264, CODEC_ID_H264, MAKEFOURCC('x','2','6','4'), &DXVA_H264 },
	{ &MEDIASUBTYPE_VSSH, CODEC_ID_H264, MAKEFOURCC('V','S','S','H'), &DXVA_H264 },
	{ &MEDIASUBTYPE_vssh, CODEC_ID_H264, MAKEFOURCC('v','s','s','h'), &DXVA_H264 },
	{ &MEDIASUBTYPE_DAVC, CODEC_ID_H264, MAKEFOURCC('D','A','V','C'), &DXVA_H264 },
	{ &MEDIASUBTYPE_davc, CODEC_ID_H264, MAKEFOURCC('d','a','v','c'), &DXVA_H264 },
	{ &MEDIASUBTYPE_PAVC, CODEC_ID_H264, MAKEFOURCC('P','A','V','C'), &DXVA_H264 },
	{ &MEDIASUBTYPE_pavc, CODEC_ID_H264, MAKEFOURCC('p','a','v','c'), &DXVA_H264 },
	{ &MEDIASUBTYPE_AVC1, CODEC_ID_H264, MAKEFOURCC('A','V','C','1'), &DXVA_H264 },
	{ &MEDIASUBTYPE_avc1, CODEC_ID_H264, MAKEFOURCC('a','v','c','1'), &DXVA_H264 },
	{ &MEDIASUBTYPE_H264_TRANSPORT, CODEC_ID_H264, MAKEFOURCC('a','v','c','1'), &DXVA_H264 },
	{ &MEDIASUBTYPE_WVC1, CODEC_ID_VC1,  MAKEFOURCC('W','V','C','1'), &DXVA_VC1 },
	{ &MEDIASUBTYPE_wvc1, CODEC_ID_VC1,  MAKEFOURCC('w','v','c','1'), &DXVA_VC1 },
};

static const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	// { &MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_VIDEO },
	{ &MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_VIDEO },
	{ &MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_VIDEO },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H264 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_h264 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_X264 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_x264 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VSSH },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vssh },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DAVC },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_davc },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PAVC },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_pavc },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AVC1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_avc1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H264_TRANSPORT },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WVC1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wvc1 },
};

CDXVADecoderFilter::CDXVADecoderFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseVideoFilter(NAME("CDXVADecoderFilter"), lpunk, phr, __uuidof(this))
{
	OSVERSIONINFO osver;

	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	
	if(GetVersionEx(&osver) && osver.dwPlatformId == VER_PLATFORM_WIN32_NT && osver.dwMajorVersion >= 6)
	{
		for(int i = 0; i < sizeof(ffCodecs) / sizeof(ffCodecs[0]); i++)
		{
			if(ffCodecs[i].nFFCodec == CODEC_ID_H264)
			{
				ffCodecs[i].DXVAModes = &DXVA_H264_VISTA;
			}
		}
	}

	if(phr) *phr = S_OK;

	delete m_pOutput;
	
	m_pOutput = new CDXVADecoderOutputPin(this, phr, L"Output");

	m_pAVCodec = NULL;
	m_pAVCtx = NULL;
	m_pFrame = NULL;
	m_nCodecNb = -1;
	m_rtAvrTimePerFrame = 0;
	m_DXVADecoderGUID = GUID_NULL;

	m_nWidth = 0;
	m_nHeight = 0;
	
	m_nDXVAMode = MODE_NONE;
	m_pDXVADecoder = NULL;
	m_pVideoOutputFormat = NULL;
	m_nVideoOutputCount = 0;
	m_hDevice = INVALID_HANDLE_VALUE;

	m_sar = Vector2i(1, 1);
	
	avcodec_init();
	avcodec_register_all();

	DetectVideoCard(NULL);
}

CDXVADecoderFilter::~CDXVADecoderFilter()
{
	FreeInputResources();
	FreeOutputResources();
}

STDMETHODIMP CDXVADecoderFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IDXVADecoderFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

UINT CDXVADecoderFilter::GetAdapter(IDirect3D9* pD3D, HWND hWnd)
{
	if(hWnd != NULL && pD3D != NULL)
	{
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

		if(hMonitor != NULL) 
		{
			for(UINT i = 0, count = pD3D->GetAdapterCount(); i < count; ++i)
			{
				if(pD3D->GetAdapterMonitor(i) == hMonitor)
				{
					return i;
				}
			}
		}
	}
	
	return D3DADAPTER_DEFAULT;
}

void CDXVADecoderFilter::DetectVideoCard(HWND hWnd)
{
	m_nPCIVendor = 0;
	m_nPCIDevice = 0;
	m_VideoDriverVersion.HighPart = 0;
	m_VideoDriverVersion.LowPart = 0;

	if(IDirect3D9* pD3D9 = Direct3DCreate9(D3D_SDK_VERSION)) 
	{
		D3DADAPTER_IDENTIFIER9 id;

		if(pD3D9->GetAdapterIdentifier(GetAdapter(pD3D9, hWnd), 0, &id) == S_OK) 
		{
			m_nPCIVendor = id.VendorId;
			m_nPCIDevice = id.DeviceId;
			m_VideoDriverVersion = id.DriverVersion;
			m_strDeviceDescription = id.Description;
			m_strDeviceDescription += Util::Format(" (%d)", m_nPCIVendor);

			if(m_nPCIVendor == 0 && stricmp(id.Driver, "nvd3dum.dll") == 0)
			{
				m_nPCIVendor = PCIV_nVidia;
			}
		}

		pD3D9->Release();
	}
}

void CDXVADecoderFilter::GetOutputSize(Vector2i& size, Vector2i& ar, Vector2i& realsize)
{
	size = Vector2i(PictWidthRounded(), PictHeightRounded());
	realsize = Vector2i(m_nWidth, m_nHeight);
}

int CDXVADecoderFilter::PictWidth()
{
	return m_nWidth; 
}

int CDXVADecoderFilter::PictHeight()
{
	return m_nHeight;
}

int CDXVADecoderFilter::PictWidthRounded()
{
	return (m_nWidth + 15) & ~15; // Picture height should be rounded to 16 for DXVA
}

int CDXVADecoderFilter::PictHeightRounded()
{
	return (m_nHeight + 15) & ~15; // Picture height should be rounded to 16 for DXVA
}

int CDXVADecoderFilter::FindCodec(const CMediaType* mtIn)
{
	for(int i = 0; i < sizeof(ffCodecs) / sizeof(ffCodecs[0]); i++)
	{
		if(mtIn->subtype == *ffCodecs[i].clsMinorType)
		{
			switch(ffCodecs[i].nFFCodec)
			{
			case CODEC_ID_H264:
			case CODEC_ID_VC1:
			case CODEC_ID_MPEG2VIDEO:
				return i;
			}

			break;
		}
	}

	return -1;
}

void CDXVADecoderFilter::FreeInputResources()
{
	if(m_pAVCtx)
	{
		if(m_pAVCtx->intra_matrix) free(m_pAVCtx->intra_matrix);
		if(m_pAVCtx->inter_matrix) free(m_pAVCtx->inter_matrix);
		if(m_pAVCtx->extradata) free((unsigned char*)m_pAVCtx->extradata);
		if(m_pAVCtx->slice_offset) av_free(m_pAVCtx->slice_offset);
		if(m_pAVCtx->codec) avcodec_close(m_pAVCtx);

		av_free(m_pAVCtx);
	}

	if(m_pFrame)
	{
		av_free(m_pFrame);
	}

	m_pAVCodec = NULL;
	m_pAVCtx = NULL;
	m_pFrame = NULL;
	m_nCodecNb = -1;
	
	//

	delete [] m_pVideoOutputFormat;

	m_pVideoOutputFormat = NULL;
}

void CDXVADecoderFilter::FreeOutputResources()
{
	delete m_pDXVADecoder;

	m_pDXVADecoder = NULL;

	if(m_hDevice != INVALID_HANDLE_VALUE)
	{
		m_pDeviceManager->CloseDeviceHandle(m_hDevice);
		m_hDevice = INVALID_HANDLE_VALUE;
	}

	m_pDeviceManager = NULL;
	m_pDecoderService = NULL;
}

HRESULT CDXVADecoderFilter::CheckInputType(const CMediaType* mtIn)
{
	for(int i = 0; i < sizeof(sudPinTypesIn) / sizeof(sudPinTypesIn[0]); i++)
	{
		if(mtIn->majortype == *sudPinTypesIn[i].clsMajorType && mtIn->subtype == *sudPinTypesIn[i].clsMinorType)
		{
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDXVADecoderFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	int nNewCodec;

	if(dir == PINDIR_INPUT)
	{
		nNewCodec = FindCodec(pmt);

		if(nNewCodec == -1)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		if(nNewCodec != m_nCodecNb)
		{
			m_nCodecNb	= nNewCodec;

			m_pAVCodec = avcodec_find_decoder(ffCodecs[nNewCodec].nFFCodec);

			CheckPointer(m_pAVCodec, VFW_E_UNSUPPORTED_VIDEO);

			m_pAVCtx = avcodec_alloc_context();

			CheckPointer(m_pAVCtx, E_POINTER);

			m_pFrame = avcodec_alloc_frame();

			CheckPointer(m_pFrame, E_POINTER);

			if(pmt->formattype == FORMAT_VideoInfo)
			{
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;

				m_pAVCtx->width = vih->bmiHeader.biWidth;
				m_pAVCtx->height = abs(vih->bmiHeader.biHeight);
				m_pAVCtx->codec_tag	= vih->bmiHeader.biCompression;
			}
			else if(pmt->formattype == FORMAT_VideoInfo2)
			{
				VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pmt->pbFormat;

				m_pAVCtx->width = vih2->bmiHeader.biWidth;
				m_pAVCtx->height = abs(vih2->bmiHeader.biHeight);
				m_pAVCtx->codec_tag	= vih2->bmiHeader.biCompression;
			}
			else if(pmt->formattype == FORMAT_MPEGVideo)
			{
				MPEG1VIDEOINFO* mpgv = (MPEG1VIDEOINFO*)pmt->pbFormat;

				m_pAVCtx->width = mpgv->hdr.bmiHeader.biWidth;
				m_pAVCtx->height = abs(mpgv->hdr.bmiHeader.biHeight);
				m_pAVCtx->codec_tag = mpgv->hdr.bmiHeader.biCompression;
			}
			else if(pmt->formattype == FORMAT_MPEG2Video)
			{
				MPEG2VIDEOINFO* mpg2v = (MPEG2VIDEOINFO*)pmt->pbFormat;

				m_pAVCtx->width = mpg2v->hdr.bmiHeader.biWidth;
				m_pAVCtx->height = abs(mpg2v->hdr.bmiHeader.biHeight);
				m_pAVCtx->codec_tag = mpg2v->hdr.bmiHeader.biCompression;

				if(mpg2v->hdr.bmiHeader.biCompression == NULL)
				{
					m_pAVCtx->codec_tag = pmt->subtype.Data1;
				}
				else if(m_pAVCtx->codec_tag == MAKEFOURCC('a','v','c','1') || m_pAVCtx->codec_tag == MAKEFOURCC('A','V','C','1'))
				{
					m_pAVCtx->nal_length_size = mpg2v->dwFlags;
				}
			}

			m_nWidth = m_pAVCtx->width;
			m_nHeight = m_pAVCtx->height;
			m_rtAvrTimePerFrame = pmt->GetAvgTimePerFrame(400000);
			
			m_pAVCtx->intra_matrix = (uint16_t*)calloc(sizeof(uint16_t),64);
			m_pAVCtx->inter_matrix = (uint16_t*)calloc(sizeof(uint16_t),64);
			m_pAVCtx->codec_tag = ffCodecs[nNewCodec].fourcc;
			m_pAVCtx->workaround_bugs = FF_BUG_AUTODETECT;
			m_pAVCtx->error_concealment = FF_EC_DEBLOCK | FF_EC_GUESS_MVS;
			m_pAVCtx->error_recognition = FF_ER_CAREFUL;
			m_pAVCtx->idct_algo = FF_IDCT_AUTO;
			m_pAVCtx->skip_loop_filter = AVDISCARD_DEFAULT;
			m_pAVCtx->dsp_mask = FF_MM_FORCE;
			m_pAVCtx->postgain = 1.0f;
			m_pAVCtx->debug_mv = 0;
			
			AllocExtradata(m_pAVCtx, pmt);

			if(avcodec_open(m_pAVCtx, m_pAVCodec) < 0)
			{
				return VFW_E_INVALIDMEDIATYPE;
			}

			switch(ffCodecs[m_nCodecNb].nFFCodec)
			{
			case CODEC_ID_H264:

				if(m_nPCIVendor == PCIV_nVidia) // TODO: B-only, http://www.mythtv.org/wiki/VDPAU#Supported_Cards
				{
					int w = PictWidthRounded();

					if(w >= 769 && w <= 784 
					|| w >= 849 && w <= 864
					|| w >= 929 && w <= 944
					|| w >= 1009 && w <= 1024
					|| w >= 1793 && w <= 1808
					|| w >= 1873 && w <= 1888
					|| w >= 1953 && w <= 1968
					|| w >= 2033 && w <= 2048)
					{
						return VFW_E_INVALIDMEDIATYPE;
					}
				}
				
				int nCompat;
				
				nCompat = FFH264CheckCompatibility(PictWidthRounded(), PictHeightRounded(), m_pAVCtx, (BYTE*)m_pAVCtx->extradata, m_pAVCtx->extradata_size, m_nPCIVendor, m_VideoDriverVersion);
				
				if(nCompat > 0)
				{
					switch(-1)
					{
					case 0:
						return VFW_E_INVALIDMEDIATYPE;
					case 1:
						if(nCompat == 2) return VFW_E_INVALIDMEDIATYPE;
						break;
					case 2:
						if(nCompat == 1) return VFW_E_INVALIDMEDIATYPE;
						break;
					}
				}

				break;

			case CODEC_ID_MPEG2VIDEO:
				
				m_pAVCtx->dsp_mask ^= FF_MM_FORCE; // DSP is disable for DXVA decoding (to keep default idct_permutation)
				
				break;
			}

			BuildDXVAOutputFormat();
		}
	}

	return __super::SetMediaType(dir, pmt);
}

void CDXVADecoderFilter::BuildDXVAOutputFormat()
{
	static VIDEO_OUTPUT_FORMATS DXVA2Formats[] =
	{
		{&MEDIASUBTYPE_NV12, 1, 12, 'avxd'},
		{&MEDIASUBTYPE_NV12, 1, 12, 'AVXD'},
		{&MEDIASUBTYPE_NV12, 1, 12, 'AVxD'},
		{&MEDIASUBTYPE_NV12, 1, 12, 'AvXD'}
	};

	delete [] m_pVideoOutputFormat;

	m_nVideoOutputCount = ffCodecs[m_nCodecNb].DXVAModeCount() + sizeof(DXVA2Formats) / sizeof(DXVA2Formats[0]);
	m_pVideoOutputFormat = new VIDEO_OUTPUT_FORMATS[m_nVideoOutputCount];

	int i = 0;

	// Dynamic DXVA media types for DXVA1
	
	for(; i < ffCodecs[m_nCodecNb].DXVAModeCount(); i++)
	{
		m_pVideoOutputFormat[i].subtype = ffCodecs[m_nCodecNb].DXVAModes->Decoder[i];
		m_pVideoOutputFormat[i].biCompression = 'avxd';
		m_pVideoOutputFormat[i].biBitCount = 12;
		m_pVideoOutputFormat[i].biPlanes = 1;
	}

	// Static list for DXVA2

	memcpy(&m_pVideoOutputFormat[i], DXVA2Formats, sizeof(DXVA2Formats));
}

int CDXVADecoderFilter::GetPicEntryNumber()
{
	return ffCodecs[m_nCodecNb].DXVAModes->PicEntryNumber;
}

void CDXVADecoderFilter::GetOutputFormats(int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats)
{
	nNumber = m_nVideoOutputCount;
	*ppFormats = m_pVideoOutputFormat;
}

void CDXVADecoderFilter::AllocExtradata(AVCodecContext* pAVCtx, const CMediaType* pmt)
{
	const BYTE* data = NULL;
	unsigned int size = 0;

	if(pmt->formattype == FORMAT_VideoInfo)
	{
		size = pmt->cbFormat - sizeof(VIDEOINFOHEADER);
		data = size ? pmt->pbFormat + sizeof(VIDEOINFOHEADER) : NULL;
	}
	else if(pmt->formattype == FORMAT_VideoInfo2)
	{
		size = pmt->cbFormat - sizeof(VIDEOINFOHEADER2);
		data = size ? pmt->pbFormat + sizeof(VIDEOINFOHEADER2) : NULL;
	}
	else if(pmt->formattype == FORMAT_MPEGVideo)
	{
		MPEG1VIDEOINFO* mpeg1info = (MPEG1VIDEOINFO*)pmt->pbFormat;

		if(mpeg1info->cbSequenceHeader > 0)
		{
			size = mpeg1info->cbSequenceHeader;
			data = mpeg1info->bSequenceHeader;
		}
	}
	else if(pmt->formattype == FORMAT_MPEG2Video)
	{
		MPEG2VIDEOINFO* mpeg2info = (MPEG2VIDEOINFO*)pmt->pbFormat;

		if(mpeg2info->cbSequenceHeader > 0)
		{
			size = mpeg2info->cbSequenceHeader;
			data = (const BYTE*)mpeg2info->dwSequenceHeader;
		}
	}

	if(size > 0)
	{
		BYTE* extradata = (BYTE*)calloc(1, size + FF_INPUT_BUFFER_PADDING_SIZE);

		pAVCtx->extradata_size = size;
		pAVCtx->extradata = extradata;

		memcpy(extradata, data, size);
	}
}

HRESULT CDXVADecoderFilter::CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin)
{
	if(dir == PINDIR_INPUT && m_pOutput->IsConnected())
	{
		ReconnectOutput(m_nWidth, m_nHeight);
	}
	else if(dir == PINDIR_OUTPUT)
	{
		if(m_nDXVAMode == MODE_DXVA1)
		{
			m_pDXVADecoder->ConfigureDXVA1();
		}
		else if(SUCCEEDED(ConfigureDXVA2(pReceivePin)) && SUCCEEDED(SetEVRForDXVA2(pReceivePin)))
		{
			m_nDXVAMode = MODE_DXVA2;
		}

		if(m_nDXVAMode == MODE_NONE)
		{
			return VFW_E_INVALIDMEDIATYPE;
		}
	}

	return __super::CompleteConnect(dir, pReceivePin);
}

HRESULT CDXVADecoderFilter::BreakConnect(PIN_DIRECTION dir)
{
	if(dir == PINDIR_INPUT)
	{
		FreeInputResources();
	}
	else if(dir == PINDIR_OUTPUT)
	{
		FreeOutputResources();
	}

	return __super::BreakConnect(dir);
}

HRESULT CDXVADecoderFilter::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(UseDXVA2())
	{
		pProperties->cbBuffer = 1;
		pProperties->cBuffers = GetPicEntryNumber();

		return S_OK;
	}

	return __super::GetBufferSize(pProperties);
}

HRESULT CDXVADecoderFilter::NewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	if(m_pAVCtx)
	{
		avcodec_flush_buffers(m_pAVCtx);
	}

	if(m_pDXVADecoder)
	{
		m_pDXVADecoder->Flush();
	}

	return __super::NewSegment(rtStart, rtStop, dRate);
}

HRESULT CDXVADecoderFilter::Transform(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

	BYTE* pDataIn;
	int nSize;

	if(FAILED(hr = pIn->GetPointer(&pDataIn)))
	{
		return hr;
	}

	nSize = pIn->GetActualDataLength();

	REFERENCE_TIME start = _I64_MIN;
	REFERENCE_TIME stop = _I64_MIN;

	hr = pIn->GetTime(&start, &stop);

	if(SUCCEEDED(hr) && stop <= start && stop != _I64_MIN)
	{
		stop = start + m_rtAvrTimePerFrame;
	}

// printf("v %I64d\n", start / 10000);

	m_pAVCtx->reordered_opaque = start;
	m_pAVCtx->reordered_opaque2 = stop;

	CheckPointer(m_pDXVADecoder, E_UNEXPECTED);

	UpdateAspectRatio();

	// Change aspect ratio for DXVA1

	if(m_nDXVAMode == MODE_DXVA1 && ReconnectOutput(PictWidthRounded(), PictHeightRounded()) == S_OK) // , true, PictWidth(), PictHeight()
	{
		m_pDXVADecoder->ConfigureDXVA1();
	}

	hr = m_pDXVADecoder->DecodeFrame(pDataIn, nSize, start, stop);

	return SUCCEEDED(hr) ? S_OK : hr;
}

void CDXVADecoderFilter::UpdateAspectRatio()
{
	if(m_pAVCtx != NULL && m_pAVCtx->sample_aspect_ratio.num > 0 && m_pAVCtx->sample_aspect_ratio.den > 0)
	{ 
		Vector2i SAR(m_pAVCtx->sample_aspect_ratio.num, m_pAVCtx->sample_aspect_ratio.den); 

		if(m_sar != SAR) 
		{ 
			m_sar = SAR; 

			SetAspectRatio(Vector2i(m_nWidth * SAR.x / SAR.y, m_nHeight));
		}
	}
}

BOOL CDXVADecoderFilter::IsSupportedDecoderMode(const GUID& mode)
{
	for(int i = 0; i < MAX_SUPPORTED_MODE && ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] != NULL; i++)
	{
		if(*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == mode)
		{
			return true;
		}
	}

	return false;
}

BOOL CDXVADecoderFilter::IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered)
{
	bIsPrefered = config.ConfigBitstreamRaw == ffCodecs[m_nCodecNb].DXVAModes->PreferedConfigBitstream;

	return nD3DFormat == MAKEFOURCC('N', 'V', '1', '2');
}

HRESULT CDXVADecoderFilter::FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService* pDecoderService, const GUID& guidDecoder, DXVA2_ConfigPictureDecode* pSelectedConfig, BOOL* pbFoundDXVA2Configuration)
{
    HRESULT hr = S_OK;

	UINT cFormats = 0;
    UINT cConfigurations = 0;
	bool bIsPrefered = false;
    D3DFORMAT* pFormats = NULL; // size = cFormats
    DXVA2_ConfigPictureDecode* pConfig = NULL; // size = cConfigurations

    // Find the valid render target formats for this decoder GUID.

	hr = pDecoderService->GetDecoderRenderTargets(guidDecoder, &cFormats, &pFormats);
	
    if(SUCCEEDED(hr))
    {
        // Look for a format that matches our output format.

        for(UINT iFormat = 0; iFormat < cFormats; iFormat++)
        {
            // Fill in the video description. Set the width, height, format, and frame rate.

			memset(&m_VideoDesc, 0, sizeof(m_VideoDesc));

			m_VideoDesc.SampleWidth = PictWidthRounded();
			m_VideoDesc.SampleHeight = PictHeightRounded();
			m_VideoDesc.UABProtectionLevel = 1;
            m_VideoDesc.Format = pFormats[iFormat];

            // Get the available configurations.

			hr = pDecoderService->GetDecoderConfigurations(guidDecoder, &m_VideoDesc, NULL, &cConfigurations, &pConfig);

            if(FAILED(hr))
            {
                continue;
            }

            // Find a supported configuration.
            
			for(UINT iConfig = 0; iConfig < cConfigurations; iConfig++)
            {
                if(IsSupportedDecoderConfig(pFormats[iFormat], pConfig[iConfig], bIsPrefered))
                {
                    // This configuration is good.
					
					if(bIsPrefered || !*pbFoundDXVA2Configuration)
					{
						*pbFoundDXVA2Configuration = TRUE;
						*pSelectedConfig = pConfig[iConfig];
					}
                    
					if(bIsPrefered) break;
                }
            }

            CoTaskMemFree(pConfig);
        }
    }

    CoTaskMemFree(pFormats);

    // Note: It is possible to return S_OK without finding a configuration.

	return hr;
}

HRESULT CDXVADecoderFilter::ConfigureDXVA2(IPin* pPin)
{
    HRESULT hr = S_OK;

	if(CComQIPtr<IMFGetService> pGetService = pPin)
    {
	    CComPtr<IDirect3DDeviceManager9> pDeviceManager;

        hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, __uuidof(IDirect3DDeviceManager9), (void**)&pDeviceManager);

	    if(pDeviceManager != NULL)
	    {
		    HANDLE hDevice = INVALID_HANDLE_VALUE;
	    
			hr = pDeviceManager->OpenDeviceHandle(&hDevice);

			if(SUCCEEDED(hr))
		    {
			    CComPtr<IDirectXVideoDecoderService> pDecoderService;
		    
				hr = pDeviceManager->GetVideoService(hDevice, __uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService);

			    if(SUCCEEDED(hr))
			    {
				    UINT cDecoderGuids = 0;
				    GUID* pDecoderGuids = NULL;

			        hr = pDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids);

				    if(SUCCEEDED(hr))
				    {
					    BOOL bFoundDXVA2Configuration = FALSE;

						for(UINT iGuid = 0; iGuid < cDecoderGuids; iGuid++)
				        {
							if(!IsSupportedDecoderMode(pDecoderGuids[iGuid]))
				            {
				                continue;
				            }

						    DXVA2_ConfigPictureDecode config;

						    memset(&config, 0, sizeof(config));

				            hr = FindDXVA2DecoderConfiguration(pDecoderService, pDecoderGuids[iGuid], &config, &bFoundDXVA2Configuration);

				            if(FAILED(hr))
				            {
				                break;
				            }

				            if(bFoundDXVA2Configuration)
				            {
								m_pDeviceManager = pDeviceManager;
						        m_pDecoderService = pDecoderService;
						        m_DXVA2Config = config;
						        m_DXVADecoderGUID = pDecoderGuids[iGuid];
						        m_hDevice = hDevice;

								break;
				            }
				        }
						
						CoTaskMemFree(pDecoderGuids);

			            if(bFoundDXVA2Configuration)
			            {
							return S_OK;
			            }
					}
				}

				pDeviceManager->CloseDeviceHandle(hDevice);
		    }
	    }
    }

    return E_FAIL;
}

HRESULT CDXVADecoderFilter::SetEVRForDXVA2(IPin* pPin)
{
    HRESULT hr;

    if(CComQIPtr<IMFGetService> pGetService = pPin)
	{
	    CComPtr<IDirectXVideoMemoryConfiguration> pVideoConfig;

        hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, __uuidof(IDirectXVideoMemoryConfiguration), (void**)&pVideoConfig);
		
		if(SUCCEEDED(hr))
		{
			CComPtr<IMFVideoDisplayControl> pVDC;

			if(SUCCEEDED(pGetService->GetService(MR_VIDEO_RENDER_SERVICE, __uuidof(IMFVideoDisplayControl), (void**)&pVDC)))
			{
				HWND hWnd;

				if(SUCCEEDED(pVDC->GetVideoWindow(&hWnd)))
				{
					DetectVideoCard(hWnd);
				}
			}

	        DXVA2_SurfaceType surfaceType;

	        for(DWORD iTypeIndex = 0; ; iTypeIndex++)
	        {
	            hr = pVideoConfig->GetAvailableSurfaceTypeByIndex(iTypeIndex, &surfaceType);
	            
	            if(FAILED(hr))
				{
					break;
				}

	            if(surfaceType == DXVA2_SurfaceType_DecoderRenderTarget)
	            {
	                hr = pVideoConfig->SetSurfaceType(DXVA2_SurfaceType_DecoderRenderTarget);
	                
					break;
	            }
	        }
		}
	}

	return hr;
}

HRESULT CDXVADecoderFilter::CreateDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets)
{
	HRESULT hr;

	CComPtr<IDirectXVideoDecoder> pDirectXVideoDec;
	
	if(m_pDXVADecoder != NULL)
	{
		m_pDXVADecoder->SetDirectXVideoDec(NULL);
	}

	hr = m_pDecoderService->CreateVideoDecoder(m_DXVADecoderGUID, &m_VideoDesc, &m_DXVA2Config, pDecoderRenderTargets, nNumRenderTargets, &pDirectXVideoDec);

	if(SUCCEEDED(hr))
	{
		if(m_pDXVADecoder == NULL)
		{
			m_pDXVADecoder = CDXVADecoder::CreateDecoder(this, pDirectXVideoDec, &m_DXVADecoderGUID, GetPicEntryNumber(), &m_DXVA2Config);

			if(m_pDXVADecoder != NULL) 
			{
				m_pDXVADecoder->SetExtraData((BYTE*)m_pAVCtx->extradata, m_pAVCtx->extradata_size);
			}
		}

		m_pDXVADecoder->SetDirectXVideoDec(pDirectXVideoDec);
	}

	return hr;
}

HRESULT CDXVADecoderFilter::FindDXVA1DecoderConfiguration(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DDPIXELFORMAT* pPixelFormat)
{
	HRESULT hr = E_FAIL;

	DWORD dwFormats = 0;

	pAMVideoAccelerator->GetUncompFormatsSupported(guidDecoder, &dwFormats, NULL);

	if(dwFormats > 0)
	{
	    // Find the valid render target formats for this decoder GUID.
		
		DDPIXELFORMAT* pPixelFormats = new DDPIXELFORMAT[dwFormats];
		
		hr = pAMVideoAccelerator->GetUncompFormatsSupported(guidDecoder, &dwFormats, pPixelFormats);
		
		if(SUCCEEDED(hr))
		{
			// Look for a format that matches our output format.

			hr = E_FAIL;

			for(DWORD iFormat = 0; iFormat < dwFormats; iFormat++)
			{
				if(pPixelFormats[iFormat].dwFourCC == MAKEFOURCC('N', 'V', '1', '2'))
				{
					memcpy(pPixelFormat, &pPixelFormats[iFormat], sizeof(DDPIXELFORMAT));

					hr = S_OK;

					break;
				}
			}

			delete [] pPixelFormats;

			pPixelFormats = NULL;
		}
	}

	return hr;
}

HRESULT CDXVADecoderFilter::CheckDXVA1Decoder(const GUID* pGuid)
{
	if(m_nCodecNb != -1)
	{
		for(int i = 0; i < MAX_SUPPORTED_MODE && ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] != NULL; i++)
		{
			if(*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == *pGuid)
			{
				return S_OK;
			}
		}
	}

	return E_INVALIDARG;
}

void CDXVADecoderFilter::SetDXVA1Params(const GUID* pGuid, DDPIXELFORMAT* pPixelFormat)
{
	m_DXVADecoderGUID = *pGuid;

	memcpy(&m_PixelFormat, pPixelFormat, sizeof(DDPIXELFORMAT));
}

WORD CDXVADecoderFilter::GetDXVA1RestrictedMode()
{
	if(m_nCodecNb != -1)
	{
		for(int i = 0; i < MAX_SUPPORTED_MODE && ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] != NULL; i++)
		{
			if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == m_DXVADecoderGUID)
			{
				return ffCodecs[m_nCodecNb].DXVAModes->RestrictedMode[i];
			}
		}
	}

	return DXVA_RESTRICTED_MODE_UNRESTRICTED;
}

HRESULT CDXVADecoderFilter::CreateDXVA1Decoder(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* pDecoderGuid, DWORD dwSurfaceCount)
{
	if(m_pDXVADecoder != NULL && m_DXVADecoderGUID == *pDecoderGuid) 
	{
		return S_OK;
	}

	delete m_pDXVADecoder;

	m_nDXVAMode = MODE_DXVA1;
	m_DXVADecoderGUID = *pDecoderGuid;
	m_pDXVADecoder = CDXVADecoder::CreateDecoder(this, pAMVideoAccelerator, &m_DXVADecoderGUID, dwSurfaceCount);
	
	if(m_pDXVADecoder != NULL)
	{
		m_pDXVADecoder->SetExtraData((BYTE*)m_pAVCtx->extradata, m_pAVCtx->extradata_size);
	}

	return S_OK;
}

STDMETHODIMP_(GUID*) CDXVADecoderFilter::GetDXVADecoderGuid()
{
	return m_pGraph ? &m_DXVADecoderGUID : NULL;
}
