#include "stdafx.h"
#include <dxva2api.h>
#include "DXVADecoderH264.h"
#include "DXVADecoderVC1.h"
#include "DXVADecoderMpeg2.h"
#include "DXVADecoderFilter.h"
#include "DXVADecoderAllocator.h"
#include "moreuuids.h"
#include <evr.h>

#define MAX_RETRY_ON_PENDING 50

#define DO_DXVA_PENDING_LOOP(x)	for(int i = 0; E_PENDING == (hr = (x)) && i < MAX_RETRY_ON_PENDING; i++) {Sleep(1);}

CDXVADecoder::CDXVADecoder(CDXVADecoderFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
{
	m_nEngine = ENGINE_DXVA1;
	m_pAMVideoAccelerator = pAMVideoAccelerator;
	m_dwBufferIndex = 0;
	m_nMaxWaiting = 3;

	Init(pFilter, nMode, nPicEntryNumber);
}

CDXVADecoder::CDXVADecoder(CDXVADecoderFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)	
{
	m_nEngine = ENGINE_DXVA2;
	m_pDirectXVideoDec = pDirectXVideoDec;
	memcpy(&m_DXVA2Config, pDXVA2Config, sizeof(DXVA2_ConfigPictureDecode));

	Init(pFilter, nMode, nPicEntryNumber);
};

CDXVADecoder::~CDXVADecoder()
{
	delete [] m_pPictureStore;
	delete [] m_ExecuteParams.pCompressedBuffers;
}

CDXVADecoder* CDXVADecoder::CreateDecoder(CDXVADecoderFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, int nPicEntryNumber)
{
	CDXVADecoder* pDecoder = NULL;

	if(*guidDecoder == DXVA2_ModeH264_E || *guidDecoder == DXVA2_ModeH264_F || *guidDecoder == DXVA_Intel_H264_ClearVideo)
	{
		pDecoder = new CDXVADecoderH264(pFilter, pAMVideoAccelerator, H264_VLD, nPicEntryNumber);
	}
	else if(*guidDecoder == DXVA2_ModeVC1_D || *guidDecoder == DXVA_Intel_VC1_ClearVideo)
	{
		pDecoder = new CDXVADecoderVC1(pFilter, pAMVideoAccelerator, VC1_VLD, nPicEntryNumber);
	}
	else if(*guidDecoder == DXVA2_ModeMPEG2_VLD)
	{
		pDecoder = new CDXVADecoderMpeg2(pFilter, pAMVideoAccelerator, MPEG2_VLD, nPicEntryNumber);
	}
	else
	{
		ASSERT(0); // Unknown decoder !!
	}

	return pDecoder;
}

CDXVADecoder* CDXVADecoder::CreateDecoder(CDXVADecoderFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
{
	CDXVADecoder* pDecoder = NULL;

	if(*guidDecoder == DXVA2_ModeH264_E || *guidDecoder == DXVA2_ModeH264_F || *guidDecoder == DXVA_Intel_H264_ClearVideo)
	{
		pDecoder = new CDXVADecoderH264(pFilter, pDirectXVideoDec, H264_VLD, nPicEntryNumber, pDXVA2Config);
	}
	else if(*guidDecoder == DXVA2_ModeVC1_D || *guidDecoder == DXVA_Intel_VC1_ClearVideo)
	{
		pDecoder = new CDXVADecoderVC1(pFilter, pDirectXVideoDec, VC1_VLD, nPicEntryNumber, pDXVA2Config);
	}
	else if(*guidDecoder == DXVA2_ModeMPEG2_VLD)
	{
		pDecoder = new CDXVADecoderMpeg2(pFilter, pDirectXVideoDec, MPEG2_VLD, nPicEntryNumber, pDXVA2Config);
	}
	else
	{
		ASSERT(0); // Unknown decoder !!
	}

	return pDecoder;
}

void CDXVADecoder::Init(CDXVADecoderFilter* pFilter, DXVAMode nMode, int nPicEntryNumber)
{
	m_pFilter = pFilter;
	m_nMode = nMode;
	m_nPicEntryNumber = nPicEntryNumber;
	m_pPictureStore = new PICTURE_STORE[nPicEntryNumber];
	m_dwNumBuffersInfo = 0;

	memset(&m_DXVA1Config, 0, sizeof(m_DXVA1Config));
	memset(&m_DXVA1BufferDesc, 0, sizeof(m_DXVA1BufferDesc));

	m_DXVA1Config.guidConfigBitstreamEncryption = DXVA_NoEncrypt;
	m_DXVA1Config.guidConfigMBcontrolEncryption = DXVA_NoEncrypt;
	m_DXVA1Config.guidConfigResidDiffEncryption = DXVA_NoEncrypt;
	m_DXVA1Config.bConfigBitstreamRaw = 2;

	memset(&m_DXVA1BufferInfo, 0, sizeof(m_DXVA1BufferInfo));
	memset(&m_ExecuteParams, 0, sizeof(m_ExecuteParams));

	Flush();
}

void CDXVADecoder::AllocExecuteParams(int nSize)
{
	m_ExecuteParams.pCompressedBuffers = new DXVA2_DecodeBufferDesc[nSize];

	for(int i = 0; i < nSize; i++)
	{
		memset(&m_ExecuteParams.pCompressedBuffers[i], 0, sizeof(DXVA2_DecodeBufferDesc));
	}
}

void CDXVADecoder::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	memcpy(pDXVABuffer, (BYTE*)pBuffer, nSize);
}

void CDXVADecoder::Flush()
{
	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		m_pPictureStore[i].bRefPicture = false;
		m_pPictureStore[i].bInUse = false;
		m_pPictureStore[i].bDisplayed = false;
		m_pPictureStore[i].pSample = NULL;
		m_pPictureStore[i].nCodecSpecific = -1;
		m_pPictureStore[i].dwDisplayCount = 0;
	}

	m_nWaitingPics = 0;
	m_bFlushed = true;
	m_nFieldSurface = -1;
	m_dwDisplayCount = 1;
	m_pFieldSample = NULL;
}

HRESULT CDXVADecoder::ConfigureDXVA1()
{
	HRESULT hr = S_FALSE;

	DXVA_ConfigPictureDecode ConfigRequested;

	if(m_pAMVideoAccelerator)
	{
		memset(&ConfigRequested, 0, sizeof(ConfigRequested));

		ConfigRequested.guidConfigBitstreamEncryption = DXVA_NoEncrypt;
		ConfigRequested.guidConfigMBcontrolEncryption = DXVA_NoEncrypt;
		ConfigRequested.guidConfigResidDiffEncryption = DXVA_NoEncrypt;
		ConfigRequested.bConfigBitstreamRaw = 2;

		writeDXVA_QueryOrReplyFunc(&ConfigRequested.dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_PROBE_QUERY, DXVA_PICTURE_DECODING_FUNCTION);

		hr = m_pAMVideoAccelerator->Execute(ConfigRequested.dwFunction, &ConfigRequested, sizeof(DXVA_ConfigPictureDecode), &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), 0, NULL);

		// Copy to DXVA2 structure (simplify code based on accelerator config)

		m_DXVA2Config.guidConfigBitstreamEncryption = m_DXVA1Config.guidConfigBitstreamEncryption;
		m_DXVA2Config.guidConfigMBcontrolEncryption = m_DXVA1Config.guidConfigMBcontrolEncryption;
		m_DXVA2Config.guidConfigResidDiffEncryption = m_DXVA1Config.guidConfigResidDiffEncryption;
		m_DXVA2Config.ConfigBitstreamRaw = m_DXVA1Config.bConfigBitstreamRaw;
		m_DXVA2Config.ConfigMBcontrolRasterOrder = m_DXVA1Config.bConfigMBcontrolRasterOrder;
		m_DXVA2Config.ConfigResidDiffHost = m_DXVA1Config.bConfigResidDiffHost;
		m_DXVA2Config.ConfigSpatialResid8 = m_DXVA1Config.bConfigSpatialResid8;
		m_DXVA2Config.ConfigResid8Subtraction = m_DXVA1Config.bConfigResid8Subtraction;
		m_DXVA2Config.ConfigSpatialHost8or9Clipping = m_DXVA1Config.bConfigSpatialHost8or9Clipping;
		m_DXVA2Config.ConfigSpatialResidInterleaved = m_DXVA1Config.bConfigSpatialResidInterleaved;
		m_DXVA2Config.ConfigIntraResidUnsigned = m_DXVA1Config.bConfigIntraResidUnsigned;
		m_DXVA2Config.ConfigResidDiffAccelerator = m_DXVA1Config.bConfigResidDiffAccelerator;
		m_DXVA2Config.ConfigHostInverseScan = m_DXVA1Config.bConfigHostInverseScan;
		m_DXVA2Config.ConfigSpecificIDCT = m_DXVA1Config.bConfigSpecificIDCT;
		m_DXVA2Config.Config4GroupedCoefs = m_DXVA1Config.bConfig4GroupedCoefs;

		if(SUCCEEDED(hr))
		{
			writeDXVA_QueryOrReplyFunc(&m_DXVA1Config.dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_LOCK_QUERY, DXVA_PICTURE_DECODING_FUNCTION);

			hr = m_pAMVideoAccelerator->Execute(m_DXVA1Config.dwFunction, &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), &ConfigRequested, sizeof(DXVA_ConfigPictureDecode), 0, NULL);

			// TODO : check config!

			// ASSERT(ConfigRequested.bConfigBitstreamRaw == 2);

			AMVAUncompDataInfo DataInfo;

			DataInfo.dwUncompWidth = m_pFilter->PictWidthRounded();
			DataInfo.dwUncompHeight = m_pFilter->PictHeightRounded(); 
			memcpy(&DataInfo.ddUncompPixelFormat, m_pFilter->GetPixelFormat(), sizeof(DDPIXELFORMAT));

			DWORD dwNum = COMP_BUFFER_COUNT;

			hr = m_pAMVideoAccelerator->GetCompBufferInfo(m_pFilter->GetDXVADecoderGuid(), &DataInfo, &dwNum, m_ComBufferInfo);
		}
	}

	return hr;
}

HRESULT CDXVADecoder::AddExecuteBuffer(DWORD CompressedBufferType, UINT nSize, void* pBuffer, UINT* pRealSize)
{
	HRESULT hr = E_INVALIDARG;
	
	DWORD dwTypeIndex;
	BYTE* pDXVABuffer;

	switch(m_nEngine)
	{
	case ENGINE_DXVA1:
		
		dwTypeIndex = GetDXVA1CompressedType(CompressedBufferType);

		LONG lStride;

		hr = m_pAMVideoAccelerator->GetBuffer(dwTypeIndex, m_dwBufferIndex, FALSE, (void**)&pDXVABuffer, &lStride);

		ASSERT(SUCCEEDED(hr));

		if(SUCCEEDED(hr))
		{
			if(CompressedBufferType == DXVA2_BitStreamDateBufferType)
			{
				CopyBitstream(pDXVABuffer, (BYTE*)pBuffer, nSize);
			}
			else
			{
				memcpy(pDXVABuffer, (BYTE*)pBuffer, nSize);
			}

			m_DXVA1BufferInfo[m_dwNumBuffersInfo].dwTypeIndex = dwTypeIndex;
			m_DXVA1BufferInfo[m_dwNumBuffersInfo].dwBufferIndex = m_dwBufferIndex;
			m_DXVA1BufferInfo[m_dwNumBuffersInfo].dwDataSize = nSize;

			m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwTypeIndex = dwTypeIndex;
			m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwBufferIndex = m_dwBufferIndex;
			m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwDataSize = nSize;
			m_DXVA1BufferDesc[m_dwNumBuffersInfo].dwNumMBsInBuffer = 0;

			m_dwNumBuffersInfo++;
		}

		break;

	case ENGINE_DXVA2:

		UINT nDXVASize;

		hr = m_pDirectXVideoDec->GetBuffer(CompressedBufferType, (void**)&pDXVABuffer, &nDXVASize);

		ASSERT(nSize <= nDXVASize);

		if(SUCCEEDED(hr) && nSize <= nDXVASize)
		{
			if(CompressedBufferType == DXVA2_BitStreamDateBufferType)
			{
				CopyBitstream(pDXVABuffer, (BYTE*)pBuffer, nSize);
			}
			else
			{
				memcpy(pDXVABuffer, (BYTE*)pBuffer, nSize);
			}

			m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].CompressedBufferType = CompressedBufferType;
			m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].DataSize = nSize;
			m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].NumMBsInBuffer = 0;
			m_ExecuteParams.NumCompBuffers++;
		}

		break;
	}

	if(pRealSize) 
	{
		*pRealSize = nSize;
	}

	return hr;
}

HRESULT CDXVADecoder::GetDeliveryBuffer(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, IMediaSample** ppSampleToDeliver)
{
	HRESULT hr;

	// Change aspect ratio for DXVA2

	if(m_nEngine == ENGINE_DXVA2)
	{
		m_pFilter->UpdateAspectRatio();
		m_pFilter->ReconnectOutput(m_pFilter->PictWidthRounded(), m_pFilter->PictHeightRounded()); //, true, m_pFilter->PictWidth(), m_pFilter->PictHeight());
	}

	CComPtr<IMediaSample> pNewSample;

	hr = m_pFilter->GetOutputPin()->GetDeliveryBuffer(&pNewSample, 0, 0, 0);

	if(SUCCEEDED(hr))
	{
		pNewSample->SetTime(&rtStart, &rtStop);
		pNewSample->SetMediaTime(NULL, NULL);

		*ppSampleToDeliver = pNewSample.Detach();
	}

	return hr;
}

HRESULT CDXVADecoder::Execute()
{
	HRESULT hr = E_INVALIDARG;

	DWORD dwResult;
		
	switch(m_nEngine)
	{
	case ENGINE_DXVA1:

//		writeDXVA_QueryOrReplyFunc (&dwFunction, DXVA_QUERYORREPLYFUNCFLAG_DECODER_LOCK_QUERY, DXVA_PICTURE_DECODING_FUNCTION);
//		hr = m_pAMVideoAccelerator->Execute (dwFunction, &m_DXVA1Config, sizeof(DXVA_ConfigPictureDecode), NULL, 0, m_dwNumBuffersInfo, m_DXVA1BufferInfo);

		hr = m_pAMVideoAccelerator->Execute(0x01000000, m_DXVA1BufferDesc, sizeof(DXVA_BufferDescription) * m_dwNumBuffersInfo, &dwResult, sizeof(dwResult), m_dwNumBuffersInfo, m_DXVA1BufferInfo);
		
		ASSERT(SUCCEEDED(hr));

		for(DWORD i = 0; i < m_dwNumBuffersInfo; i++)
		{
			HRESULT hr2 = m_pAMVideoAccelerator->ReleaseBuffer(m_DXVA1BufferInfo[i].dwTypeIndex, m_DXVA1BufferInfo[i].dwBufferIndex);

			ASSERT(SUCCEEDED(hr2));
		}

		m_dwNumBuffersInfo = 0;

		break;

	case ENGINE_DXVA2:

		for(DWORD i = 0; i < m_ExecuteParams.NumCompBuffers; i++)
		{
			HRESULT hr2 = m_pDirectXVideoDec->ReleaseBuffer(m_ExecuteParams.pCompressedBuffers[i].CompressedBufferType);

			ASSERT(SUCCEEDED(hr2));
		}

		hr = m_pDirectXVideoDec->Execute(&m_ExecuteParams);

		m_ExecuteParams.NumCompBuffers	= 0;

		break;
	}

	return hr;
}

HRESULT CDXVADecoder::QueryStatus(void* pDXVAStatus, UINT nSize)
{
	HRESULT hr = E_INVALIDARG;

	DXVA2_DecodeExecuteParams ExecuteParams;
	DXVA2_DecodeExtensionData ExtensionData;

	switch (m_nEngine)
	{
	case ENGINE_DXVA1:

		hr = m_pAMVideoAccelerator->Execute(0x07000000, NULL, 0, pDXVAStatus, nSize, 0, NULL);

		break;

	case ENGINE_DXVA2:

		memset(&ExecuteParams, 0, sizeof(ExecuteParams));
		memset(&ExtensionData, 0, sizeof(ExtensionData));
		ExecuteParams.pExtensionData = &ExtensionData;
		ExtensionData.pPrivateOutputData = pDXVAStatus;
		ExtensionData.PrivateOutputDataSize = nSize;
		ExtensionData.Function = 7;
		
		hr = m_pDirectXVideoDec->Execute(&ExecuteParams);

		break;
	}

	return hr;
}

DWORD CDXVADecoder::GetDXVA1CompressedType(DWORD dwDXVA2CompressedType)
{
	if(dwDXVA2CompressedType <= DXVA2_BitStreamDateBufferType)
	{
		return dwDXVA2CompressedType + 1;
	}
	else
	{
		switch(dwDXVA2CompressedType)
		{
		case DXVA2_MotionVectorBuffer:
			return DXVA_MOTION_VECTOR_BUFFER;
		case DXVA2_FilmGrainBuffer:
			return DXVA_FILM_GRAIN_BUFFER;
		default:
			ASSERT(0);
			return DXVA_COMPBUFFER_TYPE_THAT_IS_NOT_USED;
		}
	}
}

HRESULT CDXVADecoder::FindFreeDXVA1Buffer(DWORD dwTypeIndex, DWORD& dwBufferIndex)
{
	HRESULT hr;

	dwBufferIndex = 0; //(dwBufferIndex + 1) % m_ComBufferInfo[DXVA_PICTURE_DECODE_BUFFER].dwNumCompBuffers;

	DO_DXVA_PENDING_LOOP(m_pAMVideoAccelerator->QueryRenderStatus(-1, dwBufferIndex, 0));
	
	return hr;
}

HRESULT CDXVADecoder::BeginFrame(int nSurfaceIndex, IMediaSample* pSampleToDeliver)
{
	HRESULT hr = E_INVALIDARG;

	for(int i = 0; i < 20; i++)
	{
		switch(m_nEngine)
		{
		case ENGINE_DXVA1:

			AMVABeginFrameInfo BeginFrameInfo;

			BeginFrameInfo.dwDestSurfaceIndex = nSurfaceIndex;
			BeginFrameInfo.dwSizeInputData = sizeof(nSurfaceIndex);
			BeginFrameInfo.pInputData = &nSurfaceIndex;
			BeginFrameInfo.dwSizeOutputData = 0;
			BeginFrameInfo.pOutputData = NULL;

			DO_DXVA_PENDING_LOOP(m_pAMVideoAccelerator->BeginFrame(&BeginFrameInfo));

			ASSERT(SUCCEEDED(hr));

			if(SUCCEEDED(hr))
			{
				hr = FindFreeDXVA1Buffer(-1, m_dwBufferIndex);
			}

			break;

		case ENGINE_DXVA2:

			if(CComQIPtr<IMFGetService> pSampleService = pSampleToDeliver)
			{
				CComPtr<IDirect3DSurface9> pDecoderRenderTarget;

				hr = pSampleService->GetService(MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&pDecoderRenderTarget);

				if(SUCCEEDED(hr))
				{
					DO_DXVA_PENDING_LOOP(m_pDirectXVideoDec->BeginFrame(pDecoderRenderTarget, NULL));
				}
			}

			break;
		}

		// For slow accelerator wait a little...

		if(SUCCEEDED(hr)) break;

		Sleep(1);
	}

	return hr;
}

HRESULT CDXVADecoder::EndFrame(int nSurfaceIndex)
{
	HRESULT hr = E_INVALIDARG;

	DWORD dwDummy = nSurfaceIndex;

	switch(m_nEngine)
	{
	case ENGINE_DXVA1:

		AMVAEndFrameInfo EndFrameInfo;

		EndFrameInfo.dwSizeMiscData = sizeof(dwDummy); // TODO : usefull ??
		EndFrameInfo.pMiscData = &dwDummy;

		hr = m_pAMVideoAccelerator->EndFrame(&EndFrameInfo);

		ASSERT(SUCCEEDED(hr));

		break;

	case ENGINE_DXVA2:

		hr = m_pDirectXVideoDec->EndFrame(NULL);

		break;

	default:
		
		ASSERT(0);

		break;
	}

	return hr;
}

bool CDXVADecoder::AddToStore(int nSurfaceIndex, IMediaSample* pSample, bool bRefPicture, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool bIsField, FF_FIELD_TYPE nFieldType, FF_SLICE_TYPE nSliceType, int nCodecSpecific)
{
	PICTURE_STORE& p = m_pPictureStore[nSurfaceIndex];

	if(bIsField && m_nFieldSurface == -1)
	{
		m_nFieldSurface = nSurfaceIndex;
		m_pFieldSample = pSample;

		p.n1FieldType = nFieldType;
		p.rtStart = rtStart;
		p.rtStop = rtStop;
		p.nCodecSpecific = nCodecSpecific;

		return false;
	}

	ASSERT(p.pSample == NULL);
	ASSERT(!p.bInUse);
	ASSERT(nSurfaceIndex < m_nPicEntryNumber && p.pSample == NULL);

	p.bRefPicture = bRefPicture;
	p.bInUse = true;
	p.bDisplayed = false;
	p.pSample = pSample;
	p.nSliceType = nSliceType;

	if(!bIsField)
	{
		p.rtStart = rtStart;
		p.rtStop = rtStop;
		p.n1FieldType = nFieldType;
		p.nCodecSpecific = nCodecSpecific;
	}

	m_nFieldSurface	= -1;
	m_nWaitingPics++;

	return true;
}

void CDXVADecoder::UpdateStore(int nSurfaceIndex, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	PICTURE_STORE& p = m_pPictureStore[nSurfaceIndex];

	ASSERT(nSurfaceIndex < m_nPicEntryNumber && p.bInUse && !p.bDisplayed);

	p.rtStart = rtStart;
	p.rtStop = rtStop;
}

void CDXVADecoder::RemoveRefFrame(int nSurfaceIndex)
{
	PICTURE_STORE& p = m_pPictureStore[nSurfaceIndex];

	ASSERT(nSurfaceIndex < m_nPicEntryNumber && p.bInUse);

	p.bRefPicture = false;
	
	if(p.bDisplayed)
	{
		FreePictureSlot(nSurfaceIndex);
	}
}

int CDXVADecoder::FindOldestFrame()
{
	// TODO : find better solution...

	REFERENCE_TIME rtMin = _I64_MAX;
	int nPos = -1;

	if(m_nWaitingPics > m_nMaxWaiting)
	{
		for(int i=0; i<m_nPicEntryNumber; i++)
		{
			PICTURE_STORE& p = m_pPictureStore[i];

			if(!p.bDisplayed && p.bInUse && p.rtStart < rtMin)
			{
				rtMin = p.rtStart;
				nPos = i;
			}
		}
	}

	return nPos;
}

void CDXVADecoder::SetTypeSpecificFlags(PICTURE_STORE* pPicture, IMediaSample* pMS)
{
	if(CComQIPtr<IMediaSample2> pMS2 = pMS)
	{
		AM_SAMPLE2_PROPERTIES props;

		if(SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props)))
		{
			props.dwTypeSpecificFlags &= ~0x7f;

			if(pPicture->n1FieldType == PICT_FRAME)
			{
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_WEAVE;
			}
			else
			{
				if(pPicture->n1FieldType == PICT_TOP_FIELD)
				{
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_FIELD1FIRST;
				}

				//if(m_fb.flags & PIC_FLAG_REPEAT_FIRST_FIELD)
				//	props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_REPEAT_FIELD;
			}

			switch(pPicture->nSliceType)
			{
			case I_TYPE:
			case SI_TYPE:
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_I_SAMPLE;
				break;
			case P_TYPE:
			case SP_TYPE:
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_P_SAMPLE;
				break;
			default:
				props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_B_SAMPLE;
				break;
			}

			pMS2->SetProperties(sizeof(props), (BYTE*)&props);
		}
	}

	pMS->SetTime(&pPicture->rtStart, &pPicture->rtStop);
}

HRESULT CDXVADecoder::DisplayNextFrame()
{
	HRESULT hr = S_FALSE;
	CComPtr<IMediaSample> pSampleToDeliver;
	int nPicIndex;

	if((nPicIndex = FindOldestFrame()) != -1)
	// while((nPicIndex = FindOldestFrame()) != -1)
	{
		PICTURE_STORE& p = m_pPictureStore[nPicIndex];

		if(p.rtStart >= 0)
		{
			switch(m_nEngine)
			{
			case ENGINE_DXVA1: // For DXVA1, query a media sample at the last time (only one in the allocator)
				hr = GetDeliveryBuffer(p.rtStart, p.rtStop, &pSampleToDeliver);
				SetTypeSpecificFlags(&p, pSampleToDeliver);
				if(SUCCEEDED(hr)) hr = m_pAMVideoAccelerator->DisplayFrame(nPicIndex, pSampleToDeliver);
				break;
			case ENGINE_DXVA2: // For DXVA2 media sample is in the picture store
				SetTypeSpecificFlags(&p, p.pSample);
				hr = m_pFilter->GetOutputPin()->Deliver(p.pSample);
				break;
			}
		}

		p.bDisplayed = true;
	}

	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i];

		if(p.bInUse)
		{
			if(p.bDisplayed && !p.bRefPicture)
			{
				FreePictureSlot(i);
			}
		}
	}

	return hr;
}

HRESULT CDXVADecoder::GetFreeSurfaceIndex(int& nSurfaceIndex, IMediaSample** ppSampleToDeliver, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT hr = E_UNEXPECTED;
	int nPos = -1;
	DWORD dwMinDisplay = MAXDWORD;

	if(m_nFieldSurface != -1)
	{
		nSurfaceIndex = m_nFieldSurface;
		*ppSampleToDeliver = m_pFieldSample.Detach();

		return S_FALSE;
	}

	CComPtr<IMediaSample> pNewSample;

	switch(m_nEngine)
	{
	case ENGINE_DXVA1:

		for(int i = 0; i < m_nPicEntryNumber; i++)
		{
			PICTURE_STORE& p = m_pPictureStore[i];

			if(!p.bInUse && p.dwDisplayCount < dwMinDisplay)
			{
				dwMinDisplay = p.dwDisplayCount;
				nSurfaceIndex = i;

				return S_OK;
			}
		}

		// Ho ho... 

		ASSERT(0);

		Flush();

		break;

	case ENGINE_DXVA2:

		// TODO : test  IDirect3DDeviceManager9::TestDevice !!!

		if(SUCCEEDED(hr = GetDeliveryBuffer(rtStart, rtStop, &pNewSample)))
		{
			if(CComQIPtr<IDXVA2Sample> pDXVA2Sample = pNewSample)
			{
				nSurfaceIndex = pDXVA2Sample ? pDXVA2Sample->GetDXSurfaceId() : 0;
			}

			*ppSampleToDeliver = pNewSample.Detach();
		}

		break;
	}

	return hr;
}

void CDXVADecoder::FreePictureSlot(int nSurfaceIndex)
{
	PICTURE_STORE& p = m_pPictureStore[nSurfaceIndex];

	p.dwDisplayCount = m_dwDisplayCount++;
	p.bInUse = false;
	p.bDisplayed = false;
	p.pSample = NULL;
	p.nCodecSpecific = -1;

	m_nWaitingPics--;
}

BYTE CDXVADecoder::GetConfigResidDiffAccelerator()
{
	switch(m_nEngine)
	{
	case ENGINE_DXVA1:
		return m_DXVA1Config.bConfigResidDiffAccelerator;
	case ENGINE_DXVA2:
		return m_DXVA2Config.ConfigResidDiffAccelerator;
	}

	return 0;
}

BYTE CDXVADecoder::GetConfigIntraResidUnsigned()
{
	switch(m_nEngine)
	{
	case ENGINE_DXVA1:
		return m_DXVA1Config.bConfigIntraResidUnsigned;
	case ENGINE_DXVA2:
		return m_DXVA2Config.ConfigIntraResidUnsigned;
	}

	return 0;
}
