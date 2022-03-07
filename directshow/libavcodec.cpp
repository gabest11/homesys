#include "stdafx.h"
#include "libavcodec.h"
#include <intrin.h>

void my_av_log_default_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix=1;
    static int count;
    static char line[1024], prev[1024];
    AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
    if(level>100)
        return;
#undef fprintf
    if(print_prefix && avc) {
        sprintf(line, "[%s @ %p]", avc->item_name(ptr), ptr);
    }else
        line[0]=0;

    vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);

    print_prefix= line[strlen(line)-1] == '\n';
    if(print_prefix && !strcmp(line, prev)){
        count++;
        return;
    }
    if(count>0){
        fprintf(stderr, "    Last message repeated %d times\n", count);
        count=0;
    }
    fputs(line, stdout);
    strcpy(prev, line);
}

libavcodec::libavcodec(const wchar_t* ffmpeg_dll)
	: m_codec(NULL)
	, m_ctx(NULL)
	, m_frame(NULL)
	, m_buff(NULL)
	, m_buff_size(0)
{
	if(ffmpeg_dll == NULL)
	{
		ffmpeg_dll = L"libavcodec.dll";
	}

	wchar_t buff[MAX_PATH + 1] = {0};

	GetModuleFileName(NULL, buff, MAX_PATH);

	PathRemoveFileSpec(buff);

	std::wstring s = buff;

	m_dll = LoadLibrary((s + L"/" + ffmpeg_dll).c_str());

	if(m_dll == NULL)
	{
		m_dll = LoadLibrary((s + L"/../" + ffmpeg_dll).c_str());
	}

	if(m_dll != NULL)
	{
		try
		{
			GetProcAddress(m_dll, avcodec_init_dynamic, "avcodec_init");
			//GetProcAddress(m_dll, avcodec_version_dynamic, "avcodec_version");
			GetProcAddress(m_dll, avcodec_register_all_dynamic, "avcodec_register_all");
			GetProcAddress(m_dll, avcodec_default_get_buffer_dynamic, "avcodec_default_get_buffer");
			GetProcAddress(m_dll, avcodec_default_release_buffer_dynamic, "avcodec_default_release_buffer");
			GetProcAddress(m_dll, avcodec_default_reget_buffer_dynamic, "avcodec_default_reget_buffer");
			GetProcAddress(m_dll, avcodec_find_decoder_dynamic, "avcodec_find_decoder");
			GetProcAddress(m_dll, avcodec_find_encoder_dynamic, "avcodec_find_encoder");
			GetProcAddress(m_dll, avcodec_open_dynamic, "avcodec_open");
			GetProcAddress(m_dll, avcodec_close_dynamic, "avcodec_close");
			GetProcAddress(m_dll, avcodec_alloc_context_dynamic, "avcodec_alloc_context");
			GetProcAddress(m_dll, avcodec_alloc_frame_dynamic, "avcodec_alloc_frame");
			GetProcAddress(m_dll, avcodec_thread_init_dynamic, "avcodec_thread_init");
			GetProcAddress(m_dll, avcodec_thread_free_dynamic, "avcodec_thread_free");
			GetProcAddress(m_dll, avcodec_decode_video2_dynamic, "avcodec_decode_video2");
			GetProcAddress(m_dll, avcodec_encode_video_dynamic, "avcodec_encode_video");		
			GetProcAddress(m_dll, avcodec_decode_audio3_dynamic, "avcodec_decode_audio3");
			GetProcAddress(m_dll, avcodec_encode_audio_dynamic, "avcodec_encode_audio");
			GetProcAddress(m_dll, avcodec_flush_buffers_dynamic, "avcodec_flush_buffers");
			GetProcAddress(m_dll, av_free_dynamic, "av_free");
			GetProcAddress(m_dll, av_init_packet_dynamic, "av_init_packet");			
			GetProcAddress(m_dll, av_log_set_level_dynamic, "av_log_set_level");			
			GetProcAddress(m_dll, av_log_set_callback_dynamic, "av_log_set_callback");	

			avcodec_init_dynamic();
			avcodec_register_all_dynamic();

			//
			av_log_set_callback_dynamic(my_av_log_default_callback);
			//
			av_log_set_level_dynamic(48);
		}
		catch(...)
		{
			FreeLibrary(m_dll);

			m_dll = NULL;
		}
	}
}

libavcodec::~libavcodec()
{
	Close();

	if(m_dll)
	{
		FreeLibrary(m_dll);

		m_dll = NULL;
	}

	if(m_buff != NULL)
	{
		free(m_buff);
	}
}

void libavcodec::Close()
{
	if(m_ctx != NULL)
	{
		if(m_ctx->intra_matrix) free(m_ctx->intra_matrix);
		if(m_ctx->inter_matrix) free(m_ctx->inter_matrix);
		if(m_ctx->extradata) free((unsigned char*)m_ctx->extradata);
		if(m_ctx->slice_offset) av_free_dynamic(m_ctx->slice_offset);
		if(m_ctx->codec) avcodec_close_dynamic(m_ctx);
		if(m_ctx->thread_opaque) avcodec_thread_free_dynamic(m_ctx);

		av_free_dynamic(m_ctx);

		m_ctx = NULL;
	}

	if(m_frame != NULL)
	{
		av_free_dynamic(m_frame);

		m_frame = NULL;
	}
}

void libavcodec::Flush()
{
	if(m_ctx != NULL)
	{
		avcodec_flush_buffers_dynamic(m_ctx);
	}
}

bool libavcodec::InitH264Decoder(int width, int height, int nal_length_size, const void* extradata, int extradata_size)
{
	if(m_dll == NULL) return false;

	Close();

	m_codec = avcodec_find_decoder_dynamic(CODEC_ID_H264);
	m_ctx = avcodec_alloc_context_dynamic();
	m_frame = avcodec_alloc_frame_dynamic();

	avcodec_thread_init_dynamic(m_ctx, 1); // FIXME: m_cpu.m_processors);

	m_ctx->width = width;
	m_ctx->height = height;
	m_ctx->codec_tag = '1CVA';

	m_ctx->nal_length_size = nal_length_size;

	m_ctx->intra_matrix = (uint16_t*)calloc(sizeof(uint16_t), 64);
	m_ctx->inter_matrix = (uint16_t*)calloc(sizeof(uint16_t), 64);
	m_ctx->workaround_bugs = FF_BUG_AUTODETECT;
	m_ctx->error_concealment = FF_EC_DEBLOCK | FF_EC_GUESS_MVS;
	m_ctx->error_recognition = FF_ER_CAREFUL;
	m_ctx->idct_algo = FF_IDCT_AUTO;
	m_ctx->skip_loop_filter = AVDISCARD_NONKEY;
	m_ctx->dsp_mask = FF_MM_FORCE | m_cpu.m_features;
	m_ctx->debug_mv = 0;
	// TODO: m_ctx->opaque = this;
	// TODO: m_ctx->get_buffer = avcodec_default_get_buffer;
	// m_ctx->flags |= ;
	// m_ctx->flags2 |= CODEC_FLAG2_CHUNKS; 
	// m_ctx->flags2 |= CODEC_FLAG2_FAST;

	if(extradata_size > 0)
	{
		m_ctx->extradata_size = extradata_size;
		m_ctx->extradata = (const BYTE*)calloc(1, extradata_size + 8);
		memcpy((void*)m_ctx->extradata, extradata, extradata_size);
	}

	return avcodec_open_dynamic(m_ctx, m_codec) >= 0;
}

bool libavcodec::InitH264Encoder(int width, int height, bool p_frames, bool b_frames)
{
	if(m_dll == NULL) return false;

	 _mm_empty();

	Close();
	/**/
	for(int i = 0; i < 256; i++)
	{
		if(avcodec_find_encoder_dynamic((CodecID)i))
		{
			printf("%d\n", i);
		}
	}
	
	m_codec = avcodec_find_encoder_dynamic(CODEC_ID_H264);
	m_ctx = avcodec_alloc_context_dynamic();
	m_frame = avcodec_alloc_frame_dynamic();

	avcodec_thread_init_dynamic(m_ctx, m_cpu.m_processors);

	m_ctx->width = width;
	m_ctx->height = height;
	m_ctx->codec_tag = '462H';
/*
	mb_width = (width + 15) / 16;
	mb_height = (height + 15) / 16;
	mb_count = mb_width * mb_height;
*/
	m_ctx->time_base.den = 25;
	m_ctx->time_base.num = 1;
	m_ctx->gop_size = p_frames ? 25 : 1;

	// m_ctx->flags |= CODEC_FLAG_INTERLACED_DCT | CODEC_FLAG_INTERLACED_ME;
	m_ctx->mb_decision = 1;

	m_ctx->me_cmp |= FF_CMP_CHROMA;
	m_ctx->dia_size = 1;
	m_ctx->pre_me = 1;
	m_ctx->pre_dia_size = 1;
	m_ctx->nsse_weight = 8;

	m_ctx->qmin_i = 2; 
	m_ctx->qmax_i = 31;
	m_ctx->qmin = 2;
	m_ctx->qmax = 31;
	m_ctx->qmin_b = 2;
	m_ctx->qmax_b = 31;

	m_ctx->i_quant_factor = -80.0f / 100;
	m_ctx->i_quant_offset = 0 / 100.0f;
	m_ctx->intra_dc_precision = 1;

	m_ctx->pix_fmt = PIX_FMT_YUV420P;

	m_ctx->bit_rate = 4000 * 1000;
	m_ctx->bit_rate_tolerance = 1024 * 8 * 1000;
	m_ctx->qcompress = 50.0f / 100;
	m_ctx->qblur = 0.5f;
	m_ctx->max_qdiff = 3;
	m_ctx->rc_buffer_aggressivity = 1.0f;

	m_ctx->sample_aspect_ratio.num = 1;
	m_ctx->sample_aspect_ratio.den = 1;

	m_ctx->max_b_frames = p_frames && b_frames ? 2 : 0;
	m_ctx->b_quant_factor = 125.0f / 100;
	m_ctx->b_quant_offset = 125.0f / 100;
	m_ctx->b_frame_strategy = 1;

	m_ctx->flags |= CODEC_FLAG_NORMALIZE_AQP;
	m_ctx->lmin = 118;
	m_ctx->lmax = 3658;

	return avcodec_open_dynamic(m_ctx, m_codec) >= 0;
}

bool libavcodec::InitMpeg2Encoder(int width, int height, bool p_frames, bool b_frames)
{
	if(m_dll == NULL) return false;

	 _mm_empty();

	Close();
	
	m_codec = avcodec_find_encoder_dynamic(CODEC_ID_MPEG2VIDEO);
	m_ctx = avcodec_alloc_context_dynamic();
	m_frame = avcodec_alloc_frame_dynamic();

	avcodec_thread_init_dynamic(m_ctx, m_cpu.m_processors);

	m_ctx->width = width;
	m_ctx->height = height;
	m_ctx->codec_tag	= 'GEPM';
/*
	mb_width = (width + 15) / 16;
	mb_height = (height + 15) / 16;
	mb_count = mb_width * mb_height;
*/
	m_ctx->time_base.den = 25;
	m_ctx->time_base.num = 1;
	m_ctx->gop_size = p_frames ? 25 : 1;

	// m_ctx->flags |= CODEC_FLAG_INTERLACED_DCT | CODEC_FLAG_INTERLACED_ME;
	m_ctx->mb_decision = 1;

	m_ctx->me_cmp |= FF_CMP_CHROMA;
	m_ctx->dia_size = 1;
	m_ctx->pre_me = 1;
	m_ctx->pre_dia_size = 1;
	m_ctx->nsse_weight = 8;

	m_ctx->qmin_i = 2; 
	m_ctx->qmax_i = 31;
	m_ctx->qmin = 2;
	m_ctx->qmax = 31;
	m_ctx->qmin_b = 2;
	m_ctx->qmax_b = 31;

	m_ctx->i_quant_factor = -80.0f / 100;
	m_ctx->i_quant_offset = 0 / 100.0f;
	m_ctx->intra_dc_precision = 1;

	m_ctx->pix_fmt = PIX_FMT_YUV420P;

	m_ctx->bit_rate = 4000 * 1000;
	m_ctx->bit_rate_tolerance = 1024 * 8 * 1000;
	m_ctx->qcompress = 50.0f / 100;
	m_ctx->qblur = 0.5f;
	m_ctx->max_qdiff = 3;
	m_ctx->rc_buffer_aggressivity = 1.0f;

	m_ctx->sample_aspect_ratio.num = 1;
	m_ctx->sample_aspect_ratio.den = 1;

	m_ctx->max_b_frames = p_frames && b_frames ? 2 : 0;
	m_ctx->b_quant_factor = 125.0f / 100;
	m_ctx->b_quant_offset = 125.0f / 100;
	m_ctx->b_frame_strategy = 1;

	m_ctx->flags |= CODEC_FLAG_NORMALIZE_AQP;
	m_ctx->lmin = 118;
	m_ctx->lmax = 3658;

	return avcodec_open_dynamic(m_ctx, m_codec) >= 0;
}

bool libavcodec::InitAudioDecoder(const CMediaType& mt)
{
	if(m_dll == NULL) return false;

	Close();

	if(mt.formattype != FORMAT_WaveFormatEx)
	{
		return false;
	}

	const WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;

	CodecID codec_id = CODEC_ID_FLAC; // TODO

	m_codec = avcodec_find_decoder_dynamic(codec_id);

	if(m_codec == NULL) return false;

	m_ctx = avcodec_alloc_context_dynamic();
	m_ctx->codec_id = codec_id; // ???
	m_ctx->sample_rate = wfe->nSamplesPerSec;
	m_ctx->channels = wfe->nChannels;
	m_ctx->bit_rate = wfe->nAvgBytesPerSec * 8;
	m_ctx->bits_per_coded_sample = wfe->wBitsPerSample;
	m_ctx->block_align = wfe->nBlockAlign;

	const BYTE* extradata = (const BYTE*)(wfe + 1);
	int extradata_size = std::min<int>(wfe->cbSize, mt.cbFormat - sizeof(WAVEFORMATEX));

	if(extradata_size > 0)
	{
		if(codec_id == CODEC_ID_FLAC)
		{
			if(extradata_size < 8 + 34 || *(const DWORD*)extradata != 'CaLf')
			{
				return false;
			}

//			extradata += 8;
//			extradata_size = 34;
		}

		m_ctx->extradata_size = extradata_size;
		m_ctx->extradata = (const BYTE*)calloc(1, extradata_size + 8);
		memcpy((void*)m_ctx->extradata, extradata, extradata_size);
	}

	return avcodec_open_dynamic(m_ctx, m_codec) >= 0;
}

bool libavcodec::InitAC3Encoder(int sample_rate, int channels)
{
	if(m_dll == NULL) return false;

	 _mm_empty();

	Close();

	m_codec = avcodec_find_encoder_dynamic(CODEC_ID_AC3);
	m_ctx = avcodec_alloc_context_dynamic();

    m_ctx->sample_rate = sample_rate;
	m_ctx->channels = channels;
    m_ctx->bit_rate = 128000;
	m_ctx->ac3mode = 2; // stereo
	m_ctx->ac3lfe = 0;
	m_ctx->ac3channels[0] = 0;
	m_ctx->ac3channels[1] = 1;
	m_ctx->channel_layout = CH_LAYOUT_STEREO;

	return avcodec_open_dynamic(m_ctx, m_codec) >= 0;
}

bool libavcodec::InitAACEncoder(int sample_rate, int channels)
{
	if(m_dll == NULL) return false;

	 _mm_empty();

	Close();

	for(int i = 0; i < 256; i++)
	{
		if(avcodec_find_encoder_dynamic((CodecID)i))
		{
			printf("%d\n", i);
		}
	}

	m_codec = avcodec_find_encoder_dynamic(CODEC_ID_AAC);
	m_ctx = avcodec_alloc_context_dynamic();

	m_ctx->profile = FF_PROFILE_AAC_MAIN;
	m_ctx->sample_fmt = SAMPLE_FMT_S16;
	m_ctx->sample_rate = sample_rate;
	m_ctx->channels = channels;
	m_ctx->bit_rate = 64000;
	m_ctx->time_base.num = 1;
	m_ctx->time_base.den = sample_rate;
	m_ctx->channel_layout = CH_LAYOUT_STEREO;

	return avcodec_open_dynamic(m_ctx, m_codec) >= 0;
}

int libavcodec::DecodeVideo(int* got_picture, const BYTE* buff, int size, int late)
{
	if(m_buff_size < size + FF_INPUT_BUFFER_PADDING_SIZE)
	{
		m_buff_size = size + FF_INPUT_BUFFER_PADDING_SIZE;
		m_buff = (BYTE*)realloc(m_buff, m_buff_size);
	}

	memcpy(m_buff, buff, size);
	memset(m_buff + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

	m_ctx->skip_loop_filter = late > 50 ? AVDISCARD_ALL : AVDISCARD_NONKEY;
	m_ctx->skip_frame = late > 500 ? AVDISCARD_NONREF : AVDISCARD_NONE;
	//m_ctx->skip_frame = AVDISCARD_NONREF;

	AVPacket2 pkt;

	av_init_packet_dynamic(&pkt);

	pkt.data = m_buff;
	pkt.size = size;

	return avcodec_decode_video2_dynamic(m_ctx, m_frame, got_picture, &pkt);
}

int libavcodec::EncodeVideo(BYTE* y, BYTE* u, BYTE* v, int pitch, BYTE* buff, int size, bool* keyframe, int* pict_type, int quality)
{
	m_ctx->flags |= CODEC_FLAG_QSCALE;

	m_frame->quality = (100 - quality) * 40;

	m_frame->pict_type = 0;

	for(int i = 0; i < 4; i++)
	{
		m_frame->data[i] = NULL;
		m_frame->linesize[i] = 0;
	}

	if(y)
	{
		m_frame->data[0] = y;
		m_frame->data[1] = u;
		m_frame->data[2] = v;

		m_frame->linesize[0] = pitch;
		m_frame->linesize[1] = pitch >> 1;
		m_frame->linesize[2] = pitch >> 1;
	}

	int ret = avcodec_encode_video_dynamic(m_ctx, buff, size, y ? m_frame : NULL);

	if(ret >= 0)
	{
		if(keyframe != NULL)
		{
			*keyframe = m_ctx->coded_frame->key_frame != 0;
		}

		if(pict_type != NULL)
		{
			*pict_type = m_ctx->coded_frame->pict_type;
		}
	}

	return ret;
}

int libavcodec::DecodeAudio(const BYTE* src, int src_size, std::vector<float>& dst)
{
	dst.clear();

	std::vector<BYTE> tmp(AVCODEC_MAX_AUDIO_FRAME_SIZE);

	int tmp_size = tmp.size();

	AVPacket2 pkt;

	av_init_packet_dynamic(&pkt);

	pkt.data = (uint8_t*)src;
	pkt.size = src_size;

	int res = avcodec_decode_audio3_dynamic(m_ctx, (WORD*)tmp.data(), &tmp_size, &pkt);

	if(res >= 0 && tmp_size > 0)
	{
		// TODO: sse

		switch(m_ctx->sample_fmt)
		{
		case SAMPLE_FMT_U8:

			dst.resize(tmp_size);

			for(int i = 0; i < tmp_size; i++)
			{
				dst[i] = (float)(((BYTE*)tmp.data())[i] - 127) / 255;
			}

			break;

		case SAMPLE_FMT_S16:

			tmp_size /= 2;

			dst.resize(tmp_size);

			for(int i = 0; i < tmp_size; i++)
			{
				dst[i] = (float)((short*)tmp.data())[i] / SHRT_MAX;
			}

			break;

		case SAMPLE_FMT_S32:

			tmp_size /= 4;

			dst.resize(tmp_size);

			for(int i = 0; i < tmp_size; i++)
			{
				dst[i] = (float)((int*)tmp.data())[i] / INT_MAX;
			}

			break;

		case SAMPLE_FMT_FLT:

			tmp_size /= 4;

			dst.resize(tmp_size);

			memcpy(dst.data(), tmp.data(), dst.size());

			break;

		case SAMPLE_FMT_DBL:

			tmp_size /= 8;

			dst.resize(tmp_size);

			for(int i = 0; i < tmp_size; i++)
			{
				dst[i] = (float)((double*)tmp.data())[i];
			}

			break;

		default: 
			
			res = -1;

			break;
		}
	}

	return res;
}

int libavcodec::EncodeAudio(BYTE* src, int src_size, BYTE* buff, int size, int* frame_size)
{
	*frame_size = m_ctx->frame_size * m_ctx->channels * 16 >> 3;

	return src_size >= *frame_size ? avcodec_encode_audio_dynamic(m_ctx, buff, *frame_size, (const short*)src) : 0;
}

// CPU

libavcodec::CPU::CPU()
	: m_features(0)
	, m_type(PROCESSOR_UNKNOWN)
{
	SYSTEM_INFO si;

	GetSystemInfo(&si);

	m_processors = si.dwNumberOfProcessors;

	int buff[4];

	__cpuid(buff, 0);

	DWORD nHighestFeature = (DWORD)buff[0];

	char type[13];

	*(int*)&type[0] = buff[1];
	*(int*)&type[4] = buff[3];
	*(int*)&type[8] = buff[2];

	type[12] = 0;

	if(strcmp(type, "AuthenticAMD") == 0) m_type = PROCESSOR_AMD;
	else if(strcmp(type, "GenuineIntel") == 0) m_type = PROCESSOR_INTEL;

	// Get highest extended feature

	__cpuid(buff, 0x80000000);

	DWORD nHighestFeatureEx = (DWORD)buff[0];

	if(nHighestFeature >= 1)
	{
		__cpuid(buff, 1);

		if(buff[3] & (1 << 23)) m_features |= FF_MM_MMX;
		if(buff[3] & (1 << 25)) m_features |= FF_MM_SSE;
		if(buff[3] & (1 << 26)) m_features |= FF_MM_SSE2;
		if(buff[2] & (1 << 0)) m_features |= FF_MM_SSE3;

		if(m_type == PROCESSOR_INTEL)
		{
			if(buff[2] & (1 << 9)) m_features |= FF_MM_SSSE3;
		}
	}

	if(m_type == PROCESSOR_AMD)
	{
		__cpuid(buff, 0x80000000);

		if(nHighestFeatureEx >= 0x80000001)
		{
			__cpuid(buff, 0x80000001);

			if(buff[3] & (1 << 31)) m_features |= FF_MM_3DNOW;
			if(buff[3] & (1 << 22)) m_features |= FF_MM_MMXEXT;
		}
	}
}
