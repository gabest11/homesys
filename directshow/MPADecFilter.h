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

#pragma once

#include "../3rdparty/libmad/mad.h"
#include "../3rdparty/a52dec/a52.h"
#include "../3rdparty/dtsdec/dts.h"
// #include "faad2/neaacdec.h" // conflicts with dxtrans.h
#include "../3rdparty/libvorbis/ivorbiscodec.h"
#include "IMpaDecFilter.h"
#include "DeCSSInputPin.h"

struct aac_state_t
{
	void* h; // NeAACDecHandle h;
	DWORD freq;
	BYTE channels;

	aac_state_t();
	~aac_state_t();
	bool open();
	void close();
	bool init(const CMediaType& mt);
};

struct ps2_state_t
{
	bool sync;
	double a[2], b[2];
	ps2_state_t() {reset();}
	void reset() {sync = false; a[0] = a[1] = b[0] = b[1] = 0;}
};

struct vorbis_state_t
{
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_block vb;
	vorbis_dsp_state vd;
	ogg_packet op;
	int packetno;
	double postgain;

	vorbis_state_t();
	~vorbis_state_t();
	void clear();
	bool init(const CMediaType& mt);
};

struct libavcodec;

[uuid("3D446B6F-71DE-4437-BE15-8CE47174340F")]
class CMPADecoderFilter 
	: public CTransformFilter
	, public IMPADecoderFilter
{
protected:
	CCritSec m_csReceive;

	a52_state_t* m_a52_state;
	dts_state_t* m_dts_state;
	aac_state_t m_aac_state;
	mad_stream m_stream;
	mad_frame m_frame;
	mad_synth m_synth;
	ps2_state_t m_ps2_state;
	vorbis_state_t m_vorbis;
	libavcodec* m_libavcodec;

	std::vector<BYTE> m_buff;
	REFERENCE_TIME m_start;
	bool m_discontinuity;
	float m_sample_max;

	void DetectMediaType();

	HRESULT ProcessLPCM();
	HRESULT ProcessAC3();
	HRESULT ProcessDTS();
	HRESULT ProcessAAC();
	HRESULT ProcessPS2PCM();
	HRESULT ProcessPS2ADPCM();
	HRESULT ProcessVorbis();
	HRESULT ProcessMPA();
	HRESULT ProcessFLAC();

	HRESULT GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData);
	HRESULT Deliver(std::vector<float>& buff, DWORD samplerate, WORD channels, DWORD chmask = 0);
	HRESULT Deliver(BYTE* buff, int size, int bitrate, BYTE spdiftype);
	HRESULT ReconnectOutput(int samples, CMediaType& mt);
	
	CMediaType CreateMediaType(Format f, DWORD samplerate, WORD channels, DWORD chmask = 0);
	CMediaType CreateMediaTypeSPDIF();

protected:
	CCritSec m_csProps;
	Format m_format;
	bool m_normalize;
	int m_config[CodecLast];
	bool m_drc[CodecLast];
	float m_boost;
	bool m_autodetect;

public:
	CMPADecoderFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMPADecoderFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT Receive(IMediaSample* pIn);

    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

	// IMPADecoderFilter

	STDMETHODIMP SetOutputFormat(Format f);
	STDMETHODIMP_(Format) GetOutputFormat();
	STDMETHODIMP SetNormalize(bool normalize);
	STDMETHODIMP_(bool) GetNormalize();
	STDMETHODIMP SetSpeakerConfig(Codec c, int config);
	STDMETHODIMP_(int) GetSpeakerConfig(Codec c);
	STDMETHODIMP SetDynamicRangeControl(Codec c, bool drc);
	STDMETHODIMP_(bool) GetDynamicRangeControl(Codec c);
	STDMETHODIMP SetBoost(float boost);
	STDMETHODIMP_(float) GetBoost();
	STDMETHODIMP SetAutoDetectMediaType(bool autodetect);
	STDMETHODIMP SaveConfig();
};

class CMpaDecInputPin : public CDeCSSInputPin
{
public:
    CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);
};
