#pragma once

#include <dxva.h>
#include "DXVADecoder.h"

#define H264_MAX_SLICES 16 // Also define in ffmpeg!

class CDXVADecoderH264 : public CDXVADecoder
{
	DXVA_PicParams_H264 m_DXVAPicParams;
	DXVA_Qmatrix_H264 m_DXVAScalingMatrix;
	DXVA_Slice_H264_Short m_pSliceShort[H264_MAX_SLICES];
	DXVA_Slice_H264_Long m_pSliceLong[H264_MAX_SLICES]; 
	UINT m_nMaxSlices;
	int	m_nNALLength;
	int m_nOutPOC;
	bool m_bUseLongSlice;
	REFERENCE_TIME m_rtOutStart;
	REFERENCE_TIME m_rtOutStop;

	// Private functions
	
	void Init();
	HRESULT DisplayStatus();

	// DXVA functions

	void RemoveUndisplayedFrame(int nPOC);
	void ClearRefFramesList();
	void ClearUnusedRefFrames();

protected:
	virtual int FindOldestFrame();

public:
	CDXVADecoderH264(CDXVADecoderFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderH264(CDXVADecoderFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderH264();

	virtual HRESULT DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void SetExtraData(BYTE* pDataIn, UINT nSize);
	virtual void CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void Flush();
};
