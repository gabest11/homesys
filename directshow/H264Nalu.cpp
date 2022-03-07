#include "StdAfx.h"
#include "H264Nalu.h"

void CH264Nalu::SetBuffer(BYTE* pBuffer, DWORD nSize, DWORD nNALSize)
{
	m_pBuffer = pBuffer;
	m_nSize = nSize;
	m_nNALSize = nNALSize;
	m_nCurPos = 0;
	m_nNALStartPos = 0;
	m_nNALDataPos = 0;
}

bool CH264Nalu::ReadNext()
{
	if(m_nCurPos + (m_nNALSize > 0 ? m_nNALSize : 0) >= m_nSize) 
	{
		return false;
	}

	m_nNALStartPos = m_nCurPos;

	if(m_nNALSize != 0)
	{
		// RTP Nalu type : (XX XX) XX XX NAL..., with XX XX XX XX or XX XX equal to NAL size
		
		m_nNALDataPos = m_nCurPos + m_nNALSize;

		DWORD nSize = 0;

		for(DWORD i = 0; i < m_nNALSize; i++)
		{
			nSize = (nSize << 8) + m_pBuffer[m_nCurPos++];
		}

		m_nCurPos += nSize;

		if(m_nCurPos > m_nSize)
		{
			printf("bogus h264 data\n");

			m_nCurPos = m_nSize;
		}
	}
	else
	{
		// AnnexB Nalu : 00 00 01 NAL...

		DWORD nBuffEnd = m_nSize - 3;

		while(m_nCurPos < nBuffEnd && m_pBuffer[m_nCurPos] == 0 && (*((DWORD*)(m_pBuffer + m_nCurPos)) & 0xFFFFFF) != 0x010000)
		{
			m_nCurPos++;
		}

		m_nCurPos += 3;
		m_nNALDataPos = m_nCurPos;

		DWORD nZeros = 0;

		while(m_nCurPos < nBuffEnd && (*((DWORD*)(m_pBuffer + m_nCurPos)) & 0xFFFFFF) != 0x010000)
		{
			nZeros = m_pBuffer[m_nCurPos] == 0 ? nZeros + 1 : 0;

			m_nCurPos++;
		}

		if(m_nCurPos == nBuffEnd)
		{
			m_nCurPos = m_nSize;
		}
		else
		{
			m_nCurPos -= nZeros;
		}
	}

	ASSERT(m_nNALDataPos < m_nSize);

	forbidden_bit = (m_pBuffer[m_nNALDataPos] >> 7) & 1;
	nal_reference_idc = (m_pBuffer[m_nNALDataPos] >> 5) & 3;
	nal_unit_type = (NALU_TYPE)(m_pBuffer[m_nNALDataPos] & 0x1f);

	return true;
}

void CH264Nalu::ParseAll(std::vector<std::pair<NALU_TYPE, DWORD>>& nalus)
{
	nalus.clear();

	nalus.reserve(4);

	while(ReadNext())
	{
		nalus.push_back(std::make_pair<NALU_TYPE, DWORD>(GetType(), GetDataLength()));
	}
}
