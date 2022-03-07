#include "stdafx.h"
#include "DXVADecoderMpeg2.h"
#include "DXVADecoderFilter.h"
#include "../3rdparty/ffmpeg/ffmpeg.h"

CDXVADecoderMpeg2::CDXVADecoderMpeg2(CDXVADecoderFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	Init();
}

CDXVADecoderMpeg2::CDXVADecoderMpeg2 (CDXVADecoderFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, nMode, nPicEntryNumber, pDXVA2Config)
{
	Init();
}

CDXVADecoderMpeg2::~CDXVADecoderMpeg2()
{
	Flush();
}

void CDXVADecoderMpeg2::Init()
{
	memset(&m_PictureParams, 0, sizeof(m_PictureParams));
	memset(&m_SliceInfo, 0, sizeof(m_SliceInfo));
	memset(&m_QMatrixData, 0, sizeof(m_QMatrixData));

	m_nMaxWaiting = 5;
	m_wRefPictureIndex[0] = NO_REF_FRAME;
	m_wRefPictureIndex[1] = NO_REF_FRAME;
	m_nSliceCount = 0;
	m_rtLastStart = 0;
	m_nCountEstimated = 0;

	m_start = _I64_MIN;
	m_stop = _I64_MIN;
	m_buff_size = 0;

	switch(GetMode())
	{
	case MPEG2_VLD:
		AllocExecuteParams(4);
		break;
	default:
		ASSERT(0);
		break;
	}
}

HRESULT CDXVADecoderMpeg2::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT hr = S_FALSE;

	{
		int size = m_buff_size + nSize;

		if(size > m_buff.size())
		{
			m_buff.resize(size);
		}

		memcpy(m_buff.data() + m_buff_size, pDataIn, nSize);

		m_buff_size = size;
	}

	if(m_buff_size > 0)
	{
		std::vector<int> pos;
		std::list<std::pair<int, int>> pos2;

		DWORD c = 0xffffffff;

		int i = 0;

		while(i < m_buff_size)
		{
			c = (c << 8) | m_buff[i++];

			if(c == 0x00000100 || c == 0x000001b3 || c == 0x000001b7)
			{
				pos.push_back(i - 4);
			}

			if((c & 0xffffff00) == 0x00000100)
			{
				pos2.push_back(std::pair<int, int>(i - 4, c & 0xff));
			}
		}

		if(pos.empty())
		{
			m_buff_size = 0;

			return S_FALSE;
		}

		for(int i = 0; i < pos.size() - 1; i++)
		{
			hr = DecodeFrameInternal(&m_buff[pos[i]], pos[i + 1] - pos[i], m_start, m_stop);

			m_start = rtStart;
			m_stop = rtStop;

			rtStart = _I64_MIN;
			rtStop = _I64_MIN;
		}

		int last = pos.back();

		if(last > 0)
		{
			memmove(&m_buff[0], &m_buff[last], m_buff_size - last);

			m_buff_size -= last;
		}
	}

	return hr;
}

HRESULT CDXVADecoderMpeg2::DecodeFrameInternal(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT hr;

	int nFieldType = 0;
	int nSliceType = 0;

	FFMpeg2DecodeFrame(&m_PictureParams, &m_QMatrixData, m_SliceInfo, &m_nSliceCount, m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), &m_nNextCodecIndex, &nFieldType, &nSliceType, pDataIn, nSize);

	ASSERT(m_nSliceCount <= MPEG2_MAX_SLICES * 2);

	// Wait I frame after a flush

	if(m_bFlushed && !m_PictureParams.bPicIntra)
	{
		return S_FALSE;
	}

	if(*(DWORD*)pDataIn == 0xb3010000)
	{
		return S_FALSE;
	}

	int nSurfaceIndex;
	CComPtr<IMediaSample> pSampleToDeliver;

	hr = GetFreeSurfaceIndex(nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop);

	if(FAILED(hr))
	{
		ASSERT(hr == VFW_E_NOT_COMMITTED); // Normal when stop playing

		return hr;
	}

	hr = BeginFrame(nSurfaceIndex, pSampleToDeliver);

	if(FAILED(hr))
	{
		return hr;
	}

	UpdatePictureParams(nSurfaceIndex);

	hr = AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(m_QMatrixData), &m_QMatrixData);

	if(FAILED(hr))
	{
		return hr;
	}

	// Send bitstream to accelerator

	hr = AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_SliceInfo) * m_nSliceCount, &m_SliceInfo);

	if(FAILED(hr))
	{
		return hr;
	}
	
	hr = AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize, pDataIn, &nSize);

	if(FAILED(hr))
	{
		return hr;
	}

	// Decode frame

	hr = Execute();

	if(FAILED(hr))
	{
		return hr;
	}
	
	hr = EndFrame(nSurfaceIndex);

	if(FAILED(hr))
	{
		return hr;
	}

	AddToStore(nSurfaceIndex, pSampleToDeliver, m_PictureParams.bPicBackwardPrediction != 1, rtStart, rtStop, false, (FF_FIELD_TYPE)nFieldType, (FF_SLICE_TYPE)nSliceType, FFGetCodedPicture(m_pFilter->GetAVCtx()));

	m_bFlushed = false;

	return DisplayNextFrame();
}

void CDXVADecoderMpeg2::UpdatePictureParams(int nSurfaceIndex)
{
	DXVA2_ConfigPictureDecode* cpd = GetDXVA2Config(); // Ok for DXVA1 too (parameters have been copied)

	m_PictureParams.wDecodedPictureIndex = nSurfaceIndex;

	// Manage reference picture list

	if(!m_PictureParams.bPicBackwardPrediction)
	{
		if(m_wRefPictureIndex[0] != NO_REF_FRAME) 
		{
			RemoveRefFrame(m_wRefPictureIndex[0]);
		}

		m_wRefPictureIndex[0] = m_wRefPictureIndex[1];
		m_wRefPictureIndex[1] = nSurfaceIndex;
	}

	m_PictureParams.wForwardRefPictureIndex = m_PictureParams.bPicIntra == 0 ? m_wRefPictureIndex[0] : NO_REF_FRAME;
	m_PictureParams.wBackwardRefPictureIndex = m_PictureParams.bPicBackwardPrediction == 1 ? m_wRefPictureIndex[1] : NO_REF_FRAME;

	// Shall be 0 if bConfigResidDiffHost is 0 or if BPP > 8

	if(cpd->ConfigResidDiffHost == 0 || m_PictureParams.bBPPminus1 > 7)
	{
		m_PictureParams.bPicSpatialResid8 = 0;
	}
	else
	{
		if(m_PictureParams.bBPPminus1 == 7 && m_PictureParams.bPicIntra && cpd->ConfigResidDiffHost)
		{
			// Shall be 1 if BPP is 8 and bPicIntra is 1 and bConfigResidDiffHost is 1

			m_PictureParams.bPicSpatialResid8 = 1;
		}
		else
		{
			// Shall be 1 if bConfigSpatialResid8 is 1

			m_PictureParams.bPicSpatialResid8 = cpd->ConfigSpatialResid8;
		}
	}

	// Shall be 0 if bConfigResidDiffHost is 0 or if bConfigSpatialResid8 is 0 or if BPP > 8

	if(cpd->ConfigResidDiffHost == 0 || cpd->ConfigSpatialResid8 == 0 || m_PictureParams.bBPPminus1 > 7)
	{
		m_PictureParams.bPicOverflowBlocks = 0;
	}

	// Shall be 1 if bConfigHostInverseScan is 1 or if bConfigResidDiffAccelerator is 0.

	if(cpd->ConfigHostInverseScan == 1 || cpd->ConfigResidDiffAccelerator == 0)
	{
		m_PictureParams.bPicScanFixed	= 1;

		if(cpd->ConfigHostInverseScan != 0)
		{
			m_PictureParams.bPicScanMethod	= 3; // 11 = Arbitrary scan with absolute coefficient address.
		}
		else if(FFGetAlternateScan(m_pFilter->GetAVCtx()))
		{
			m_PictureParams.bPicScanMethod	= 1; // 00 = Zig-zag scan (MPEG-2 Figure 7-2)
		}
		else
		{
			m_PictureParams.bPicScanMethod	= 0; // 01 = Alternate-vertical (MPEG-2 Figure 7-3),
		}
	}
}

void CDXVADecoderMpeg2::SetExtraData(BYTE* pDataIn, UINT nSize)
{
	m_PictureParams.wPicWidthInMBminus1 = m_pFilter->PictWidth()  - 1;
	m_PictureParams.wPicHeightInMBminus1 = m_pFilter->PictHeight() - 1;
}

void CDXVADecoderMpeg2::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	while(*((DWORD*)pBuffer) != 0x01010000)
	{
		pBuffer++;
		nSize--;

		if(nSize <= 0) return;
	}

	memcpy(pDXVABuffer, pBuffer, nSize);
}

void CDXVADecoderMpeg2::Flush()
{
	m_nNextCodecIndex = INT_MIN;

	if(m_wRefPictureIndex[0] != NO_REF_FRAME) RemoveRefFrame(m_wRefPictureIndex[0]);
	if(m_wRefPictureIndex[1] != NO_REF_FRAME) RemoveRefFrame(m_wRefPictureIndex[1]);

	m_wRefPictureIndex[0] = NO_REF_FRAME;
	m_wRefPictureIndex[1] = NO_REF_FRAME;

	m_rtLastStart = 0;
	m_nCountEstimated = 0;

	m_start = _I64_MIN;
	m_stop = _I64_MIN;
	m_buff_size = 0;
	m_buff.clear();

	__super::Flush();
}

int CDXVADecoderMpeg2::FindOldestFrame()
{
	/*
	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i];

		if(p.bInUse && !p.bDisplayed)
		{
			if(p.rtStart < m_rtLastStart)
			{
				wprintf(L"*********************** %I64d %d\n", p.rtStart / 10000, p.nSliceType);

				p.bDisplayed = true;
			}
		}
	}
	*/
	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i];

		if(!p.bDisplayed && p.bInUse && p.nCodecSpecific == m_nNextCodecIndex)
		{
			m_nNextCodecIndex = INT_MIN;

			REFERENCE_TIME atpf = m_pFilter->GetAvrTimePerFrame();

			if(p.rtStart == _I64_MIN)
			{
				// If reference time has not been set by splitter, extrapolate start time
				// from last known start time already delivered

				p.rtStart = m_rtLastStart + atpf * m_nCountEstimated;
				m_nCountEstimated++;
			}
			else
			{
				// Known start time, set as new reference

				m_rtLastStart = p.rtStart;
				m_nCountEstimated = 1;
			}

			p.rtStop = p.rtStart + atpf;
/*
			static REFERENCE_TIME rtLast = 0;
			wprintf(L"p %I64d %d (%d)\n", p.rtStart / 10000, p.nSliceType, (p.rtStart - rtLast) / 10000);
			rtLast = p.rtStart;
*/
			return i;
		}
	}

	return -1;
}
