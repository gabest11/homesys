#include "stdafx.h"
#include "TeletextFilter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "../directshow/moreuuids.h"

#include "Teletext.inc"

// CTeletextFilter

CTeletextFilter::CTeletextFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseFilter(NAME("CTeletextFilter"), lpunk, this, __uuidof(this))
{
	HRESULT hr;

	m_input = new CInputPin(this, &hr);
	m_output = new COutputPin(this, &hr);
}

CTeletextFilter::~CTeletextFilter()
{
	delete m_input;
	delete m_output;
}

int CTeletextFilter::GetPinCount()
{
	return 2;
}

CBasePin* CTeletextFilter::GetPin(int n)
{
	switch(n)
	{
	case 0: return m_input;
	case 1: return m_output;
	}
	
	return NULL;
}

STDMETHODIMP CTeletextFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

CTeletextFilter::CInputPin::CInputPin(CTeletextFilter* pFilter, HRESULT* phr)
	: CBaseInputPin(NAME("CInputPin"), pFilter, pFilter, phr, L"Input")
{
}

CTeletextFilter::CInputPin::~CInputPin()
{
	for(auto i = m_rows.begin(); i != m_rows.end(); i++)
	{
		delete *i;
	}
}

struct VBIINFOHEADER
{
    ULONG       StartLine;              // inclusive
    ULONG       EndLine;                // inclusive
    ULONG       SamplingFrequency;      // Hz.
    ULONG       MinLineStartTime;       // microSec * 100 from HSync LE
    ULONG       MaxLineStartTime;       // microSec * 100 from HSync LE
    ULONG       ActualLineStartTime;    // microSec * 100 from HSync LE
    ULONG       ActualLineEndTime;      // microSec * 100 from HSync LE
    ULONG       VideoStandard;          // KS_AnalogVideoStandard*
    ULONG       SamplesPerLine;
    ULONG       StrideInBytes;          // May be > SamplesPerLine
    ULONG       BufferSize;             // Bytes
};

HRESULT CTeletextFilter::CInputPin::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->majortype == MEDIATYPE_TeletextPacket
	//|| pmt->majortype == MEDIATYPE_VBI && pmt->subtype == KSDATAFORMAT_SUBTYPE_RAW8
	) 
	{
		KS_VBIINFOHEADER* h = (KS_VBIINFOHEADER*)pmt->pbFormat;
		VBIINFOHEADER* h2 = (VBIINFOHEADER*)pmt->pbFormat;

		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

STDMETHODIMP CTeletextFilter::CInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	
	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CTeletextFilter::CInputPin::Receive(IMediaSample* pIn)
{
	HRESULT hr;

	CAutoLock cAutoLock(&m_csReceive);

	REFERENCE_TIME start = 0, stop = 0;

    hr = pIn->GetTime(&start, &stop);

	BYTE* src = NULL;

    hr = pIn->GetPointer(&src);

    if(FAILED(hr) || src == NULL) 
	{
		return hr;
	}

	int len = pIn->GetActualDataLength();

	if(len <= 0 || src == NULL)
	{
		return S_FALSE;
	}

	if(m_mt.majortype == MEDIATYPE_TeletextPacket)
	{
		if(src[0] < 0x10 || src[0] >= 0x20) // private stream id
		{
			return S_FALSE;
		}

		src++; 
		len--;

		for(int i = 0; i <= len - 46; i += 46)
		{
			if(src[i + 1] != 44) break;

			row_t* r = new row_t();

			r->start = start;
			r->stop = stop;

			for(int j = 0; j < 42; j++)
			{
				r->row[j] = invtab[src[i + 4 + j]];
			}

			r->row[42] = 0;

			m_rows.push_back(r);
		}

		CBaseOutputPin* pin = ((CTeletextFilter*)m_pFilter)->m_output;

		while(1)
		{
			CComPtr<IMediaSample> pOut;

			hr = pin->GetDeliveryBuffer(&pOut, NULL, NULL, 0);

			if(FAILED(hr) || pOut == NULL) return hr;

			long n = pOut->GetSize() / 43;

			if(m_rows.size() < n)
			{
				break;
			}

			row_t* first = m_rows.front();

			if(first->start != 0 && first->stop != 0)
			{
				pOut->SetTime(&first->start, &first->stop);
			}

			BYTE* dst = NULL;

			pOut->GetPointer(&dst);

			int i = 0;

			while(n-- > 0)
			{
				row_t* r = m_rows.front();

				m_rows.pop_front();

				memcpy(&dst[i], r->row, 43);

				i += 43;

				delete r;
			}

			pOut->SetActualDataLength(i);

			hr = pin->Deliver(pOut);

			if(hr != S_OK) return hr;
		}
	}
	else // if(m_mt.majortype == MEDIATYPE_VBI && m_mt.subtype == KSDATAFORMAT_SUBTYPE_RAW8)
	{
		// TODO

		int i = 0;
	}

	return S_OK;
}

STDMETHODIMP CTeletextFilter::CInputPin::EndOfStream()
{
	return __super::EndOfStream();
}

STDMETHODIMP CTeletextFilter::CInputPin::BeginFlush()
{
	HRESULT hr = __super::BeginFlush();

	((CTeletextFilter*)m_pFilter)->m_output->DeliverBeginFlush();

	return hr;
}

STDMETHODIMP CTeletextFilter::CInputPin::EndFlush()
{
	((CTeletextFilter*)m_pFilter)->m_output->DeliverEndFlush();

	return __super::EndFlush();
}

CTeletextFilter::COutputPin::COutputPin(CTeletextFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(NAME("COutputPin"), pFilter, pFilter, phr, L"Output")
{
}

HRESULT CTeletextFilter::COutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_VBI && pmt->subtype == MEDIASUBTYPE_TELETEXT ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTeletextFilter::COutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* props)
{
	props->cbBuffer = 20 * 43;
	props->cBuffers = 1;

	return S_OK;
}

HRESULT CTeletextFilter::COutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pLock);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->InitMediaType();

	pmt->majortype = MEDIATYPE_VBI;
	pmt->subtype = MEDIASUBTYPE_TELETEXT;

	return S_OK;
}

// CTeletextRenderer

CTeletextRenderer::CTeletextRenderer(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseRenderer(__uuidof(this), NAME("CTeletextRenderer"), lpunk, phr)
{
	m_has_page = false;
	m_erase_page = false;
	m_page = 0x100;
	m_page_new = 0;
	m_magazine = new magazine_t[8];
	m_image = new BYTE[40 * 25 * 13 * 16];
	memset(m_image, 0, 40 * 25 * 13 * 16);
	memset(m_box, 0, sizeof(m_box));
	
	m_clut[0] = 0x000000; // black
	m_clut[1] = 0x0000ff; // red
	m_clut[2] = 0x00ff00; // green
	m_clut[3] = 0x00ffff; // yellow
	m_clut[4] = 0xff0000; // blue
	m_clut[5] = 0xff00ff; // magenta
	m_clut[6] = 0xffff00; // cyan
	m_clut[7] = 0xffffff; // white
}

CTeletextRenderer::~CTeletextRenderer()
{
	delete [] m_magazine;
	delete [] m_image;

	for(auto i = m_cache.begin(); i != m_cache.end(); i++)
	{
		delete [] *i;
	}
}

STDMETHODIMP CTeletextRenderer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ITeletextRenderer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CTeletextRenderer::DoRenderSample(IMediaSample* pSample)
{
	HRESULT hr;

	CAutoLock cAutoLock(this);

	REFERENCE_TIME start = 0, stop = 0;

    hr = pSample->GetTime(&start, &stop);

	BYTE* src = NULL;

    hr = pSample->GetPointer(&src);

    if(FAILED(hr) || src == NULL) 
	{
		return hr;
	}

	int len = pSample->GetActualDataLength();

	if(len != 20 * 43 || src == NULL)
	{
		return S_FALSE;
	}

	for(int i = 0; i < 20; i++)
	{
		src += 43;

		BYTE* tmp = new BYTE[42];

		tmp[0] = unham(src[0], src[1]);;

		memcpy(&tmp[1], &src[2], 41);

		m_cache.push_back(tmp); 

		AddLine(tmp[0] & 7, (tmp[0] >> 3) & 0x1f, &tmp[1]);
	}

	while(m_cache.size() > 800 * 2 * 32)
	{
		delete [] m_cache.front();

		m_cache.pop_front();
	}

	return S_OK;
}

HRESULT CTeletextRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_VBI && pmt->subtype == MEDIASUBTYPE_TELETEXT ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

void CTeletextRenderer::AddLine(BYTE magazine, BYTE packet, BYTE* buff)
{
	if(magazine >= 8 || packet >= 32) return;

	magazine_t& m = m_magazine[magazine];

	if(packet == 0)
	{
		m_has_page = true;

		m.page = unham(buff[0], buff[1]);
        m.subpage = ((unham(buff[4], buff[5]) << 8) | unham(buff[2], buff[3])) & 0x3F7F;   
        m.language = (unham(buff[6], buff[7]) >> 5) & 0x07;
		m.packet = 0;

		m.erase_page = (buff[3] & 0x80) == 0x80; // Byte 9,  bit 8
		m.newsflash = (buff[5] & 0x20) == 0x20; // Byte 11, bit 6
		m.subtitle = (buff[5] & 0x80) == 0x80; // Byte 11, bit 8
		m.supresss_hader = (buff[6] & 0x02) == 0x02; // Byte 12, bit 2
		m.update_indicator = (buff[6] & 0x08) == 0x08; // Byte 12, bit 4
		m.interrupted_sequence = (buff[6] & 0x20) == 0x20; // Byte 12, bit 6
		m.inhibit_display = (buff[6] & 0x80) == 0x80; // Byte 12, bit 8
		m.serial = (buff[7] & 0x02) == 0x02; // Byte 13, bit 2

		if(m.erase_page)
		{
			for(int i = 0; i < sizeof(m.row) / sizeof(m.row[0]); i++)
			{
				m.row[i].Reset();
			}

			if((m_page & 0x7ff) == ((magazine << 8) | m.page))
			{
				memset(m_image, 0, 40 * 25 * 13 * 16);
				memset(m_box, m.subtitle ? 1 : 0, sizeof(m_box));
			}
		}

		if(m_erase_page)
		{
			if((m_page & 0x7ff) == ((magazine << 8) | m.page))
			{
				m_erase_page = false;

				memset(m_image, 0, 40 * 25 * 13 * 16);
				memset(m_box, m.subtitle ? 1 : 0, sizeof(m_box));
			}
		}

		row_t& r = m_row0;

		r.Reset();

		for(int i = 0; i < 8; i++)
		{
			r.text[i] = 0x20;
		}

		if(m_page_new & 0xfff)
		{
			int i = 1;

			if(m_page_new & 0xf00) r.text[i++] = 0x30 | ((m_page_new >> 8) & 0xf);
			if(m_page_new & 0xff0) r.text[i++] = 0x30 | ((m_page_new >> 4) & 0xf);
			if(m_page_new & 0xfff) r.text[i++] = 0x30 | ((m_page_new >> 0) & 0xf);
		}
		else
		{
			r.text[1] = 0x30 | ((m_page >> 8) & 0xf);
			r.text[2] = 0x30 | ((m_page >> 4) & 0xf);
			r.text[3] = 0x30 | ((m_page >> 0) & 0xf);
		}

		for(int i = 8; i < 40; i++)
		{
			r.text[i] = buff[i] & 0x7f;
		}

		UpdateImageRow(0, r, m);
	}
	else if(packet < 25)
	{
		//if(packet <= m.packet) return;

		m.packet = packet;

		row_t& r = m.row[packet - 1];

		r.Reset();

		for(int i = 0; i < 40; i++)
		{
			r.text[i] = buff[i] & 0x7f;
		}

		if((m_page & 0x7ff) == ((magazine << 8) | m.page))
		{
			if(!(packet >= 2 ? m.row[packet - 2] : m_row0).dblheight)
			{
				UpdateImageRow(packet, r, m);
			}
		}
	}
	else
	{
		// TODO

		if(packet == 25)
		{
		}
		else if(packet == 26)
		{
		}
		else if(packet == 27)
		{
			if((m_page & 0x7ff) == ((magazine << 8) | m.page))
			{
				BYTE id = unham(*buff++);

				for(int i = 0; i < 6; i++, buff += 6)
				{
					BYTE M1 = (buff[3] & 0x80) >> 7;
					BYTE M2 = (buff[5] & 0x20) >> 4;
					BYTE M3 = (buff[5] & 0x80) >> 5;

					m.link[i].magazine = (magazine ^ (M1 | M2 | M3)) & 7; 
					m.link[i].page = unham(buff[0], buff[1]);
					m.link[i].subpage = ((unham(buff[4], buff[5]) << 8) | unham(buff[2], buff[3])) & 0x3F7F;   
				}
			}
		}
		else
		{
		}
	}
}

void CTeletextRenderer::UpdateImageRow(BYTE packet, row_t& r, magazine_t& m)
{
	if(packet > 24) return;

	if(packet == 2)
	{
		DWORD sum = 0;
		for(int i = 0; i < 40; i++) sum += r.text[i];
		if(sum == 0) return;
	}

	BYTE* p = &m_image[packet * 40 * 13 * 16];

	WORD (*charset)[16] = tt_g0;
	BYTE color[2] = {r.color[0], r.color[1]};
	BYTE size = 0;

	for(int i = 0; i < 40; i++, p += 13)
	{
		BYTE c = r.text[i];

		if(c >= 0x20)
		{
			WORD* RESTRICT src = charset[c - 0x20]; // TODO
			BYTE* RESTRICT dst = p;

			if(charset == tt_g0)
			{
				if(m.language == 2) // TODO
				switch(c)
				{
				case 0x23: src = tt_g0_nat[m.language][0]; break;
				case 0x24: src = tt_g0_nat[m.language][1]; break;
				case 0x40: src = tt_g0_nat[m.language][2]; break;
				case 0x5b: src = tt_g0_nat[m.language][3]; break;
				case 0x5c: src = tt_g0_nat[m.language][4]; break;
				case 0x5d: src = tt_g0_nat[m.language][5]; break;
				case 0x5e: src = tt_g0_nat[m.language][6]; break;
				case 0x5f: src = tt_g0_nat[m.language][7]; break;
				case 0x60: src = tt_g0_nat[m.language][8]; break;
				case 0x7b: src = tt_g0_nat[m.language][9]; break;
				case 0x7c: src = tt_g0_nat[m.language][10]; break;
				case 0x7d: src = tt_g0_nat[m.language][11]; break;
				case 0x7e: src = tt_g0_nat[m.language][12]; break;
				}
			}

			if(size == 0)
			{
				for(int j = 0; j < 16; j++, dst += 13 * 40)
				{
					dst[0] = color[(src[j] >> 0) & 1];
					dst[1] = color[(src[j] >> 1) & 1];
					dst[2] = color[(src[j] >> 2) & 1];
					dst[3] = color[(src[j] >> 3) & 1];
					dst[4] = color[(src[j] >> 4) & 1];
					dst[5] = color[(src[j] >> 5) & 1];
					dst[6] = color[(src[j] >> 6) & 1];
					dst[7] = color[(src[j] >> 7) & 1];
					dst[8] = color[(src[j] >> 8) & 1];
					dst[9] = color[(src[j] >> 9) & 1];
					dst[10] = color[(src[j] >> 10) & 1];
					dst[11] = color[(src[j] >> 11) & 1];
					dst[12] = color[(src[j] >> 12) & 1];
				}
			}
			else if(size == 1)
			{
				for(int j = 0; j < 16; j++, dst += 26 * 40)
				{
					dst[0] = dst[0 + 13 * 40] = color[(src[j] >> 0) & 1];
					dst[1] = dst[1 + 13 * 40] = color[(src[j] >> 1) & 1];
					dst[2] = dst[2 + 13 * 40] = color[(src[j] >> 2) & 1];
					dst[3] = dst[3 + 13 * 40] = color[(src[j] >> 3) & 1];
					dst[4] = dst[4 + 13 * 40] = color[(src[j] >> 4) & 1];
					dst[5] = dst[5 + 13 * 40] = color[(src[j] >> 5) & 1];
					dst[6] = dst[6 + 13 * 40] = color[(src[j] >> 6) & 1];
					dst[7] = dst[7 + 13 * 40] = color[(src[j] >> 7) & 1];
					dst[8] = dst[8 + 13 * 40] = color[(src[j] >> 8) & 1];
					dst[9] = dst[9 + 13 * 40] = color[(src[j] >> 9) & 1];
					dst[10] = dst[10 + 13 * 40] = color[(src[j] >> 10) & 1];
					dst[11] = dst[11 + 13 * 40] = color[(src[j] >> 11) & 1];
					dst[12] = dst[12 + 13 * 40] = color[(src[j] >> 12) & 1];
				}
			}
			else if(size == 2)
			{
				for(int j = 0; j < 16; j++, dst += 13 * 40)
				{
					dst[0] = dst[1] = color[(src[j] >> 0) & 1];
					dst[2] = dst[3] = color[(src[j] >> 1) & 1];
					dst[4] = dst[5] = color[(src[j] >> 2) & 1];
					dst[6] = dst[7] = color[(src[j] >> 3) & 1];
					dst[8] = dst[9] = color[(src[j] >> 4) & 1];
					dst[10] = dst[11] = color[(src[j] >> 5) & 1];
					dst[12] = dst[13] = color[(src[j] >> 6) & 1];
					dst[14] = dst[15] = color[(src[j] >> 7) & 1];
					dst[16] = dst[17] = color[(src[j] >> 8) & 1];
					dst[18] = dst[19] = color[(src[j] >> 9) & 1];
					dst[20] = dst[21] = color[(src[j] >> 10) & 1];
					dst[22] = dst[23] = color[(src[j] >> 11) & 1];
					dst[24] = dst[25] = color[(src[j] >> 12) & 1];
				}

				if(c != 0x20)
				{
					i++;
					p += 13;
				}
			}
			else if(size == 3)
			{
				for(int j = 0; j < 16; j++, dst += 26 * 40)
				{
					dst[0] = dst[1] = dst[0 + 13 * 40] = dst[1 + 13 * 40] = color[(src[j] >> 0) & 1];
					dst[2] = dst[3] = dst[2 + 13 * 40] = dst[3 + 13 * 40] = color[(src[j] >> 1) & 1];
					dst[4] = dst[5] = dst[4 + 13 * 40] = dst[5 + 13 * 40] = color[(src[j] >> 2) & 1];
					dst[6] = dst[7] = dst[6 + 13 * 40] = dst[7 + 13 * 40] = color[(src[j] >> 3) & 1];
					dst[8] = dst[9] = dst[8 + 13 * 40] = dst[9 + 13 * 40] = color[(src[j] >> 4) & 1];
					dst[10] = dst[11] = dst[10 + 13 * 40] = dst[11 + 13 * 40] = color[(src[j] >> 5) & 1];
					dst[12] = dst[13] = dst[12 + 13 * 40] = dst[13 + 13 * 40] = color[(src[j] >> 6) & 1];
					dst[14] = dst[15] = dst[14 + 13 * 40] = dst[15 + 13 * 40] = color[(src[j] >> 7) & 1];
					dst[16] = dst[17] = dst[16 + 13 * 40] = dst[17 + 13 * 40] = color[(src[j] >> 8) & 1];
					dst[18] = dst[19] = dst[18 + 13 * 40] = dst[19 + 13 * 40] = color[(src[j] >> 9) & 1];
					dst[20] = dst[21] = dst[20 + 13 * 40] = dst[21 + 13 * 40] = color[(src[j] >> 10) & 1];
					dst[22] = dst[23] = dst[22 + 13 * 40] = dst[23 + 13 * 40] = color[(src[j] >> 11) & 1];
					dst[24] = dst[25] = dst[24 + 13 * 40] = dst[25 + 13 * 40] = color[(src[j] >> 12) & 1];
				}

				if(c != 0x20)
				{
					i++;
					p += 13;
				}
			}
		}
		else
		{
			if(c >= 0 && c < 8)
			{
				color[1] = c;

				charset = tt_g0;
			}
			if(c >= 16 && c < 24)
			{
				color[1] = c - 16;

				charset = tt_g1;
			}
			else if(c >= 12 && c < 16)
			{
				// normal, double height, double width, double size

				size = c - 12; 

				if(size == 1 || size == 3)
				{
					r.dblheight = 1;
				}
			}
			else if(c == 8)
			{
				// start flash
			}
			else if(c == 9)
			{
				// end flash
			}
			else if(c == 10)
			{
				// end box

				m_box[packet].end = i;
			}
			else if(c == 11)
			{
				// start box

				m_box[packet].start = i;
			}
			else if(c == 24)
			{
				// conceal
			}
			else if(c == 25)
			{
				// mosaic

				charset = tt_g1;
			}
			else if(c == 26)
			{
				// separated mosaic

				charset = tt_g1_sep;
			}
			else if(c == 27)
			{
				// escape
			}
			else if(c == 28)
			{
				// black background

				color[0] = 0;
			}
			else if(c == 29)
			{
				// new background

				color[0] = color[1];
			}
			else if(c == 30)
			{
				// hold mosaics
			}
			else if(c == 31)
			{
				// release mosaics
			}

			BYTE* RESTRICT dst = p;

			for(int j = 0; j < 16; j++, dst += 13 * 40)
			{
				memset(dst, color[0], 13);
			}
		}
	}

	if(r.dblheight && packet < 24)
	{
		BYTE* p = &m_image[(packet + 1) * 40 * 13 * 16];
	
		BYTE color[2] = {r.color[0], r.color[1]};
		BYTE size = 0;

		for(int i = 0; i < 40; i++, p += 13)
		{
			BYTE c = r.text[i];

			if(c >= 0 && c < 8)
			{
				color[1] = c;
			}
			if(c >= 16 && c < 24)
			{
				color[1] = c - 16;
			}
			else if(c >= 12 && c < 16)
			{
				// normal, double height, double width, double size

				size = c - 12; 
			}
			else if(c == 8)
			{
				// start flash
			}
			else if(c == 9)
			{
				// end flash
			}
			else if(c == 10)
			{
				// end box

				m_box[packet + 1].end = i;
			}
			else if(c == 11)
			{
				// start box

				m_box[packet + 1].start = i;
			}
			else if(c == 28)
			{
				// black background

				color[0] = 0;
			}
			else if(c == 29)
			{
				// new background

				color[0] = color[1];
			}

			if(c < 0x20 || size != 1 && size != 3)
			{
				BYTE* RESTRICT dst = p;

				for(int j = 0; j < 16; j++, dst += 13 * 40)
				{
					memset(dst, color[0], 13);
				}			
			}
		}
	}

	// if(packet >= 24) SaveImage(L"d:\\tt.jpg");
}

#include "JPEGEncoder.h"

void CTeletextRenderer::SaveImage(const wchar_t* fn)
{
	BYTE* buff = new BYTE[sizeof(BITMAPINFOHEADER) + 40 * 13 * 25 * 16 * 4];

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)buff;

	bih->biBitCount = 32;
	bih->biCompression = 0;
	bih->biHeight = 25 * 16;
	bih->biPlanes = 1;
	bih->biSize = sizeof(bih);
	bih->biSizeImage = 40 * 13 * 25 * 16 * 4;
	bih->biWidth = 40 * 13;

	BYTE* src = m_image;
	DWORD* dst = (DWORD*)(bih + 1);

	for(int j = 0; j < 25 * 16; j++)
	{
		for(int i = 0; i < 40 * 13; i++)
		{
			DWORD c = m_clut[*src++];

			*dst++ = ((c & 0xff) << 16) | (c & 0xff00) | (c >> 16);
		}
	}

	CJPEGEncoderFile e(fn);

	e.Encode(buff);

	delete [] buff;
}

// ITeletextRenderer

STDMETHODIMP CTeletextRenderer::HasPage()
{
	CAutoLock cAutoLock(this);

	return m_has_page ? S_OK : S_FALSE;
}

STDMETHODIMP CTeletextRenderer::GetPage(DWORD& page)
{
	CAutoLock cAutoLock(this);

	page = m_page;

	return S_OK;
}

STDMETHODIMP CTeletextRenderer::SetPage(DWORD page)
{
	CAutoLock cAutoLock(this);

	m_page = page;

	m_erase_page = true;

	BYTE magazine = (BYTE)(page >> 8);

	if(magazine == 8) magazine = 0;

	for(auto i = m_cache.begin(); i != m_cache.end(); i++)
	{
		BYTE* tmp = *i;

		if((tmp[0] & 7) == magazine)
		{
			AddLine(tmp[0] & 7, (tmp[0] >> 3) & 0x1f, &tmp[1]);
		}
	}

	return S_OK;
}

STDMETHODIMP CTeletextRenderer::EnterPage(BYTE digit)
{
	CAutoLock cAutoLock(this);

	if(digit > 9 || digit == 0 && m_page_new == 0)
	{
		return E_INVALIDARG;
	}

	m_page_new = ((m_page_new << 4) & 0xff0) | digit;

	if(m_page_new & 0xf00)
	{
		DWORD page = m_page_new;

		m_page_new = 0;

		SetPage(page);
	}

	return S_OK;
}

STDMETHODIMP CTeletextRenderer::JumpToLink(BYTE index)
{
	if(index >= 6) return E_INVALIDARG;

	CAutoLock cAutoLock(this);

	magazine_t& m = m_magazine[(m_page >> 8) & 0x7];

	link_t& l = m.link[index];

	if((l.page & 0x0f) > 0x09) return S_FALSE;
	if((l.page & 0xf0) > 0x99) return S_FALSE;
	if(l.magazine > 7) return S_FALSE;

	DWORD page = ((l.magazine == 0 ? 8 : l.magazine) << 8) | l.page;

	SetPage(page);

	return S_OK;
}

STDMETHODIMP CTeletextRenderer::GetImage(BYTE* buff, int pitch)
{
	CAutoLock cAutoLock(this);

	bool boxed = false;

	for(int i = 0; i < 25; i++)
	{
		if(m_box[i].end > 0)
		{
			boxed = true;

			break;
		}
	}

	DWORD a = boxed ? 0 : 0xff000000;

	BYTE* RESTRICT src = m_image;
	DWORD* RESTRICT clut = m_clut;
	box_t* RESTRICT box = m_box;

	for(int j = 0; j < 25; j++, box++)
	{
		for(int jj = 0; jj < 16; jj++, buff += pitch)
		{
			DWORD* RESTRICT dst = (DWORD*)buff;

			for(int i = 0; i < 40; i++)
			{
				DWORD aa = box->start <= i && i < box->end ? 0xff000000 : a;

				for(int ii = 0; ii < 13; ii++)
				{
					BYTE b = *src++;

					DWORD c = clut[b & 7];

					*dst++ = aa | ((c & 0xff) << 16) | (c & 0xff00) | (c >> 16);
				}
			}
		}
	}

	return S_OK;
}
