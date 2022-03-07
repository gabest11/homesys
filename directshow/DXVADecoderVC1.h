#pragma once

#include <dxva.h>
#include "DXVADecoder.h"

class CDXVADecoderVC1 :	public CDXVADecoder
{
	DXVA_PictureParameters m_PictureParams;
	DXVA_SliceInfo m_SliceInfo;
	WORD m_wRefPictureIndex[2];

	int m_nDelayedSurfaceIndex;
	REFERENCE_TIME m_rtStartDelayed;
	REFERENCE_TIME m_rtStopDelayed;

	void Init();
	HRESULT DisplayStatus();
	BYTE*FindNextStartCode(BYTE* pBuffer, UINT nSize, UINT& nPacketSize);

public:
	CDXVADecoderVC1(CDXVADecoderFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderVC1(CDXVADecoderFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderVC1();

	virtual HRESULT DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void SetExtraData(BYTE* pDataIn, UINT nSize);
	virtual void CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void Flush();

	enum
	{
		VC1_PS_TOP_FIELD = 1,
		VC1_PS_BOTTOM_FIELD = 2,
		VC1_PS_PROGRESSIVE = 3
	};

	enum VC1_CHROMA_FORMAT
	{
		VC1_CHROMA_420 = 1,
		VC1_CHROMA_422 = 2,
		VC1_CHROMA_444 = 3
	};

	enum
	{
		VC1_CR_BICUBIC_QUARTER_CHROMA = 4,
		VC1_CR_BICUBIC_HALF_CHROMA = 5,
		VC1_CR_BILINEAR_QUARTER_CHROMA = 12,
		VC1_CR_BILINEAR_HALF_CHROMA = 13,
	};

	enum VC1_PIC_SCAN_METHOD
	{
		VC1_SCAN_ZIGZAG = 0,
		VC1_SCAN_ALTERNATE_VERTICAL = 1,
		VC1_SCAN_ALTERNATE_HORIZONTAL = 2,
		VC1_SCAN_ARBITRARY = 3 // Use when bConfigHostInverseScan = 1
	};

	enum VC1_DEBLOCK_CONFINED // Values for bPicDeblockConfined when bConfigBitstreamRaw = 1
	{
		VC1_EXTENDED_DMV = 0x0001,
		VC1_PSF = 0x0002,
		VC1_REFPICFLAG = 0x0004,
		VC1_FINTERPFLAG = 0x0008,
		VC1_TFCNTRFLAG = 0x0010,
		VC1_INTERLACE = 0x0020,
		VC1_PULLDOWN = 0x0040,
		VC1_POSTPROCFLAG = 0x0080
	} ;

	enum VC1_PIC_SPATIAL_RESID8 // Values for bPicSpatialResid8
	{
		VC1_VSTRANSFORM = 0x0001,
		VC1_DQUANT = 0x0002,
		VC1_EXTENDED_MV = 0x0004,
		VC1_FASTUVMC = 0x0008,
		VC1_LOOPFILTER = 0x0010,
		VC1_REDIST_FLAG = 0x0020,
		VC1_PANSCAN_FLAG = 0x0040,
	};
};
