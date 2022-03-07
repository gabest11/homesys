#pragma once

struct libavcodec;

[uuid("C3349146-4860-4BC5-B09C-58448F2785B4")]
class CMpeg2EncoderFilter : public CTransformFilter
{
	libavcodec* m_libavcodec;

	std::vector<BYTE> m_buff;
	int m_size;

	std::list<REFERENCE_TIME> m_start;
	int m_frame;

	HRESULT Transform(IMediaSample* pIn);

    HRESULT StartStreaming();
    HRESULT StopStreaming();

public:
	CMpeg2EncoderFilter();
	virtual ~CMpeg2EncoderFilter();

	HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
    HRESULT Receive(IMediaSample* pIn);
};