#include "StdAfx.h"
#include "ColorConverter.h"
#include "MediaTypeEx.h"
#include "BitBlt.h"
#include "../util/Vector.h"
#include <initguid.h>
#include "moreuuids.h"

CColorConverterFilter::CColorConverterFilter(bool deinterlace)
	: CTransformFilter(L"Color Converter", NULL, __uuidof(*this))
	, m_deinterlace(deinterlace)
	, m_pic(NULL)
{
}

CColorConverterFilter::~CColorConverterFilter()
{
	if(m_pic != NULL)
	{
		_aligned_free(m_pic);
	}
}

HRESULT CColorConverterFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

    if(m_pInput->SampleProps()->dwStreamId != AM_STREAM_MEDIA)
	{
		return m_pOutput->Deliver(pIn);
	}

	AM_MEDIA_TYPE* pmt;

	if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	return Transform(pIn);
}

HRESULT CColorConverterFilter::Transform(IMediaSample* pIn)
{
	HRESULT hr;

	CComPtr<IMediaSample> pOut;

	hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0);

	if(FAILED(hr))
	{
		return hr;
	}

	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt != NULL)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	pOut->SetDiscontinuity(FALSE);
	pOut->SetSyncPoint(TRUE);

	REFERENCE_TIME start, stop;
	
	if(SUCCEEDED(pIn->GetTime(&start, &stop)))
	{
		pOut->SetTime(&start, &stop);
	}

	BITMAPINFOHEADER bih;

	CMediaTypeEx(m_pInput->CurrentMediaType()).ExtractBIH(&bih);

	int w = bih.biWidth;
	int h = abs(bih.biHeight);

	pOut->SetActualDataLength(w * h * 3 / 2);

	BYTE* src = NULL;
	BYTE* dst = NULL;

	pIn->GetPointer(&src);
	pOut->GetPointer(&dst);

	if(bih.biCompression == 'YV12')
	{
		int size = w * h;

		if(m_deinterlace)
		{
			Image::DeinterlaceBlend(dst, src, w, h, w, w);

			src += size;
			dst += size;

			w >>= 1;
			h >>= 1;
			size >>= 2;

			Image::DeinterlaceBlend(dst, src, w, h, w, w);

			src += size;
			dst += size;

			Image::DeinterlaceBlend(dst, src, w, h, w, w);
		}
		else
		{
			memcpy(dst, src, size * 3 / 2);
		}
	}
	else if(bih.biCompression == '2YUY' || bih.biCompression == 'YVYU')
	{
		int srcpitch = w * 2;
		int dstpitch = w;

		if(m_deinterlace)
		{
			Image::DeinterlaceBlend(m_pic, src, w * 2, h, srcpitch, srcpitch);

			src = m_pic;
		}

		BYTE* s = src;
		BYTE* dy = dst;
		BYTE* du = dy + w * h;
		BYTE* dv = du + w * h / 4;

		if(bih.biCompression == '2YUY')
		{
			if(((UINT_PTR)s & 0xf) == 0 && (srcpitch & 0xf) == 0)
			{
				BYTE* s0 = src;
				BYTE* s1 = src + srcpitch;

				BYTE* dy0 = dy;
				BYTE* dy1 = dy + dstpitch;

				int dstpitch2 = dstpitch >> 1;

				for(int j = 0; j < h; j += 2, 
					s0 += srcpitch * 2, s1 += srcpitch * 2, 
					dy0 += dstpitch * 2, dy1 += dstpitch * 2, 
					du += dstpitch2, dv += dstpitch2)
				{
					for(int i = 0, j = 0; i < w; i += 16, j += 8)
					{
						Vector4i v00 = Vector4i::load<true>(&s0[i * 2]);
						Vector4i v01 = Vector4i::load<true>(&s0[i * 2 + 16]);
						Vector4i v10 = Vector4i::load<true>(&s1[i * 2]);
						Vector4i v11 = Vector4i::load<true>(&s1[i * 2 + 16]);
					
						Vector4i mask = Vector4i::x00ff();
					
						Vector4i::store<true>(&dy0[i], (v00 & mask).pu16(v01 & mask));
						Vector4i::store<true>(&dy1[i], (v10 & mask).pu16(v11 & mask));

						v00 = v00.srl16(8);
						v01 = v01.srl16(8);
						v10 = v10.srl16(8);
						v11 = v11.srl16(8);

						Vector4i uv = v00.pu16(v01).avg8(v10.pu16(v11));

						Vector4i u = uv.srl16(8);
						Vector4i v = uv & mask;

						Vector4i::storel(&du[j], v.pu16(v));
						Vector4i::storel(&dv[j], u.pu16(u));
					}
				}
			}
			else
			{
				for(int j = 0; j < h; j++, s += srcpitch, dy += dstpitch)
				{
					for(int i = 0; i < w; i += 2)
					{
						dy[i] = s[i * 2];
						dy[i + 1] = s[i * 2 + 2];
					}
				}

				BYTE* s0 = src;
				BYTE* s1 = src + srcpitch;

				srcpitch *= 2;
				dstpitch /= 2;
				w /= 2;
				h /= 2;

				for(int j = 0; j < h; j++, s0 += srcpitch, s1 += srcpitch, du += dstpitch, dv += dstpitch)
				{
					for(int i = 0; i < w; i++)
					{
						DWORD a = s0[i * 4 + 1];
						DWORD b = s1[i * 4 + 1];
						DWORD c = s0[i * 4 + 3];
						DWORD d = s1[i * 4 + 3];

						du[i] = (BYTE)((a + b) >> 1);
						dv[i] = (BYTE)((c + d) >> 1);
					}
				}
			}
		}
		else if(bih.biCompression == 'YVYU')
		{
			if(((UINT_PTR)s & 0xf) == 0 && (srcpitch & 0xf) == 0)
			{
				BYTE* s0 = src;
				BYTE* s1 = src + srcpitch;

				BYTE* dy0 = dy;
				BYTE* dy1 = dy + dstpitch;

				int dstpitch2 = dstpitch >> 1;

				for(int j = 0; j < h; j += 2, 
					s0 += srcpitch * 2, s1 += srcpitch * 2, 
					dy0 += dstpitch * 2, dy1 += dstpitch * 2, 
					du += dstpitch2, dv += dstpitch2)
				{
					for(int i = 0, j = 0; i < w; i += 16, j += 8)
					{
						Vector4i v00 = Vector4i::load<true>(&s0[i * 2]);
						Vector4i v01 = Vector4i::load<true>(&s0[i * 2 + 16]);
						Vector4i v10 = Vector4i::load<true>(&s1[i * 2]);
						Vector4i v11 = Vector4i::load<true>(&s1[i * 2 + 16]);
					
						Vector4i mask = Vector4i::x00ff();
					
						Vector4i::store<true>(&dy0[i], v00.srl16(8).pu16(v01.srl16(8)));
						Vector4i::store<true>(&dy1[i], v10.srl16(8).pu16(v11.srl16(8)));

						v00 = v00 & mask;
						v01 = v01 & mask;
						v10 = v10 & mask;
						v11 = v11 & mask;

						Vector4i uv = v00.pu16(v01).avg8(v10.pu16(v11));

						Vector4i u = uv.srl16(8);
						Vector4i v = uv & mask;

						Vector4i::storel(&du[j], v.pu16(v));
						Vector4i::storel(&dv[j], u.pu16(u));
					}
				}
			}
			else
			{
				for(int j = 0; j < h; j++, s += srcpitch, dy += dstpitch)
				{
					for(int i = 0; i < w; i += 2)
					{
						dy[i] = s[i * 2 + 1];
						dy[i + 1] = s[i * 2 + 3];
					}
				}

				BYTE* s0 = src;
				BYTE* s1 = src + srcpitch;

				srcpitch *= 2;
				dstpitch /= 2;
				w /= 2;
				h /= 2;

				for(int j = 0; j < h; j++, s0 += srcpitch, s1 += srcpitch, du += dstpitch, dv += dstpitch)
				{
					for(int i = 0; i < w; i++)
					{
						DWORD a = s0[i * 4 + 0];
						DWORD b = s1[i * 4 + 0];
						DWORD c = s0[i * 4 + 2];
						DWORD d = s1[i * 4 + 2];

						du[i] = (BYTE)((a + b) >> 1);
						dv[i] = (BYTE)((c + d) >> 1);
					}
				}
			}
		}
	}

	m_pOutput->Deliver(pOut);

	return S_OK;
}

HRESULT CColorConverterFilter::StartStreaming()
{
	if(m_pic != NULL)
	{
		_aligned_free(m_pic);

		m_pic = NULL;
	}

	if(m_pInput->IsConnected())
	{
		BITMAPINFOHEADER bih;

		CMediaTypeEx(m_pInput->CurrentMediaType()).ExtractBIH(&bih);

		m_pic = (BYTE*)_aligned_malloc(bih.biWidth * abs(bih.biHeight) * bih.biBitCount >> 3, 16);
	}

	return __super::StartStreaming();
}

HRESULT CColorConverterFilter::StopStreaming()
{
	if(m_pic != NULL)
	{
		_aligned_free(m_pic);

		m_pic = NULL;
	}

	return __super::StopStreaming();
}

HRESULT CColorConverterFilter::CheckInputType(const CMediaType* mtIn)
{
	BITMAPINFOHEADER bih;

	CMediaTypeEx(*mtIn).ExtractBIH(&bih);

	return mtIn->majortype == MEDIATYPE_Video 
		&& (mtIn->subtype == MEDIASUBTYPE_YV12 
		 || mtIn->subtype == MEDIASUBTYPE_YUY2 
		 || mtIn->subtype == MEDIASUBTYPE_UYVY)
		&& mtIn->formattype == FORMAT_VideoInfo
		&& bih.biHeight > 0
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CColorConverterFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(FAILED(CheckInputType(mtIn)) || mtOut->majortype != MEDIATYPE_Video)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if(mtIn->majortype == MEDIATYPE_Video 
	&& (mtIn->subtype == MEDIASUBTYPE_YV12 
	 || mtIn->subtype == MEDIASUBTYPE_YUY2 
	 || mtIn->subtype == MEDIASUBTYPE_UYVY))
	{
		if(mtOut->subtype != MEDIASUBTYPE_I420
		&& mtOut->subtype != MEDIASUBTYPE_IYUV)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return S_OK;
}

HRESULT CColorConverterFilter::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	BITMAPINFOHEADER bih;

	CMediaTypeEx(m_pOutput->CurrentMediaType()).ExtractBIH(&bih);

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = bih.biWidth * bih.biHeight * 3 / 2;

    return S_OK;
}

HRESULT CColorConverterFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	struct {const GUID* subtype; WORD biBitCount; DWORD biCompression;} fmts[] =
	{
		{&MEDIASUBTYPE_I420, 12, '024I'},
		{&MEDIASUBTYPE_IYUV, 12, 'VUYI'},
	};

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= sizeof(fmts) / sizeof(fmts[0])) return VFW_S_NO_MORE_ITEMS;

	CMediaType& mt = m_pInput->CurrentMediaType();

	BITMAPINFOHEADER bih;
	
	CMediaTypeEx(mt).ExtractBIH(&bih);

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = *fmts[iPosition].subtype;
	pmt->formattype = FORMAT_VideoInfo;

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));

	memset(vih, 0, sizeof(VIDEOINFOHEADER));

	vih->AvgTimePerFrame = ((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame;
	vih->dwBitRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitRate;
	vih->dwBitErrorRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitErrorRate;
	vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
	vih->bmiHeader.biWidth = bih.biWidth;
	vih->bmiHeader.biHeight = abs(bih.biHeight);
	vih->bmiHeader.biPlanes = 1;
	vih->bmiHeader.biBitCount = fmts[iPosition].biBitCount;
	vih->bmiHeader.biCompression = fmts[iPosition].biCompression;
	vih->bmiHeader.biSizeImage = vih->bmiHeader.biWidth * vih->bmiHeader.biHeight * vih->bmiHeader.biBitCount >> 3;
	vih->bmiHeader.biXPelsPerMeter = vih->bmiHeader.biWidth * 3;
	vih->bmiHeader.biYPelsPerMeter = vih->bmiHeader.biHeight * 4;

	return S_OK;
}

