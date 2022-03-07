#include "stdafx.h"
#include "Deinterlacer.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include "BitBlt.h"
#include <initguid.h>
#include "moreuuids.h"

CDeinterlacerFilter::CDeinterlacerFilter(LPUNKNOWN punk, HRESULT* phr)
	: CTransformFilter(NAME("CDeinterlacerFilter"), punk, __uuidof(CDeinterlacerFilter))
{
	if(phr) *phr = S_OK;
}

HRESULT CDeinterlacerFilter::CheckConnect(PIN_DIRECTION dir, IPin* pPin)
{
	return DirectShow::GetCLSID(pPin) == __uuidof(*this) ? E_FAIL : S_OK;
}

HRESULT CDeinterlacerFilter::CheckInputType(const CMediaType* mtIn)
{
	BITMAPINFOHEADER bih;

	if(!CMediaTypeEx(*mtIn).ExtractBIH(&bih) /*|| bih.biHeight <= 0*/ || bih.biHeight <= 288)
	{
		return E_FAIL;
	}

	return mtIn->subtype == MEDIASUBTYPE_YUY2 || mtIn->subtype == MEDIASUBTYPE_UYVY
		|| mtIn->subtype == MEDIASUBTYPE_I420 || mtIn->subtype == MEDIASUBTYPE_YV12 || mtIn->subtype == MEDIASUBTYPE_IYUV 
		? S_OK 
		: E_FAIL;
}

HRESULT CDeinterlacerFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return mtIn->subtype == mtOut->subtype ? S_OK : E_FAIL;
}

HRESULT CDeinterlacerFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	HRESULT hr;

	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt != NULL)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	BYTE* pDataIn = NULL;

	if(FAILED(pIn->GetPointer(&pDataIn)) || !pDataIn)
	{
		return S_FALSE;
	}

	BYTE* pDataOut = NULL;

	if(FAILED(hr = pOut->GetPointer(&pDataOut)) || !pDataOut)
	{
		return hr;
	}

	const CMediaType& mtIn = m_pInput->CurrentMediaType();
	const CMediaType& mtOut = m_pOutput->CurrentMediaType();

	BITMAPINFOHEADER bihIn, bihOut;

	CMediaTypeEx(mtIn).ExtractBIH(&bihIn);
	CMediaTypeEx(mtOut).ExtractBIH(&bihOut);

	bool fInputFlipped = bihIn.biHeight >= 0 && bihIn.biCompression <= 3;
	bool fOutputFlipped = bihOut.biHeight >= 0 && bihOut.biCompression <= 3;
	bool fFlip = fInputFlipped != fOutputFlipped;

	int bppIn = !(bihIn.biBitCount & 7) ? bihIn.biBitCount : 8;
	int bppOut = !(bihOut.biBitCount & 7) ? bihOut.biBitCount : 8;
	int pitchIn = bihIn.biWidth*bppIn >> 3;
	int pitchOut = bihOut.biWidth*bppOut >> 3;

	BYTE* pDataOut2 = pDataOut;

	if(fFlip)
	{
		pDataOut2 += pitchOut * (bihIn.biHeight - 1); 
		pitchOut = -pitchOut;
	}

	if(mtIn.subtype == MEDIASUBTYPE_YUY2 || mtIn.subtype == MEDIASUBTYPE_UYVY)
	{
		Image::DeinterlaceBlend(pDataOut, pDataIn, pitchIn, bihIn.biHeight, pitchOut, pitchIn);
	}
	else if(mtIn.subtype == MEDIASUBTYPE_I420 || mtIn.subtype == MEDIASUBTYPE_YV12 || mtIn.subtype == MEDIASUBTYPE_IYUV)
	{
		Image::DeinterlaceBlend(pDataOut, pDataIn, pitchIn, bihIn.biHeight, pitchOut, pitchIn);

		int sizeIn = bihIn.biHeight * pitchIn;
		int sizeOut = abs(bihOut.biHeight) * pitchOut;

		pitchIn /= 2; 
		pitchOut /= 2;
		bihIn.biHeight /= 2;

		pDataIn += sizeIn; 
		pDataOut += sizeOut;

		Image::DeinterlaceBlend(pDataOut, pDataIn, pitchIn, bihIn.biHeight, pitchOut, pitchIn);

		pDataIn += sizeIn / 4; 
		pDataOut += sizeOut / 4;

		Image::DeinterlaceBlend(pDataOut, pDataIn, pitchIn, bihIn.biHeight, pitchOut, pitchIn);
	}

	return S_OK;
}

HRESULT CDeinterlacerFilter::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	BITMAPINFOHEADER bih;

	CMediaTypeEx(m_pOutput->CurrentMediaType()).ExtractBIH(&bih);

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = bih.biSizeImage;

	return S_OK;
}

HRESULT CDeinterlacerFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CMediaTypeEx mt(m_pInput->CurrentMediaType());
	
	mt.FixMediaType();

	*pmt = mt;

	return S_OK;
}