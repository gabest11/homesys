#pragma once

typedef enum
{
	NALU_TYPE_SLICE    = 1,
	NALU_TYPE_DPA      = 2,
	NALU_TYPE_DPB      = 3,
	NALU_TYPE_DPC      = 4,
	NALU_TYPE_IDR      = 5,
	NALU_TYPE_SEI      = 6,
	NALU_TYPE_SPS      = 7,
	NALU_TYPE_PPS      = 8,
	NALU_TYPE_AUD      = 9,
	NALU_TYPE_EOSEQ    = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL     = 12
} NALU_TYPE;

class CH264Nalu
{
	int	forbidden_bit;      	//! should be always FALSE
	int	nal_reference_idc;  	//! NALU_PRIORITY_xxxx
	NALU_TYPE nal_unit_type;    //! NALU_TYPE_xxxx    

	DWORD m_nNALStartPos;			//! NALU start (including startcode / size)
	DWORD m_nNALDataPos;			//! Useful part
	DWORD m_nDataLen;	//! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)

	BYTE* m_pBuffer;
	DWORD m_nCurPos;
	DWORD m_nSize;
	DWORD m_nNALSize;

	int GetRef() { return nal_reference_idc; };
	bool IsRefFrame() { return (nal_reference_idc != 0); };

public:
	void SetBuffer(BYTE* pBuffer, DWORD nSize, DWORD nNALSize);
	bool ReadNext();

	NALU_TYPE GetType() { return nal_unit_type; };
	DWORD GetDataLength() { return m_nCurPos - m_nNALDataPos; };
	BYTE* GetDataBuffer() { return m_pBuffer + m_nNALDataPos; };
	DWORD GetStartPos() { return m_nNALStartPos; }
	DWORD GetDataPos() { return m_nNALDataPos; }

	void ParseAll(std::vector<std::pair<NALU_TYPE, DWORD>>& nalus);
};
