#include "stdafx.h"
#include "DXVADecoderH264.h"
#include "DXVADecoderFilter.h"
#include "DXVADecoderAllocator.h"
#include "H264Nalu.h"
#include "../3rdparty/ffmpeg/ffmpeg.h"

CDXVADecoderH264::CDXVADecoderH264(CDXVADecoderFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	m_bUseLongSlice = GetDXVA1Config()->bConfigBitstreamRaw != 2;

	Init();
}

CDXVADecoderH264::CDXVADecoderH264(CDXVADecoderFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, nMode, nPicEntryNumber, pDXVA2Config)
{
	m_bUseLongSlice = pDXVA2Config->ConfigBitstreamRaw != 2;

	Init();
}

CDXVADecoderH264::~CDXVADecoderH264()
{
}

void CDXVADecoderH264::Init()
{
	memset(&m_DXVAPicParams, 0, sizeof(m_DXVAPicParams));
	memset(&m_pSliceShort, 0, sizeof(m_pSliceShort));
	memset(&m_pSliceLong, 0, sizeof(m_pSliceLong));
	
	m_DXVAPicParams.MbsConsecutiveFlag = 1;
	m_DXVAPicParams.ContinuationFlag = 1;
	m_DXVAPicParams.MinLumaBipredSize8x8Flag = 1;	// Improve accelerator performances
	
	if(m_pFilter->GetPCIVendor() == PCIV_Intel)
	{
		m_DXVAPicParams.Reserved16Bits = 0x34c;
	}

	for(int i = 0; i < 16; i++)
	{
		m_DXVAPicParams.RefFrameList[i].bPicEntry = 255;
	}

	m_nNALLength = 4;
	m_nMaxSlices = 0;

	switch(GetMode())
	{
	case H264_VLD:
		AllocExecuteParams(3);
		break;
	default:
		ASSERT(0);
	}
}

void CDXVADecoderH264::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	CH264Nalu Nalu;

	Nalu.SetBuffer(pBuffer, nSize, m_nNALLength);

	int	nSlices = 0;
	int	nDxvaNalLength;

	nSize = 0;

	while(Nalu.ReadNext())
	{
		switch(Nalu.GetType())
		{
		case NALU_TYPE_SLICE:
		case NALU_TYPE_IDR:
			
			// For AVC1, put startcode 0x000001

			pDXVABuffer[0] = pDXVABuffer[1] = 0; pDXVABuffer[2] = 1;
			
			// Copy NALU

			memcpy(pDXVABuffer + 3, Nalu.GetDataBuffer(), Nalu.GetDataLength());
			
			// Update slice control buffer

			nDxvaNalLength = Nalu.GetDataLength() + 3;
			
			m_pSliceShort[nSlices].BSNALunitDataLocation = nSize;
			m_pSliceShort[nSlices].SliceBytesInBuffer = nDxvaNalLength;

			nSlices++;

			nSize += nDxvaNalLength;
			pDXVABuffer += nDxvaNalLength;

			break;
		}
	}

	// Complete with zero padding (buffer size should be a multiple of 128)

	int nDummy = 128 - (nSize & 127);

	memset(pDXVABuffer, 0, nDummy);
	m_pSliceShort[nSlices - 1].SliceBytesInBuffer += nDummy;
	nSize += nDummy;
}

void CDXVADecoderH264::Flush()
{
	ClearRefFramesList();

	m_DXVAPicParams.UsedForReferenceFlags = 0;

	m_nOutPOC = INT_MIN;
	m_rtOutStart = 0;
	m_rtOutStop = 0;

	__super::Flush();
}

// 
FILE* g_log = NULL; // _wfopen(L"c:\\h264-2.log", L"w");

HRESULT CDXVADecoderH264::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT hr = S_FALSE;
	UINT nSlices = 0;
	int	nSurfaceIndex;
	int	nFieldType;
	int	nSliceType;
	int	nFramePOC;
	CComPtr<IMediaSample> pSampleToDeliver;
	CComQIPtr<IDXVA2Sample> pDXVA2Sample;
	int nDXIndex = 0;
	UINT nNalOffset = 0;
	int	nOutPOC = INT_MIN;
	REFERENCE_TIME rtOutStart = 0;
	REFERENCE_TIME rtOutStop = 0;
	
	CH264Nalu Nalu;

	Nalu.SetBuffer(pDataIn, nSize, m_nNALLength); 

	std::vector<std::pair<NALU_TYPE, DWORD>> nalus;

	Nalu.ParseAll(nalus);

	/*
	{
		DWORD pos = 0;
		DWORD cur = 0;
		DWORD slice = 0;
		DWORD sei = 0;

		bool skip = false;

		for(size_t i = 0; i < nalus.size(); i++)
		{
			DWORD size = nalus[i].second + 4;

			switch(nalus[i].first)
			{
			case NALU_TYPE_SLICE:
			case NALU_TYPE_IDR:

				slice++;

				break;

			case NALU_TYPE_SEI:

				if(0)if(sei > 0 && slice > 0)
				{
					DecodeFrame(pDataIn + pos, cur - pos, rtStart, rtStop);

					pos = cur;

					skip = true;
				}
				
				sei++;

				break;
			}

			if(cur > pos)
			{
				DecodeFrame(pDataIn + pos, cur - pos, rtStart, rtStop);
				pos = cur;
				skip = true;
			}

			cur += size;
		}

		if(skip) return S_OK;
	}
	*/

	// printf("size = %d\n", nSize);

	for(size_t i = 0; i < nalus.size(); i++)
	{
		// printf("%d %d\n", nalus[i].first, nalus[i].second);

		switch(nalus[i].first)
		{
		case NALU_TYPE_SLICE:
		case NALU_TYPE_IDR:

			if(m_bUseLongSlice) 
			{
				m_pSliceLong[nSlices].BSNALunitDataLocation = nNalOffset;
				m_pSliceLong[nSlices].SliceBytesInBuffer = nalus[i].second + 3; //.GetRoundedDataLength();
				m_pSliceLong[nSlices].slice_id = nSlices;
				
				FF264UpdateRefFrameSliceLong(&m_DXVAPicParams, &m_pSliceLong[nSlices], m_pFilter->GetAVCtx(), m_pFilter->GetPCIVendor());

				if(nSlices > 0)
				{
					m_pSliceLong[nSlices - 1].NumMbsForSlice = m_pSliceLong[nSlices].first_mb_in_slice - m_pSliceLong[nSlices - 1].first_mb_in_slice;
					m_pSliceLong[nSlices].NumMbsForSlice = m_pSliceLong[nSlices - 1].NumMbsForSlice; // ?
				}
			}

			nNalOffset += nalus[i].second + 3;
			nSlices++;

			break;
		}
	}

	FFH264DecodeBuffer(m_pFilter->GetAVCtx(), pDataIn, nSize, &nFramePOC, &nOutPOC, &rtOutStart, &rtOutStop);

	if(nSlices == 0) 
	{
		return S_FALSE;
	}

	// If parsing fail (probably no PPS/SPS), continue anyway it may arrived later (happen on truncated streams)
	
	if(FAILED(FFH264BuildPicParams(&m_DXVAPicParams, &m_DXVAScalingMatrix, &nFieldType, &nSliceType, m_pFilter->GetAVCtx(), m_pFilter->GetPCIVendor())))
	{
		return S_FALSE;
	}

	// Wait I frame after a flush
	
	if(m_bFlushed && !m_DXVAPicParams.IntraPicFlag)
	{
		return S_FALSE;
	}

	if(g_log)
	{
		fwprintf(g_log, L"t \t%I64d %d\n", rtStart / 10000, nSliceType);
		fflush(g_log);
	}

	hr = GetFreeSurfaceIndex(nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop);

	if(FAILED(hr)) 
	{
		return hr;
	}

	FFH264SetCurrentPicture(nSurfaceIndex, &m_DXVAPicParams, m_pFilter->GetAVCtx());

	hr = BeginFrame(nSurfaceIndex, pSampleToDeliver);

	if(FAILED(hr)) return hr;
	
	// m_DXVAPicParams.Reserved16Bits = 3;
	m_DXVAPicParams.StatusReportFeedbackNumber++;

	// Send picture parameters

	hr = AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PicParams_H264), &m_DXVAPicParams);

	if(FAILED(hr)) return hr;

	hr = Execute();

	if(FAILED(hr)) return hr;

	// Add bitstream, slice control and quantization matrix

	hr = AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize, pDataIn, &nSize);

	if(FAILED(hr)) return hr;

	if(m_bUseLongSlice)
	{
		hr = AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Long) * nSlices, m_pSliceLong);
	}
	else
	{
		hr = AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short) * nSlices, m_pSliceShort);
	}

	if(FAILED(hr)) return hr;

	hr = AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), (void*)&m_DXVAScalingMatrix);

	if(FAILED(hr)) return hr;

	// Decode bitstream

	hr = Execute();

	if(FAILED(hr)) return hr;

	hr = EndFrame(nSurfaceIndex);

	if(FAILED(hr)) return hr;

	// DisplayStatus();

	bool bAdded = AddToStore(nSurfaceIndex, pSampleToDeliver, m_DXVAPicParams.RefPicFlag, rtStart, rtStop, m_DXVAPicParams.field_pic_flag, (FF_FIELD_TYPE)nFieldType, (FF_SLICE_TYPE)nSliceType, nFramePOC);

	FFH264UpdateRefFramesList(&m_DXVAPicParams, m_pFilter->GetAVCtx());

	ClearUnusedRefFrames();

	if(bAdded) 
	{
		if(g_log)
		{
			fwprintf(g_log, L"%d %d \n", nOutPOC, m_nOutPOC);
			fflush(g_log);
		}

		if(nOutPOC != INT_MIN)
		{
			m_nOutPOC = nOutPOC;
			m_rtOutStart = rtOutStart;
			m_rtOutStop = rtOutStop;
		}

		hr = DisplayNextFrame();
	}

	m_bFlushed = false;
	
	if(nOutPOC == INT_MIN)
	{
		if(m_nWaitingPics > 16) // > m_DXVAPicParams.num_ref_frames
		{
			avcodec_flush_buffers(m_pFilter->GetAVCtx());

			Flush();

//FILE* g_log = _wfopen(L"d:\\h264.log", L"w");
			if(g_log)
			{
				fwprintf(g_log, L"!!!\n");
				fflush(g_log);
//fclose(g_log);
			}
		}
	}

	return hr;
}

void CDXVADecoderH264::RemoveUndisplayedFrame(int nPOC)
{
	// Find frame with given POC, and free the slot

	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i];

		if(p.bInUse && p.nCodecSpecific == nPOC)
		{
			p.bDisplayed = true;

			RemoveRefFrame(i);

			break;
		}
	}
}

void CDXVADecoderH264::ClearUnusedRefFrames()
{
	// Remove old reference frames (not anymore a short or long ref frame)
	
	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i];

		if(p.bRefPicture && p.bDisplayed)
		{
			if(!FFH264IsRefFrameInUse(i, m_pFilter->GetAVCtx()))
			{
				RemoveRefFrame(i);
			}
		}
	}
}

void CDXVADecoderH264::SetExtraData(BYTE* pDataIn, UINT nSize)
{
	AVCodecContext* pAVCtx = m_pFilter->GetAVCtx();
	
	m_nNALLength = pAVCtx->nal_length_size;
	
	FFH264DecodeBuffer(pAVCtx, pDataIn, nSize, NULL, NULL, NULL, NULL);
	FFH264SetDxvaSliceLong(pAVCtx, m_pSliceLong);
}

void CDXVADecoderH264::ClearRefFramesList()
{
	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i];

		if(p.bInUse)
		{
			p.bDisplayed = true;
			
			RemoveRefFrame(i);
		}
	}
}

HRESULT CDXVADecoderH264::DisplayStatus()
{
	HRESULT hr;

	DXVA_Status_H264 status;

	memset(&status, 0, sizeof(status));

	if(SUCCEEDED(hr = CDXVADecoder::QueryStatus(&status, sizeof(status))))
	{
		printf("StatusReportFeedbackNumber = %d\n", status.StatusReportFeedbackNumber);
		printf("CurrPic.bPicEntry = %d\n", status.CurrPic.bPicEntry);
		printf("field_pic_flag = %d\n", status.field_pic_flag);
		printf("bDXVA_Func = %d\n", status.bDXVA_Func);
		printf("bBufType = %d\n", status.bBufType);
		printf("bStatus = %d\n", status.bStatus);
		printf("bReserved8Bits = %d\n", status.bReserved8Bits);
		printf("wNumMbsAffected = %d\n\n", status.wNumMbsAffected);
	}

	return hr;
}

int CDXVADecoderH264::FindOldestFrame()
{
	int nInUse = 0;

	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i];

		if(p.bInUse)
		{
			if(!p.bDisplayed && p.rtStart < m_rtOutStart)
			{
				p.bDisplayed = true;
			}

			nInUse++;
		}
	}

	for(int i = 0; i < m_nPicEntryNumber; i++)
	{
		PICTURE_STORE& p = m_pPictureStore[i]; 

		if(p.bInUse)
		{
			if(!p.bDisplayed && p.nCodecSpecific == m_nOutPOC)
			{
				p.rtStart = m_rtOutStart;
				p.rtStop = m_rtOutStart + m_pFilter->GetAvrTimePerFrame(); // m_rtOutStop

				return i;
			}
		}
	}

	if(g_log)
	{
		fwprintf(g_log, L"*** %d %d %I64d ***\n", nInUse, m_nOutPOC, m_rtOutStart / 10000);
		fflush(g_log);
	}

	if(m_nOutPOC == INT_MIN || m_rtOutStart < 0)
	{
		return __super::FindOldestFrame();
	}

	return -1;
}
