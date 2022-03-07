#pragma once

#include "ffmpeg/ffcodecs.h"
#include "ffmpeg/avcodec.h"

struct libavcodec
{
	class CPU
	{
	public:
		enum PROCESSOR_TYPE
		{
			PROCESSOR_AMD,
			PROCESSOR_INTEL,
			PROCESSOR_UNKNOWN
		};

		enum CPUID_FLAGS
		{
			CPUID_MMX = (1 << 23),
			CPUID_SSE = (1 << 25),
			CPUID_SSE2 = (1 << 26),
			CPUID_SSE3 = (1 << 0),
			// Intel specifics
			CPUID_SSSE3 = (1 << 9),
			// AMD specifics
			CPUID_3DNOW = (1 << 31),
			CPUID_3DNOWEXT = (1 << 30),
			CPUID_MMXEXT = (1 << 22),
		};

	public:
		CPU();

		int m_processors;
		PROCESSOR_TYPE m_type;
		int m_features;

	} m_cpu;

	HMODULE m_dll;
	BYTE* m_buff;
	int m_buff_size;

	template<class T> void GetProcAddress(HMODULE hModule, T& fp, LPCSTR fn)
	{
		fp = (T)::GetProcAddress(hModule, fn);

		if(!fp) throw;
	}

public:
	void (*avcodec_init_dynamic)();
	//unsigned int (*avcodec_version_dynamic)();
	void (*avcodec_register_all_dynamic)();
	int (*avcodec_default_get_buffer_dynamic)(AVCodecContext2* avctx, AVFrame2* pic);
	void (*avcodec_default_release_buffer_dynamic)(AVCodecContext2* avctx, AVFrame2* pic);
	int (*avcodec_default_reget_buffer_dynamic)(AVCodecContext2* avctx, AVFrame2* pic);
	AVCodec2* (*avcodec_find_decoder_dynamic)(CodecID codecId);
	AVCodec2* (*avcodec_find_encoder_dynamic)(CodecID codecId);
	int (*avcodec_open_dynamic)(AVCodecContext2* avctx, AVCodec2* codec);
	int (*avcodec_close_dynamic)(AVCodecContext2* avctx);
	AVCodecContext2* (*avcodec_alloc_context_dynamic)();
	AVFrame2* (*avcodec_alloc_frame_dynamic)();
	int (*avcodec_thread_init_dynamic)(AVCodecContext2* avctx, int thread_count);
	void (*avcodec_thread_free_dynamic)(AVCodecContext2* avctx);
	int (*avcodec_decode_video2_dynamic)(AVCodecContext2* avctx, AVFrame2* picture, int* got_picture_ptr, AVPacket2* pkt);
	int (*avcodec_encode_video_dynamic)(AVCodecContext2* avctx, BYTE* buf, int buf_size, const AVFrame2* picture);
	int (*avcodec_decode_audio3_dynamic)(AVCodecContext2* avctx, WORD* samples, int* frame_size_ptr, AVPacket2* pkt);
	int (*avcodec_encode_audio_dynamic)(AVCodecContext2* avctx, BYTE* buf, int buf_size, const short* samples);
	void (*avcodec_flush_buffers_dynamic)(AVCodecContext2* avctx);
	void (*av_free_dynamic)(void* ptr);
	void (*av_init_packet_dynamic)(AVPacket2* pkt);
	void (*av_log_set_level_dynamic)(int level);
	void (*av_log_set_callback_dynamic)(void (*)(void*, int, const char*, va_list));

	AVCodec2* m_codec;
	AVCodecContext2* m_ctx;
	AVFrame2* m_frame;

public:	
	libavcodec(const wchar_t* ffmpeg_dll = NULL);
	virtual ~libavcodec();

	operator bool() const {return m_dll != NULL;}

	void Close();
	void Flush();

	bool InitH264Decoder(int width, int height, int nal_length_size, const void* extradata, int extradata_size);
	bool InitH264Encoder(int width, int height, bool p_frames, bool b_frames);
	bool InitMpeg2Encoder(int width, int height, bool p_frames, bool b_frames);
	bool InitAudioDecoder(const CMediaType& mt);
	bool InitAC3Encoder(int sample_rate, int channels);
	bool InitAACEncoder(int sample_rate, int channels);

	int DecodeVideo(int* got_picture, const BYTE* buff, int size, int late);
	int EncodeVideo(BYTE* y, BYTE* u, BYTE* v, int pitch, BYTE* buff, int size, bool* keyframe, int* pict_type, int quality = 95);
	int DecodeAudio(const BYTE* src, int src_size, std::vector<float>& dst);
	int EncodeAudio(BYTE* src, int src_size, BYTE* buff, int size, int* frame_size);
};
