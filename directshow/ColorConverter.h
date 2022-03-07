#pragma once

[uuid("BEEE4D65-76F6-4025-B2EB-3C8ED5ED529D")]
class CColorConverterFilter : public CTransformFilter
{
	bool m_deinterlace;
	BYTE* m_pic;

	HRESULT Transform(IMediaSample* pIn);

    HRESULT StartStreaming();
    HRESULT StopStreaming();

public:
	CColorConverterFilter(bool deinterlace = true);
	virtual ~CColorConverterFilter();

    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
    HRESULT Receive(IMediaSample* pIn);
};
