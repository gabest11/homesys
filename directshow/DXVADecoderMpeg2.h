#pragma once

#include <dxva.h>
#include "DXVADecoder.h"

#define MPEG2_MAX_SLICES 175 // Max slice number for Mpeg2 streams

class CDXVADecoderMpeg2 : public CDXVADecoder
{
	DXVA_PictureParameters m_PictureParams;
	DXVA_QmatrixData m_QMatrixData;	
	WORD m_wRefPictureIndex[2];
	DXVA_SliceInfo m_SliceInfo[MPEG2_MAX_SLICES * 2];
	int m_nSliceCount;
	int m_nNextCodecIndex;
	REFERENCE_TIME m_rtLastStart; // rtStart for last delivered frame
	int m_nCountEstimated; // Number of rtStart estimated since last rtStart received
	
	void Init();
	void UpdatePictureParams(int nSurfaceIndex);

	REFERENCE_TIME m_start;
	REFERENCE_TIME m_stop;
	std::vector<BYTE> m_buff;
	int m_buff_size;

	HRESULT DecodeFrameInternal(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);

protected:
	virtual int FindOldestFrame();

public:
	CDXVADecoderMpeg2(CDXVADecoderFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderMpeg2(CDXVADecoderFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderMpeg2();

	virtual HRESULT DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void SetExtraData(BYTE* pDataIn, UINT nSize);
	virtual void CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void Flush();
};
