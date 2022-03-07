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
#include "MPADecFilter.h"
#include "DirectShow.h"
#include "BaseSplitterFile.h"
#include "AsyncReader.h"
#include <initguid.h>
#include "moreuuids.h"

#include "../3rdparty/libfaad2/neaacdec.h"
#include "libavcodec.h"

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Payload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE_DOLBY_AC3},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE_DTS},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_LATM},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_LATM},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_LATM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_LATM},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_mp4a},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_mp4a},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_mp4a},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_mp4a},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_PS2_PCM},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_PS2_PCM},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_PS2_PCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PS2_PCM},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_PS2_ADPCM},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_PS2_ADPCM},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_PS2_ADPCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PS2_ADPCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_Vorbis2},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_FLAC_FRAMED},
};

// dshow: left, right, center, LFE, left surround, right surround
// ac3: LFE, left, center, right, left surround, right surround
// dts: center, left, right, left surround, right surround, LFE

// lets see how we can map these things to dshow (oh the joy!)

static struct scmap_t
{
	WORD channels;
	BYTE channel[6];
	DWORD mask;
}
s_scmap_ac3[2 * 11] = 
{
	{2, {0, 1,-1,-1,-1,-1}, 0},	// A52_CHANNEL
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_MONO
	{2, {0, 1,-1,-1,-1,-1}, 0}, // A52_STEREO
	{3, {0, 2, 1,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER}, // A52_3F
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER}, // A52_2F1R
	{4, {0, 2, 1, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER}, // A52_3F1R
	{4, {0, 1, 2, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_2F2R
	{5, {0, 2, 1, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_3F2R
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_CHANNEL1
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_CHANNEL2
	{2, {0, 1,-1,-1,-1,-1}, 0}, // A52_DOLBY

	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY},	// A52_CHANNEL|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_MONO|A52_LFE
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // A52_STEREO|A52_LFE
	{4, {1, 3, 2, 0,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_3F|A52_LFE
	{4, {1, 2, 0, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // A52_2F1R|A52_LFE
	{5, {1, 3, 2, 0, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // A52_3F1R|A52_LFE
	{5, {1, 2, 0, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_2F2R|A52_LFE
	{6, {1, 3, 2, 0, 4, 5}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_3F2R|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_CHANNEL1|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_CHANNEL2|A52_LFE
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // A52_DOLBY|A52_LFE
},
s_scmap_dts[2*10] = 
{
	{1, {0,-1,-1,-1,-1,-1}, 0}, // DTS_MONO
	{2, {0, 1,-1,-1,-1,-1}, 0},	// DTS_CHANNEL
	{2, {0, 1,-1,-1,-1,-1}, 0}, // DTS_STEREO
	{2, {0, 1,-1,-1,-1,-1}, 0}, // DTS_STEREO_SUMDIFF
	{2, {0, 1,-1,-1,-1,-1}, 0}, // DTS_STEREO_TOTAL
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER}, // DTS_3F
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER}, // DTS_2F1R
	{4, {1, 2, 0, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER}, // DTS_3F1R
	{4, {0, 1, 2, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_2F2R
	{5, {1, 2, 0, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_3F2R

	{2, {0, 1,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // DTS_MONO|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY},	// DTS_CHANNEL|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // DTS_STEREO|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // DTS_STEREO_SUMDIFF|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // DTS_STEREO_TOTAL|DTS_LFE
	{4, {1, 2, 0, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // DTS_3F|DTS_LFE
	{4, {0, 1, 3, 2,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // DTS_2F1R|DTS_LFE
	{5, {1, 2, 0, 4, 3,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // DTS_3F1R|DTS_LFE
	{5, {0, 1, 4, 2, 3,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_2F2R|DTS_LFE
	{6, {1, 2, 0, 5, 3, 4}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_3F2R|DTS_LFE
},
s_scmap_vorbis[6] = 
{
	{1, {0,-1,-1,-1,-1,-1}, 0}, // 1F
	{2, {0, 1,-1,-1,-1,-1}, 0},	// 2F
	{3, {0, 2, 1,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER}, // 2F1R
	{4, {0, 1, 2, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // 2F2R
	{5, {0, 2, 1, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // 3F2R
	{6, {0, 2, 1, 5, 3, 4}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // 3F2R + LFE
},
s_scmap_flac[6] = // TODO: verify channel order
{
	{1, {0,-1,-1,-1,-1,-1}, 0}, // 1F
	{2, {0, 1,-1,-1,-1,-1}, 0},	// 2F
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER}, // 2F1R
	{4, {0, 1, 2, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // 2F2R
	{5, {0, 1, 2, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // 3F2R
	{6, {0, 1, 2, 3, 4, 5}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // 3F2R + LFE
};

CMPADecoderFilter::CMPADecoderFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CMPADecoderFilter"), lpunk, __uuidof(this))
	, m_format(SF_PCM16)
	, m_normalize(false)
	, m_boost(1)
	, m_autodetect(false)
	, m_a52_state(NULL)
	, m_dts_state(NULL)
	, m_libavcodec(NULL)
{
	if(phr) *phr = S_OK;

	m_pInput = new CMpaDecInputPin(this, phr, L"In");
	
	if(FAILED(*phr)) return;

	m_pOutput = new CTransformOutputPin(NAME("CTransformOutputPin"), this, phr, L"Out");

	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}

	m_config[AC3] = A52_STEREO;
	m_config[DTS] = DTS_STEREO;
	m_config[AAC] = AAC_STEREO;

	m_drc[AC3] = false;
	m_drc[DTS] = false;
	m_drc[AAC] = false;

	CRegKey key;

	if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\Homesys\\Filters\\MPEG Audio Decoder", KEY_READ))
	{
		DWORD dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"SampleFormat", dw)) m_format = (Format)dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"Normalize", dw)) m_normalize = !!dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"Boost", dw)) m_boost = *(float*)&dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"Ac3SpeakerConfig", dw)) m_config[AC3] = (int)dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"DtsSpeakerConfig", dw)) m_config[DTS] = (int)dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"AacSpeakerConfig", dw)) m_config[AAC] = (int)dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"Ac3DynamicRangeControl", dw)) m_drc[AC3] = !!dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"DtsDynamicRangeControl", dw)) m_drc[DTS] = !!dw;
		if(ERROR_SUCCESS == key.QueryDWORDValue(L"AacDynamicRangeControl", dw)) m_drc[AAC] = !!dw;
	}

	m_libavcodec = new libavcodec();
}

CMPADecoderFilter::~CMPADecoderFilter()
{
	delete m_libavcodec;

	a52_free(m_a52_state);
	dts_free(m_dts_state);
}

STDMETHODIMP CMPADecoderFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMPADecoderFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMPADecoderFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	return __super::EndOfStream();
}

HRESULT CMPADecoderFilter::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CMPADecoderFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);

	m_buff.clear();
	m_sample_max = 0.1f;
	m_libavcodec->Flush();

	return __super::EndFlush();
}

HRESULT CMPADecoderFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_buff.clear();
	m_sample_max = 0.1f;
	m_ps2_state.sync = false;
	m_libavcodec->Flush();

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMPADecoderFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();

    if(pProps->dwStreamId != AM_STREAM_MEDIA)
	{
		return m_pOutput->Deliver(pIn);
	}

	AM_MEDIA_TYPE* pmt = NULL;
	
	if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);

		m_pInput->SetMediaType(&mt);

		DeleteMediaType(pmt);

		pmt = NULL;

		m_sample_max = 0.1f;
		m_aac_state.init(mt);

		m_vorbis.init(mt);
	}

	BYTE* pDataIn = NULL;

	if(FAILED(hr = pIn->GetPointer(&pDataIn))) 
	{
		return hr;
	}

	long len = pIn->GetActualDataLength();

	((CDeCSSInputPin*)m_pInput)->StripPacket(pDataIn, len);

	REFERENCE_TIME start = _I64_MIN;
	REFERENCE_TIME stop = _I64_MIN;

	bool hasTime = SUCCEEDED(pIn->GetTime(&start, &stop));

	if(pIn->IsDiscontinuity() == S_OK)
	{
		m_discontinuity = true;
		m_buff.clear();
		m_sample_max = 0.1f;

		// ASSERT(hasTime); // what to do if not?
		if(!hasTime) return S_OK; // lets wait then...

		m_start = start;
	}

	const CMediaType& mt = m_pInput->CurrentMediaType();

	if(hasTime && abs((int)(m_start - start)) > 10000)
	{
		m_start = start;
	}

	int size = m_buff.size();
	m_buff.resize(size + len);
	memcpy(m_buff.data() + size, pDataIn, len);
	len += size;

	if(m_autodetect && hasTime)
	{
		DetectMediaType();
	}

	if(mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
	{
		hr = ProcessLPCM();
	}
	else if(mt.subtype == MEDIASUBTYPE_DOLBY_AC3 || mt.subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
	{
		hr = ProcessAC3();
	}
	else if(mt.subtype == MEDIASUBTYPE_DTS || mt.subtype == MEDIASUBTYPE_WAVE_DTS)
	{
		hr = ProcessDTS();
	}
	else if(mt.subtype == MEDIASUBTYPE_AAC || mt.subtype == MEDIASUBTYPE_MP4A || mt.subtype == MEDIASUBTYPE_mp4a)
	{
		hr = ProcessAAC();
	}
	else if(mt.subtype == MEDIASUBTYPE_LATM)
	{
		hr = ProcessAAC();
	}
	else if(mt.subtype == MEDIASUBTYPE_PS2_PCM)
	{
		hr = ProcessPS2PCM();
	}
	else if(mt.subtype == MEDIASUBTYPE_PS2_ADPCM)
	{
		hr = ProcessPS2ADPCM();
	}
	else if(mt.subtype == MEDIASUBTYPE_Vorbis2)
	{
		hr = ProcessVorbis();
	}
	else if(mt.subtype == MEDIASUBTYPE_FLAC_FRAMED)
	{
		hr = ProcessFLAC();
	}
	else // if(.. the rest ..)
	{
		hr = ProcessMPA();
	}

	if(FAILED(hr))
	{
		m_buff.clear();
	}

	return hr;
}

void CMPADecoderFilter::DetectMediaType()
{
	CMediaType& curmt = m_pInput->CurrentMediaType();

	int size = m_buff.size();
	
	if(curmt.subtype != MEDIASUBTYPE_LATM)
	{
		size = min(size, 16);
	}

	CBaseSplitterFile f(&m_buff[0], size);

	CMediaType mt;
	
	if(curmt.subtype == MEDIASUBTYPE_MPEG2_AUDIO
	|| curmt.subtype == MEDIASUBTYPE_MP3
	|| curmt.subtype == MEDIASUBTYPE_MPEG1AudioPayload)
	{
		CBaseSplitterFile::MpegAudioHeader h;
		
		if(f.Read(h, size, false, &mt))
		{
			if(curmt == mt) 
			{
				return;
			}
		}
	}
	else if(curmt.subtype == MEDIASUBTYPE_DOLBY_AC3 || curmt.subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
	{
		CBaseSplitterFile::AC3Header h;
		
		if(f.Read(h, size, &mt))
		{
			if(curmt == mt) 
			{
				return;
			}
		}
	}
	else if(curmt.subtype == MEDIASUBTYPE_DTS)
	{
		CBaseSplitterFile::DTSHeader h;
		
		if(f.Read(h, size, &mt))
		{
			if(curmt == mt) 
			{
				return;
			}
		}
	}
	else if(curmt.subtype == MEDIASUBTYPE_LATM)
	{
		CBaseSplitterFile::LATMHeader h;

		bool nosync = false;
		
		if(f.Read(h, size, &mt, &nosync))
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)curmt.pbFormat;

			if(curmt.majortype == mt.majortype && curmt.subtype == mt.subtype && curmt.formattype == curmt.formattype) 
			{
				if(wfe->nChannels == h.channels && wfe->nSamplesPerSec == h.rate)
				{
					return;
				}
			}
		}

		if(nosync)
		{
			return;
		}
		/*
		nosync = false;

		f.Seek(0);
		f.Read(h, size, &mt, &nosync);
		*/
	}
	else if(curmt.subtype == MEDIASUBTYPE_AAC)
	{
		if(f.BitRead(8, true) == 0x21)
		{
			return;
		}

		CBaseSplitterFile::AACHeader h;
		
		if(f.Read(h, size, &mt))
		{
			if(curmt == mt) 
			{
				return;
			}
		}
	}
	else
	{
		return;
	}

	f.Seek(0);

	{
		CBaseSplitterFile::MpegAudioHeader h;
		
		if(f.Read(h, size, false, &mt))
		{
			curmt = mt;

			return;
		}
	}

	f.Seek(0);

	{
		CBaseSplitterFile::AC3Header h;
		
		if(f.Read(h, size, &mt))
		{
			curmt = mt;

			return;
		}
	}

	f.Seek(0);

	{
		CBaseSplitterFile::DTSHeader h;
		
		if(f.Read(h, size, &mt))
		{
			curmt = mt;

			return;
		}
	}

	f.Seek(0);

	{
		CBaseSplitterFile::LATMHeader h;
		
		if(f.Read(h, size, &mt))
		{
			curmt = mt;

			return;
		}
	}

	f.Seek(0);

	{
		CBaseSplitterFile::AACHeader h;
		
		if(f.Read(h, size, &mt))
		{
			curmt = mt;

			return;
		}
	}
}

HRESULT CMPADecoderFilter::ProcessLPCM()
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();

	ASSERT(wfe->nChannels == 2);
	ASSERT(wfe->wBitsPerSample == 16);

	BYTE* src = m_buff.data();

	int len = m_buff.size() & ~(wfe->nChannels * wfe->wBitsPerSample / 8 - 1);

	std::vector<float> buff(len * 8 / wfe->wBitsPerSample);

	float* dst = &buff[0];

	for(int i = 0; i < len; i += 2, src += 2, dst++)
	{
		*dst = (float)(short)((src[0] << 8) | src[1]) / 0x8000; // FIXME: care about 20/24 bps too
	}

	len = m_buff.size() - len;

	memmove(m_buff.data(), src, len);

	m_buff.resize(len);

	return Deliver(buff, wfe->nSamplesPerSec, wfe->nChannels);
}

HRESULT CMPADecoderFilter::ProcessAC3()
{
	BYTE* p = m_buff.data();
	BYTE* base = p;
	BYTE* end = p + m_buff.size();

	while(end - p >= 7)
	{
		int size, flags, samplerate, bitrate;

		size = a52_syncinfo(p, &flags, &samplerate, &bitrate);

		if(size <= 0)
		{
			p++;

			continue;
		}

		bool enough = p + size <= end;

		if(enough)
		{
			int config = GetSpeakerConfig(AC3);

			if(config < 0)
			{
				HRESULT hr;

				hr = Deliver(p, size, bitrate, 0x0001);

				if(S_OK != hr)
				{
					return hr;
				}
			}
			else
			{
				flags = (config & (A52_CHANNEL_MASK | A52_LFE)) | A52_ADJUST_LEVEL;

				sample_t level = 1, gain = 1, bias = 0;

				level *= gain;

				if(a52_frame(m_a52_state, p, &flags, &level, bias) == 0)
				{
					if(GetDynamicRangeControl(AC3))
					{
						a52_dynrng(m_a52_state, NULL, NULL);
					}

					int scmapidx = min(flags & A52_CHANNEL_MASK, countof(s_scmap_ac3) / 2);

                    scmap_t& scmap = s_scmap_ac3[scmapidx + ((flags & A52_LFE) ? countof(s_scmap_ac3) / 2 : 0)];

					std::vector<float> buff(6 * 256 * scmap.channels);

					float* dst = &buff[0];

					int i = 0;

					for(; i < 6 && a52_block(m_a52_state) == 0; i++)
					{
						sample_t* samples = a52_samples(m_a52_state);

						for(int j = 0; j < 256; j++, samples++)
						{
							for(int ch = 0; ch < scmap.channels; ch++)
							{
								ASSERT(scmap.channel[ch] != -1);

								*dst++ = (float)samples[scmap.channel[ch] << 8] / level;
							}
						}
					}

					if(i == 6)
					{
						HRESULT hr;

						hr = Deliver(buff, samplerate, scmap.channels, scmap.mask);

						if(S_OK != hr)
						{
							return hr;
						}
					}
				}
			}

			p += size;
		}

		memmove(base, p, end - p);
		end = base + (end - p);
		p = base;

		if(!enough)
		{
			break;
		}
	}

	m_buff.resize(end - p);

	return S_OK;
}

HRESULT CMPADecoderFilter::ProcessDTS()
{
	BYTE* p = m_buff.data();
	BYTE* base = p;
	BYTE* end = p + m_buff.size();

	while(end - p >= 14)
	{
		int size, flags, samplerate, bitrate, framelength;

		size = dts_syncinfo(m_dts_state, p, &flags, &samplerate, &bitrate, &framelength);

		if(size <= 0)
		{
			p++;

			continue;
		}
		
		bool enough = p + size <= end;

		if(enough)
		{
			int config = GetSpeakerConfig(DTS);

			if(config < 0)
			{
				HRESULT hr;

				hr = Deliver(p, size, bitrate, 0x000b);

				if(S_OK != hr)
				{
					return hr;
				}
			}
			else
			{
				flags = (config & (DTS_CHANNEL_MASK | DTS_LFE)) | DTS_ADJUST_LEVEL;

				sample_t level = 1, gain = 1, bias = 0;

				level *= gain;

				if(dts_frame(m_dts_state, p, &flags, &level, bias) == 0)
				{
					if(GetDynamicRangeControl(DTS))
					{
						dts_dynrng(m_dts_state, NULL, NULL);
					}

					int scmapidx = min(flags & DTS_CHANNEL_MASK, countof(s_scmap_dts) / 2);
                    
					scmap_t& scmap = s_scmap_dts[scmapidx + ((flags & DTS_LFE) ? countof(s_scmap_dts) / 2 : 0)];

					int blocks = dts_blocks_num(m_dts_state);

					std::vector<float> buff(blocks * 256 * scmap.channels);

					float* p = &buff[0];

					int i = 0;

					for(; i < blocks && dts_block(m_dts_state) == 0; i++)
					{
						sample_t* samples = dts_samples(m_dts_state);

						for(int j = 0; j < 256; j++, samples++)
						{
							for(int ch = 0; ch < scmap.channels; ch++)
							{
								ASSERT(scmap.channel[ch] != -1);

								*p++ = (float)samples[scmap.channel[ch] << 8] / level;
							}
						}
					}

					if(i == blocks)
					{
						HRESULT hr;

						hr = Deliver(buff, samplerate, scmap.channels, scmap.mask);

						if(S_OK != hr)
						{
							return hr;
						}
					}
				}
			}

			p += size;
		}

		memmove(base, p, end - p);
		end = base + (end - p);
		p = base;

		if(!enough)
		{
			break;
		}
	}

	m_buff.resize(end - p);

	return S_OK;
}

HRESULT CMPADecoderFilter::ProcessAAC()
{
	int config = GetSpeakerConfig(AAC);

	NeAACDecConfigurationPtr c = NeAACDecGetCurrentConfiguration(m_aac_state.h);
	
	c->downMatrix = config;

	NeAACDecSetConfiguration(m_aac_state.h, c);

	while(!m_buff.empty())
	{
		NeAACDecFrameInfo info;

		memset(&info, 0, sizeof(info));

		float* src = (float*)NeAACDecDecode(m_aac_state.h, &info, &m_buff[0], m_buff.size());
		
		if(info.bytesconsumed > 0)
		{
			if(info.bytesconsumed < m_buff.size()) 
			{
				BYTE* start = m_buff.data();
				BYTE* next = start + info.bytesconsumed;
				BYTE* end = start + m_buff.size();

				CMediaType& curmt = m_pInput->CurrentMediaType();

				if(curmt.subtype == MEDIASUBTYPE_LATM)
				{
					while(next < end - 1)
					{
						DWORD sync = (DWORD)(((next[0] << 8) | next[1])) >> 5;

						if(sync == 0x2b7)
						{
							break;
						}

						next++;
					}

					if(end - next == 1)
					{
						next = end;
					}
				}

				size_t size = end - next;

				memmove(start, next, size);

				m_buff.resize(size);
			}
			else
			{
				m_buff.clear();
			}
		}

		if(info.error > 1 || info.error == 1 && m_buff.size() >= 4) 
		{
			m_aac_state.init(m_pInput->CurrentMediaType());

			m_buff.clear();
		}

		if(src == NULL || info.samples == 0)
		{
			return S_OK;
		}

		// HACK: bug in faad2 with mono sources?

		if(info.channels == 2 && info.channel_position[1] == UNKNOWN_CHANNEL)
		{
			info.channel_position[0] = FRONT_CHANNEL_LEFT;
			info.channel_position[1] = FRONT_CHANNEL_RIGHT;
		}

		std::vector<float> buff(info.samples);

		float* dst = &buff[0];

		static std::map<int, int> chmask;

		if(chmask.empty())
		{
			chmask[FRONT_CHANNEL_CENTER] = SPEAKER_FRONT_CENTER;
			chmask[FRONT_CHANNEL_LEFT] = SPEAKER_FRONT_LEFT;
			chmask[FRONT_CHANNEL_RIGHT] = SPEAKER_FRONT_RIGHT;
			chmask[SIDE_CHANNEL_LEFT] = SPEAKER_SIDE_LEFT;
			chmask[SIDE_CHANNEL_RIGHT] = SPEAKER_SIDE_RIGHT;
			chmask[BACK_CHANNEL_LEFT] = SPEAKER_BACK_LEFT;
			chmask[BACK_CHANNEL_RIGHT] = SPEAKER_BACK_RIGHT;
			chmask[BACK_CHANNEL_CENTER] = SPEAKER_BACK_CENTER;
			chmask[LFE_CHANNEL] = SPEAKER_LOW_FREQUENCY;
		}

		DWORD mask = 0;

		for(int i = 0; i < info.channels; i++)
		{
			if(info.channel_position[i] == UNKNOWN_CHANNEL)
			{
				ASSERT(0); 

				return E_FAIL;
			}

			mask |= chmask[info.channel_position[i]];
		}

		int chmap[countof(info.channel_position)];

		memset(chmap, 0, sizeof(chmap));

		for(int i = 0; i < info.channels; i++)
		{
			unsigned int ch = 0, m = chmask[info.channel_position[i]];

			for(int j = 0; j < 32; j++)
			{
				if(mask & (1 << j))
				{
					if((1 << j) == m) {chmap[i] = ch; break;}

					ch++;
				}
			}
		}

		if(info.channels <= 2) mask = 0;

		for(int j = 0; j < info.samples; j += info.channels, dst += info.channels)
		{
			for(int i = 0; i < info.channels; i++)
			{
				dst[chmap[i]] = *src++;
			}
		}

		HRESULT hr;

		hr = Deliver(buff, info.samplerate, info.channels, mask);

		if(S_OK != hr)
		{
			return hr;
		}
	}

	return S_OK;
}

HRESULT CMPADecoderFilter::ProcessPS2PCM()
{
	BYTE* p = m_buff.data();
	BYTE* base = p;
	BYTE* end = p + m_buff.size();

	WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)m_pInput->CurrentMediaType().Format();

	int size = wfe->dwInterleave * wfe->nChannels;
	int samples = wfe->dwInterleave / (wfe->wBitsPerSample >> 3);
	int channels = wfe->nChannels;

	std::vector<float> buff(samples * channels);

	float* dst = buff.data();

	while(end - p >= size)
	{
		DWORD* dw = (DWORD*)p;

		if(dw[0] == 'dhSS')
		{
			p += dw[1] + 8;
		}
		else if(dw[0] == 'dbSS')
		{
			p += 8;

			m_ps2_state.sync = true;
		}
		else
		{
			if(m_ps2_state.sync)
			{
				float* d = dst;

				for(int i = 0; i < samples; i++)
				{
					short* s = (short*)p + i;

					for(int j = 0; j < channels; j++, s += samples, d++)
					{
						*d = (float)*s / 32768;
					}
				}
			}
			else
			{
				memset(dst, 0, samples * channels * sizeof(dst[0]));
			}

			HRESULT hr;

			hr = Deliver(buff, wfe->nSamplesPerSec, wfe->nChannels);

			if(S_OK != hr)
			{
				return hr;
			}

			p += size;

			memmove(base, p, end - p);
			end = base + (end - p);
			p = base;
		}
	}

	m_buff.resize(end - p);

	return S_OK;
}

static void decodeps2adpcm(ps2_state_t& s, int channel, BYTE* src, double* dst)
{
	int tbl_index = src[0] >> 4;
	int shift = src[0] & 0xf;
    int unk = src[1]; // ?

	if(tbl_index >= 10) {ASSERT(0); return;}

	// if(unk == 7) {ASSERT(0); return;} // ???

	static double s_tbl[] = 
	{
		0.0, 0.0, 0.9375, 0.0, 1.796875, -0.8125, 1.53125, -0.859375, 1.90625, -0.9375, 
		0.0, 0.0, -0.9375, 0.0, -1.796875, 0.8125, -1.53125, 0.859375 -1.90625, 0.9375
	};

	double* tbl = &s_tbl[tbl_index*2];
	double& a = s.a[channel];
	double& b = s.b[channel];

	for(int i = 0; i < 28; i++)
	{
		short input = (short)(((src[2 + i / 2] >> ((i & 1) << 2)) & 0xf) << 12) >> shift;
		double output = a * tbl[1] + b * tbl[0] + input;

		a = b;
		b = output;

		*dst++ = output / SHRT_MAX;
	}
}

HRESULT CMPADecoderFilter::ProcessPS2ADPCM()
{
	BYTE* p = m_buff.data();
	BYTE* base = p;
	BYTE* end = p + m_buff.size();

	WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)m_pInput->CurrentMediaType().Format();

	int size = wfe->dwInterleave * wfe->nChannels;
	int samples = wfe->dwInterleave * 14 / 16 * 2;
	int channels = wfe->nChannels;

	std::vector<float> buff(samples * channels);

	float* dst = &buff[0];

	while(end - p >= size)
	{
		DWORD* dw = (DWORD*)p;

		if(dw[0] == 'dhSS')
		{
			p += dw[1] + 8;
		}
		else if(dw[0] == 'dbSS')
		{
			p += 8;

			m_ps2_state.sync = true;
		}
		else
		{
			if(m_ps2_state.sync)
			{
				double* src = new double[samples * channels];

				for(int channel = 0, j = 0, k = 0; channel < channels; channel++, j += wfe->dwInterleave)
				{
					for(int i = 0; i < wfe->dwInterleave; i += 16, k += 28)
					{
						decodeps2adpcm(m_ps2_state, channel, p + i + j, src + k);
					}
				}

				float* d = dst;

				for(int i = 0; i < samples; i++)
				{
					double* s = src + i;

					for(int j = 0; j < channels; j++, s += samples, d++)
					{
						*d = (float)*s;
					}
				}

				delete [] src;
			}
			else
			{
				memset(dst, 0, samples * channels * sizeof(dst[0]));
			}

			HRESULT hr;

			hr = Deliver(buff, wfe->nSamplesPerSec, wfe->nChannels);

			if(S_OK != hr)
			{
				return hr;
			}

			p += size;
		}
	}

	memmove(base, p, end - p);
	end = base + (end - p);
	p = base;

	m_buff.resize(end - p);

	return S_OK;
}

HRESULT CMPADecoderFilter::ProcessVorbis()
{
	if(m_vorbis.vi.channels < 1 || m_vorbis.vi.channels > 6)
	{
		return E_FAIL;
	}

	if(m_buff.empty())
	{
		return S_OK;
	}

	HRESULT hr = S_OK;

	ogg_packet op;

	memset(&op, 0, sizeof(op));

	op.packet = m_buff.data();
	op.bytes = m_buff.size();
	op.b_o_s = 0;
	op.packetno = m_vorbis.packetno++;

	if(vorbis_synthesis(&m_vorbis.vb, &op, 1) == 0)
	{
		vorbis_synthesis_blockin(&m_vorbis.vd, &m_vorbis.vb);

		int samples;

		ogg_int32_t** src;

		while((samples = vorbis_synthesis_pcmout(&m_vorbis.vd, &src)) > 0)
		{
			const scmap_t& scmap = s_scmap_vorbis[m_vorbis.vi.channels - 1];

			std::vector<float> buff(samples * scmap.channels);

			float* dst = &buff[0];

			for(int j = 0, ch = scmap.channels; j < ch; j++)
			{
				int* s = src[scmap.channel[j]];
				float* d = &dst[j];

				for(int i = 0; i < samples; i++, d += ch)
				{
					*d = (float)max(min(s[i], 1 << 24), -1 << 24) / (1 << 24);
				}
			}

			hr = Deliver(buff, m_vorbis.vi.rate, scmap.channels, scmap.mask);

			if(S_OK != hr)
			{
				break;
			}

			vorbis_synthesis_read(&m_vorbis.vd, samples);
		}
	}

	m_buff.clear();

	return hr;
}

static inline float fscale(mad_fixed_t sample)
{
	if(sample >= MAD_F_ONE) 
	{
		sample = MAD_F_ONE - 1;
	}
	else if(sample < -MAD_F_ONE)
	{
		sample = -MAD_F_ONE;
	}

	return (float)sample / (1 << MAD_F_FRACBITS);
}

HRESULT CMPADecoderFilter::ProcessMPA()
{
	mad_stream_buffer(&m_stream, m_buff.data(), m_buff.size());

	while(1)
	{
		if(mad_frame_decode(&m_frame, &m_stream) == -1)
		{
			if(m_stream.error == MAD_ERROR_BUFLEN)
			{
				memmove(m_buff.data(), m_stream.this_frame, m_stream.bufend - m_stream.this_frame);

				m_buff.resize(m_stream.bufend - m_stream.this_frame);

				break;
			}
			/*
			else if(m_stream.error == MAD_ERROR_LOSTSYNC)
			{
				// TODO: digikábel / ng hd

				m_buff.clear();

				break;
			}
			*/

			if(!MAD_RECOVERABLE(m_stream.error))
			{
				printf("madlib error, cannot recover\n");

				return E_FAIL;
			}

			// FIXME: the renderer doesn't like this
			// m_discontinuity = true;
			
			continue;
		}

		if(m_stream.bufend > m_stream.next_frame)
		{
			if(m_stream.next_frame[0] != 0xff)
			{
				continue;
			}
		}
/*
// TODO: needs to be tested... (has anybody got an external mpeg audio decoder?)
HRESULT hr;
if(S_OK != (hr = Deliver(
   (BYTE*)m_stream.this_frame, 
   m_stream.next_frame - m_stream.this_frame, 
   m_frame.header.bitrate, 
   m_frame.header.layer == 1 ? 0x0004 : 0x0005)))
	return hr;
continue;
*/
		mad_synth_frame(&m_synth, &m_frame);

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();

		if(wfe->nChannels != m_synth.pcm.channels || wfe->nSamplesPerSec != m_synth.pcm.samplerate)
		{
			continue;
		}

		const mad_fixed_t* left_ch = m_synth.pcm.samples[0];
		const mad_fixed_t* right_ch = m_synth.pcm.samples[1];

		std::vector<float> buff(m_synth.pcm.length * m_synth.pcm.channels);

		float* dst = &buff[0];

		if(m_synth.pcm.channels == 2)
		{
			for(int i = 0; i < m_synth.pcm.length; i++)
			{
				dst[i * 2 + 0] = fscale(left_ch[i]);
				dst[i * 2 + 1] = fscale(right_ch[i]);
			}
		}
		else
		{
			for(int i = 0; i < m_synth.pcm.length; i++)
			{
				dst[i] = fscale(left_ch[i]);
			}
		}

		HRESULT hr;

		hr = Deliver(buff, m_synth.pcm.samplerate, m_synth.pcm.channels);

		if(S_OK != hr)
		{
			return hr;
		}
	}

	return S_OK;
}

HRESULT CMPADecoderFilter::ProcessFLAC()
{
	HRESULT hr = S_OK;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();

	DWORD channel_mask = 0;

	if(wfe->nChannels > 2)
	{
		// TODO: BYTE channel_config = m_buff[3] >> 4;
	
		for(int i = 0; i < sizeof(s_scmap_flac) / sizeof(s_scmap_flac[0]); i++)
		{
			if(s_scmap_flac[i].channels == wfe->nChannels)
			{
				channel_mask = s_scmap_flac[i].mask;
				
				break;
			}
		}
	}

	std::vector<float> samples;

	BYTE* src = m_buff.data();

	int i = 0;

	while(i < m_buff.size())
	{
		int size = samples.size();

		int res = m_libavcodec->DecodeAudio(&src[i], m_buff.size() - i, samples);

		if(res < 0 || res == 0 && samples.empty())
		{
			i = m_buff.size();

			break;
		}

		i += res;

		if(!samples.empty())
		{
			hr = Deliver(samples, wfe->nSamplesPerSec, wfe->nChannels, channel_mask);

			if(S_OK != hr)
			{
				break;
			}
		}
	}		

	int n = m_buff.size() - i;

	if(n > 0) 
	{
		memmove(&src[0], &src[i], n);

		m_buff.resize(n);
	}
	else
	{
		m_buff.clear();
	}

	return hr;
}

HRESULT CMPADecoderFilter::GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData)
{
	HRESULT hr;

	*pData = NULL;

	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(pSample, NULL, NULL, 0))
	|| FAILED(hr = (*pSample)->GetPointer(pData)))
	{
		return hr;
	}

	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED((*pSample)->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
	}

	return S_OK;
}

HRESULT CMPADecoderFilter::Deliver(std::vector<float>& buff, DWORD samplerate, WORD channels, DWORD mask)
{
	HRESULT hr;

	Format format = GetOutputFormat();

	CMediaType mt = CreateMediaType(format, samplerate, channels, mask);

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	int samples = buff.size() / wfe->nChannels;

	hr = ReconnectOutput(samples, mt);

	if(FAILED(hr))
	{
		return hr;
	}

	bool setmt = hr == S_OK;

	CComPtr<IMediaSample> pOut;

	BYTE* dst = NULL;

	hr = GetDeliveryBuffer(&pOut, &dst);

	if(FAILED(hr))
	{
		return E_FAIL;
	}

	REFERENCE_TIME dur = 10000000i64 * samples / wfe->nSamplesPerSec;
	REFERENCE_TIME start = m_start;
	REFERENCE_TIME stop = m_start + dur;

	m_start += dur;

// printf("a %I64d\n", start / 10000);

	if(start < 0)
	{
		return S_OK;
	}

	if(setmt)
	{
		m_pOutput->SetMediaType(&mt);

		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&start, &stop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_discontinuity);
	pOut->SetSyncPoint(TRUE);

	m_discontinuity = false;

	pOut->SetActualDataLength(buff.size() * wfe->wBitsPerSample / 8);

WAVEFORMATEX* wfeout = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();
ASSERT(wfeout->nChannels == wfe->nChannels);
ASSERT(wfeout->nSamplesPerSec == wfe->nSamplesPerSec);

	float* src = &buff[0];

	// TODO: move this into the audio switcher

	float sample_mul = 1;

	if(m_normalize)
	{
		for(int i = 0; i < buff.size(); i++)
		{
			float f = abs(src[i]);

			if(m_sample_max < f) 
			{
				m_sample_max = f;
			}
		}

		sample_mul = 1.0f / m_sample_max;
	}

	float boost = m_boost > 1 ? 1 + log10(m_boost) : 0;

	for(int i = 0; i < buff.size(); i++)
	{
		float f = src[i];

		// TODO: move this into the audio switcher

		if(m_normalize)
		{
			f *= sample_mul;
		}

		if(boost > 0)
		{
			f *= boost;
		}

		if(f < -1) 
		{
			f = -1;
		}
		else if(f > 1) 
		{
			f = 1;
		}

		auto round = [] (float f) -> int {return (int)(f >= 0 ? f + 0.5 : f - 0.5);};

		switch(format)
		{
		default:
		case SF_PCM16:
			*(short*)dst = (short)round(f * SHRT_MAX);
			dst += sizeof(short);
			break;
		case SF_PCM24:
			{DWORD dw = (DWORD)round(f * ((1 << 23) - 1));
			*dst++ = (BYTE)(dw);
			*dst++ = (BYTE)(dw >> 8);
			*dst++ = (BYTE)(dw >> 16);}
			break;
		case SF_PCM32:
			*(int*)dst = round(f * INT_MAX);
			dst += sizeof(int);
			break;
		case SF_FLOAT32:
			*(float*)dst = f;
			dst += sizeof(float);
			break;
		}
	}

	return m_pOutput->Deliver(pOut);
}

HRESULT CMPADecoderFilter::Deliver(BYTE* buff, int size, int bitrate, BYTE spdiftype)
{
	HRESULT hr;

	CMediaType mt = CreateMediaTypeSPDIF();

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	int len = 0;

	while(len < size + sizeof(WORD) * 4) 
	{
		len += 0x800;
	}

	int size2 = (__int64)wfe->nBlockAlign * wfe->nSamplesPerSec * size * 8 / bitrate;

	while(len < size2)
	{
		len += 0x800;
	}

	hr = ReconnectOutput(len / wfe->nBlockAlign, mt);

	if(FAILED(hr))
	{
		return hr;
	}

	CComPtr<IMediaSample> pOut;

	WORD* dst = NULL;

	hr = GetDeliveryBuffer(&pOut, (BYTE**)&dst);

	if(FAILED(hr))
	{
		return E_FAIL;
	}

	REFERENCE_TIME dur = 10000000i64 * size * 8 / bitrate;
	REFERENCE_TIME start = m_start;
	REFERENCE_TIME stop = m_start + dur;

	m_start += dur;

	if(start < 0)
	{
		return S_OK;
	}

	if(hr == S_OK)
	{
		m_pOutput->SetMediaType(&mt);

		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&start, &stop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_discontinuity);
	pOut->SetSyncPoint(TRUE);

	m_discontinuity = false;

	pOut->SetActualDataLength(len);

	if(size & 1) 
	{
		size++;
		memset(dst, 0, 8);
		dst += 4;
	}

	dst[0] = 0xf872;
	dst[1] = 0x4e1f;
	dst[2] = spdiftype;
	dst[3] = size * 8;

	_swab((char*)buff, (char*)&dst[4], size);

	return m_pOutput->Deliver(pOut);
}

HRESULT CMPADecoderFilter::ReconnectOutput(int samples, CMediaType& mt)
{
	HRESULT hr;

	CComQIPtr<IMemInputPin> pin = m_pOutput->GetConnected();

	if(pin == NULL) 
	{
		return E_NOINTERFACE;
	}

	CComPtr<IMemAllocator> allocator;

	hr = pin->GetAllocator(&allocator);

	if(FAILED(hr) || allocator ==  NULL)
	{
		return hr;
	}

	ALLOCATOR_PROPERTIES props, actual;

	hr = allocator->GetProperties(&props);

	if(FAILED(hr))
	{
		return hr;
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	long cbBuffer = samples * wfe->nBlockAlign;

	if(mt != m_pOutput->CurrentMediaType() || cbBuffer > props.cbBuffer)
	{
		if(cbBuffer > props.cbBuffer)
		{
			props.cBuffers = 4;
			props.cbBuffer = cbBuffer * 3 / 2;

			if(FAILED(hr = m_pOutput->DeliverBeginFlush())
			|| FAILED(hr = m_pOutput->DeliverEndFlush()))
			{
				return hr;
			}

			if(FAILED(hr = allocator->Decommit())
			|| FAILED(hr = allocator->SetProperties(&props, &actual))
			|| FAILED(hr = allocator->Commit()))
			{
				return hr;
			}

			if(props.cBuffers > actual.cBuffers || props.cbBuffer > actual.cbBuffer)
			{
				NotifyEvent(EC_ERRORABORT, hr, 0);

				return E_FAIL;
			}
		}

		return S_OK;
	}

	return S_FALSE;
}

CMediaType CMPADecoderFilter::CreateMediaType(Format sf, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask)
{
	CMediaType mt;

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = sf == SF_FLOAT32 ? MEDIASUBTYPE_IEEE_FLOAT : MEDIASUBTYPE_PCM;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEXTENSIBLE wfex;

	memset(&wfex, 0, sizeof(wfex));

	WAVEFORMATEX* wfe = &wfex.Format;

	wfe->wFormatTag = (WORD)mt.subtype.Data1;
	wfe->nChannels = nChannels;
	wfe->nSamplesPerSec = nSamplesPerSec;

	switch(sf)
	{
	default:
	case SF_PCM16: wfe->wBitsPerSample = 16; break;
	case SF_PCM24: wfe->wBitsPerSample = 24; break;
	case SF_PCM32: case SF_FLOAT32: wfe->wBitsPerSample = 32; break;
	}

	wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample / 8;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;

	// FIXME: 24/32 bit only seems to work with WAVE_FORMAT_EXTENSIBLE
	if(dwChannelMask == 0 && (sf == SF_PCM24 || sf == SF_PCM32))
	{
		dwChannelMask = nChannels == 2 ? (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT) : SPEAKER_FRONT_CENTER;
	}

	if(dwChannelMask)
	{
		wfex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfex.Format.cbSize = sizeof(wfex) - sizeof(wfex.Format);
		wfex.dwChannelMask = dwChannelMask;
		wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;
		wfex.SubFormat = mt.subtype;
	}

	mt.SetFormat((BYTE*)&wfex, sizeof(wfex.Format) + wfex.Format.cbSize);

	return mt;
}

CMediaType CMPADecoderFilter::CreateMediaTypeSPDIF()
{
	CMediaType mt = CreateMediaType(SF_PCM16, 48000, 2);

	((WAVEFORMATEX*)mt.pbFormat)->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;

	return mt;
}

HRESULT CMPADecoderFilter::CheckInputType(const CMediaType* mtIn)
{
	if(mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn->Format();

		if(wfe->nChannels != 2 || wfe->wBitsPerSample != 16) // TODO: remove this limitation
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	else if(mtIn->subtype == MEDIASUBTYPE_PS2_ADPCM)
	{
		WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)mtIn->Format();

		if(wfe->dwInterleave & 0xf) // has to be a multiple of the block size (16 bytes)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	else if(mtIn->subtype == MEDIASUBTYPE_Vorbis2)
	{
		if(!m_vorbis.init(*mtIn))
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	else if(mtIn->subtype == MEDIASUBTYPE_FLAC_FRAMED)
	{
		if(!m_libavcodec->InitAudioDecoder(*mtIn))
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	for(int i = 0; i < countof(sudPinTypesIn); i++)
	{
		if(*sudPinTypesIn[i].clsMajorType == mtIn->majortype && *sudPinTypesIn[i].clsMinorType == mtIn->subtype)
		{
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMPADecoderFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_PCM
		|| mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_IEEE_FLOAT
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMPADecoderFilter::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE)
	{
		return E_UNEXPECTED;
	}

	pProperties->cBuffers = 4;
	pProperties->cbBuffer = 48000 * 6 * (32 / 8) / 10; // 48KHz 6ch 32bps 100ms
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	return NOERROR;
}

HRESULT CMPADecoderFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) 
	{
		return E_UNEXPECTED;
	}

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;
	
	CMediaType mt = m_pInput->CurrentMediaType();

	const GUID& subtype = mt.subtype;
	
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	if(GetSpeakerConfig(AC3) < 0 && (subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
	|| GetSpeakerConfig(DTS) < 0 && (subtype == MEDIASUBTYPE_DTS || subtype == MEDIASUBTYPE_WAVE_DTS))
	{
		*pmt = CreateMediaTypeSPDIF();
	}
	else if(subtype == MEDIASUBTYPE_Vorbis2)
	{
		*pmt = CreateMediaType(GetOutputFormat(), m_vorbis.vi.rate, m_vorbis.vi.channels);
	}
	else
	{
		*pmt = CreateMediaType(GetOutputFormat(), wfe->nSamplesPerSec, std::min<int>(2, wfe->nChannels));
	}

	return S_OK;
}

HRESULT CMPADecoderFilter::StartStreaming()
{
	HRESULT hr;
	
	hr = __super::StartStreaming();

	if(FAILED(hr))
	{
		return hr;
	}

	if(m_a52_state == NULL)
	{
		m_a52_state = a52_init(0);
	}

	if(m_dts_state == NULL)
	{
		m_dts_state = dts_init(0);
	}

	m_aac_state.init(m_pInput->CurrentMediaType());

	mad_stream_init(&m_stream);
	mad_frame_init(&m_frame);
	mad_synth_init(&m_synth);
	mad_stream_options(&m_stream, 0/*options*/);

	m_ps2_state.reset();

	m_libavcodec->Flush();

	m_discontinuity = false;

	m_sample_max = 0.1f;

	return S_OK;
}

HRESULT CMPADecoderFilter::StopStreaming()
{
	a52_free(m_a52_state);
	dts_free(m_dts_state);
	
	m_a52_state = NULL;
	m_dts_state = NULL;

	mad_synth_finish(&m_synth);
	mad_frame_finish(&m_frame);
	mad_stream_finish(&m_stream);

	return __super::StopStreaming();
}

// IMPADecoderFilter

STDMETHODIMP CMPADecoderFilter::SetOutputFormat(Format f)
{
	CAutoLock cAutoLock(&m_csProps);

	m_format = f;

	return S_OK;
}

STDMETHODIMP_(IMPADecoderFilter::Format) CMPADecoderFilter::GetOutputFormat()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_format;
}

STDMETHODIMP CMPADecoderFilter::SetNormalize(bool normalize)
{
	CAutoLock cAutoLock(&m_csProps);

	if(m_normalize != normalize) 
	{
		m_sample_max = 0.1f;
	}

	m_normalize = normalize;
	
	return S_OK;
}

STDMETHODIMP_(bool) CMPADecoderFilter::GetNormalize()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_normalize;
}

STDMETHODIMP CMPADecoderFilter::SetSpeakerConfig(Codec c, int config)
{
	CAutoLock cAutoLock(&m_csProps);

	if(c >= 0 && c < CodecLast)
	{
		m_config[c] = config;
	}
	else 
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP_(int) CMPADecoderFilter::GetSpeakerConfig(Codec c)
{
	CAutoLock cAutoLock(&m_csProps);

	if(c >= 0 && c < CodecLast) 
	{
		return m_config[c];
	}

	return -1;
}

STDMETHODIMP CMPADecoderFilter::SetDynamicRangeControl(Codec c, bool drc)
{
	CAutoLock cAutoLock(&m_csProps);

	if(c >= 0 && c < CodecLast) 
	{
		m_drc[c] = drc;
	}
	else 
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP_(bool) CMPADecoderFilter::GetDynamicRangeControl(Codec c)
{
	CAutoLock cAutoLock(&m_csProps);
	
	if(c >= 0 && c < CodecLast) 
	{
		return m_drc[c];
	}

	return false;
}

STDMETHODIMP CMPADecoderFilter::SetBoost(float boost)
{
	CAutoLock cAutoLock(&m_csProps);

	m_boost = max(boost, 1);

	return S_OK;
}

STDMETHODIMP_(float) CMPADecoderFilter::GetBoost()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_boost;
}

STDMETHODIMP CMPADecoderFilter::SetAutoDetectMediaType(bool autodetect)
{
	CAutoLock cAutoLock(&m_csProps);

	m_autodetect = autodetect;

	return S_OK;
}

STDMETHODIMP CMPADecoderFilter::SaveConfig()
{
	CAutoLock cAutoLock(&m_csProps);

	CRegKey key;

	if(ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, L"Software\\Homesys\\Filters\\MPEG Audio Decoder"))
	{
		key.SetDWORDValue(L"SampleFormat", m_format);
		key.SetDWORDValue(L"Normalize", m_normalize);
		key.SetDWORDValue(L"Boost", *(DWORD*)&m_boost);
		key.SetDWORDValue(L"Ac3SpeakerConfig", m_config[AC3]);
		key.SetDWORDValue(L"DtsSpeakerConfig", m_config[DTS]);
		key.SetDWORDValue(L"AacSpeakerConfig", m_config[AAC]);
		key.SetDWORDValue(L"Ac3DynamicRangeControl", m_drc[AC3]);
		key.SetDWORDValue(L"DtsDynamicRangeControl", m_drc[DTS]);
		key.SetDWORDValue(L"AacDynamicRangeControl", m_drc[AAC]);
	}

	return S_OK;
}

// CMpaDecInputPin

CMpaDecInputPin::CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CDeCSSInputPin(NAME("CMpaDecInputPin"), pFilter, phr, pName)
{
}

// aac_state_t

aac_state_t::aac_state_t() 
	: h(NULL), freq(0), channels(0) 
{
	open();
}

aac_state_t::~aac_state_t() 
{
	close();
}

bool aac_state_t::open()
{
	close();

	h = NeAACDecOpen();

	if(h == NULL) 
	{
		return false;
	}

	NeAACDecConfigurationPtr c = NeAACDecGetCurrentConfiguration(h);

	c->outputFormat = FAAD_FMT_FLOAT;

	NeAACDecSetConfiguration(h, c);

	return true;
}

void aac_state_t::close()
{
	if(h != NULL) 
	{
		NeAACDecClose(h);
	
		h = NULL;
	}
}

bool aac_state_t::init(const CMediaType& mt)
{
	if(mt.subtype != MEDIASUBTYPE_AAC 
	&& mt.subtype != MEDIASUBTYPE_LATM 
	&& mt.subtype != MEDIASUBTYPE_MP4A 
	&& mt.subtype != MEDIASUBTYPE_mp4a)
	{
		return true; // nothing to do
	}

	open();

	const WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	if(wfe->cbSize > 0)
	{
		std::vector<BYTE> buff((wfe->cbSize + 3) & ~3);

		memcpy(buff.data(), wfe + 1, buff.size());

		if(mt.subtype == MEDIASUBTYPE_LATM)
		{
			return NeAACDecInit(h, buff.data(), buff.size(), &freq, &channels) >= 0;
		}
		else
		{
			return NeAACDecInit2(h, buff.data(), buff.size(), &freq, &channels) >= 0;
		}
	}

	return false;
}

// vorbis_state_t

vorbis_state_t::vorbis_state_t()
{
	memset(&vd, 0, sizeof(vd));
	memset(&vb, 0, sizeof(vb));
	memset(&vc, 0, sizeof(vc));
	memset(&vi, 0, sizeof(vi));
}

vorbis_state_t::~vorbis_state_t()
{
	clear();
}

void vorbis_state_t::clear()
{
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);
}

bool vorbis_state_t::init(const CMediaType& mt)
{
	if(mt.subtype != MEDIASUBTYPE_Vorbis2)
	{
		return true; // nothing to do
	}

	clear();

	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);

	VORBISFORMAT2* vf = (VORBISFORMAT2*)mt.Format();

	BYTE* fmt = mt.Format();

	packetno = 0;
	
	memset(&op, 0, sizeof(op));

	op.packet = (fmt += sizeof(*vf));
	op.bytes = vf->HeaderSize[0];
	op.b_o_s = 1;
	op.packetno = packetno++;

	if(vorbis_synthesis_headerin(&vi, &vc, &op) < 0)
	{
		return false;
	}

	memset(&op, 0, sizeof(op));
	
	op.packet = (fmt += vf->HeaderSize[0]);
	op.bytes = vf->HeaderSize[1];
	op.b_o_s = 0;
	op.packetno = packetno++;

	if(vorbis_synthesis_headerin(&vi, &vc, &op) < 0)
	{
		return false;
	}

	memset(&op, 0, sizeof(op));

	op.packet = (fmt += vf->HeaderSize[1]);
	op.bytes = vf->HeaderSize[2];
	op.b_o_s = 0;
	op.packetno = packetno++;

	if(vorbis_synthesis_headerin(&vi, &vc, &op) < 0)
	{
		return false;
	}

	postgain = 1.0;

	if(vorbis_comment_query_count(&vc, "LWING_GAIN"))
	{
		postgain = atof(vorbis_comment_query(&vc, "LWING_GAIN", 0));
	}

	if(vorbis_comment_query_count(&vc, "POSTGAIN"))
	{
		postgain = atof(vorbis_comment_query(&vc, "POSTGAIN", 0));
	}

	if(vorbis_comment_query_count(&vc, "REPLAYGAIN_TRACK_GAIN"))
	{
		postgain = pow(10.0, atof(vorbis_comment_query(&vc, "REPLAYGAIN_TRACK_GAIN", 0)) / 20.0);
	}

	vorbis_synthesis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	return true;
}
