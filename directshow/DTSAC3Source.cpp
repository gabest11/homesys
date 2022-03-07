/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DTSAC3Source.h"
#include "DirectShow.h"
#include "AsyncReader.h"
#include <initguid.h>
#include "moreuuids.h"

// CDTSAC3Source

CDTSAC3Source::CDTSAC3Source(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseSource<CDTSAC3Stream>(NAME("CDTSAC3Source"), lpunk, phr, __uuidof(this))
{
}

CDTSAC3Source::~CDTSAC3Source()
{
}

// CDTSAC3Stream

CDTSAC3Stream::CDTSAC3Stream(LPCWSTR fn, CSource* pParent, HRESULT* phr) 
	: CBaseStream(NAME("CDTSAC3Stream"), pParent, phr)
	, m_file(NULL)
	, m_stream_id(0)
{
	HRESULT hr = E_FAIL;

	CComPtr<IAsyncReader> reader = new CAsyncFileReader(fn, hr);

	if(FAILED(hr))
	{
		if(phr) *phr = hr;

		return;
	}

	hr = E_FAIL;

	m_file = new CBaseSplitterFile(reader, hr);

	if(FAILED(hr))
	{
		if(phr) *phr = hr;

		return;
	}

	CMediaType mt;

	if(!m_mt.IsValid())
	{
		CBaseSplitterFile::DTSHeader h;

		m_file->Seek(0);

		int len = 10;

		if(m_file->BitRead(64, true) == 0x4454534844484452ull) // DTSHDHDR
		{
			len = 0x100;
		}

		if(m_file->Read(h, len, &mt))
		{
			m_mt = mt;

			m_stream_id = 0x88;
		}
	}

	if(!m_mt.IsValid())
	{
		CBaseSplitterFile::AC3Header h;

		m_file->Seek(0);

		if(m_file->Read(h, 7, &mt))
		{
			m_mt = mt;

			m_stream_id = 0x80;
		}
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.Format();

	if(!m_mt.IsValid() || wfe == NULL || wfe->nAvgBytesPerSec == 0)
	{
		if(phr) *phr = E_FAIL;

		return;
	}

	m_AvgTimePerFrame = 10000000i64 * wfe->nBlockAlign / wfe->nAvgBytesPerSec;

	m_rtDuration = 10000000i64 * m_file->GetLength() / wfe->nAvgBytesPerSec;

	m_rtStop = m_rtDuration;
}

CDTSAC3Stream::~CDTSAC3Stream()
{
	delete m_file;
}

HRESULT CDTSAC3Stream::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = ((((WAVEFORMATEX*)m_mt.Format())->nBlockAlign + 35) + 15) & ~15;

    return NOERROR;
}

HRESULT CDTSAC3Stream::FillBuffer(IMediaSample* pSample, int frame, BYTE* buff, long& len)
{
	BYTE* base = buff;

	const GUID* majortype = &m_mt.majortype;
	const GUID* subtype = &m_mt.subtype;

	if(*majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
	{
		BYTE h[] = 
		{
			0x00, 0x00, 0x01, 0xBA,				// PES id
			0x44, 0x00, 0x04, 0x00, 0x04, 0x01,	// SCR (0)
			0x01, 0x89, 0xC3, 0xF8,				// mux rate (1260000 bytes/sec, 22bits), marker (2bits), reserved (~0, 5bits), stuffing (0, 3bits)
		};

		memcpy(buff, &h, sizeof(h));

		buff += sizeof(h);

		majortype = &MEDIATYPE_MPEG2_PES;
	}

	if(*majortype == MEDIATYPE_MPEG2_PES)
	{
		BYTE h[] = 
		{
			0x00, 0x00, 0x01, 0xBD,				// private stream 1 id
			0x07, 0xEC,							// packet length (TODO: modify it later)
			0x81, 0x80,							// marker, original, PTS - flags
			0x08,								// packet data starting offset
			0x21, 0x00, 0x01, 0x00, 0x01,		// PTS (0)
			0xFF, 0xFF, 0xFF,					// stuffing
			m_stream_id,						// stream id (0)
			0x01, 0x00, 0x01,					// no idea about this (might be the current sector on the disc), but dvd2avi doesn't output it to the ac3/dts file so we have to put it back
		};

		memcpy(buff, &h, sizeof(h));

		buff += sizeof(h);

		majortype = &MEDIATYPE_Audio;
	}

	if(*majortype == MEDIATYPE_Audio)
	{
		int size = ((WAVEFORMATEX*)m_mt.Format())->nBlockAlign;

		m_file->Seek((__int64)frame * size);

		if(FAILED(m_file->ByteRead(buff, size)))
		{
			return S_FALSE;
		}

		buff += size;
	}

	len = buff - base;

	return S_OK;
}

bool CDTSAC3Stream::CheckDTS(const CMediaType* pmt)
{
	return (pmt->majortype == MEDIATYPE_Audio
			|| pmt->majortype == MEDIATYPE_MPEG2_PES 
			|| pmt->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
		&& pmt->subtype == MEDIASUBTYPE_DTS;
}

bool CDTSAC3Stream::CheckWAVEDTS(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		&& pmt->subtype == MEDIASUBTYPE_WAVE_DTS
		&& pmt->formattype == FORMAT_WaveFormatEx
		&& ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_DVD_DTS;
}

bool CDTSAC3Stream::CheckAC3(const CMediaType* pmt)
{
	return (pmt->majortype == MEDIATYPE_Audio
			|| pmt->majortype == MEDIATYPE_MPEG2_PES 
			|| pmt->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
		&& pmt->subtype == MEDIASUBTYPE_DOLBY_AC3;
}

bool CDTSAC3Stream::CheckWAVEAC3(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		&& pmt->subtype == MEDIASUBTYPE_DOLBY_AC3
		&& pmt->formattype == FORMAT_WaveFormatEx
		&& ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_DOLBY_AC3;
}

HRESULT CDTSAC3Stream::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

	if(iPosition < 0)
	{
		return E_INVALIDARG;
	}

	*pmt = m_mt;

	switch(iPosition)
	{
	case 0:
		pmt->majortype = MEDIATYPE_Audio;
		break;
	case 1:
		pmt->ResetFormatBuffer();
		pmt->formattype = FORMAT_None;
	case 2:
		pmt->majortype = MEDIATYPE_MPEG2_PES;
		break;
	case 3:
		pmt->ResetFormatBuffer();
		pmt->formattype = FORMAT_None;
	case 4:
		pmt->majortype = MEDIATYPE_DVD_ENCRYPTED_PACK;
		break;
	default:
		return VFW_S_NO_MORE_ITEMS;
	}

	return S_OK;
}

HRESULT CDTSAC3Stream::CheckMediaType(const CMediaType* pmt)
{
	return CheckDTS(pmt) || CheckWAVEDTS(pmt) || CheckAC3(pmt) || CheckWAVEAC3(pmt) ? S_OK : E_INVALIDARG;
}
