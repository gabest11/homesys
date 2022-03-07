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

#include "StdAfx.h"
#include "VobSubProvider.h"
#include "../util/String.h"

// CVobSubImage

CVobSubImage::CVobSubImage()
	: m_lang(0)
	, m_cookie(0)
	, m_forced(false)
	, m_start(0)
	, m_delay(0)
	, m_pixels(NULL)
{
	memset(m_buff, 0, sizeof(m_buff));

	m_rect = Vector4i::zero();
	m_size = Vector2i(0, 0);
}

CVobSubImage::~CVobSubImage()
{
	Free();
}

void CVobSubImage::Alloc(int w, int h)
{
	// if there is nothing to crop TrimSubImage might even add a 1 pixel
	// wide border around the text, that's why we need a bit more memory
	// to be allocated.

	if(m_buff[0] == NULL || w * h > m_size.x * m_size.y)
	{
		Free();

		m_buff[0] = new RGBQUAD[w * h];
		m_buff[1] = new RGBQUAD[(w + 2) * (h + 2)];

		m_size.x = w; 
		m_size.y = h;
	}

	m_pixels = m_buff[0];
}

void CVobSubImage::Free()
{
	delete [] m_buff[0];
	delete [] m_buff[1];
	
	m_buff[0] = NULL;
	m_buff[1] = NULL;

	m_pixels = NULL;
}

bool CVobSubImage::Decode(BYTE* buff, int packetsize, int datasize, bool custompal, int tridx, RGBQUAD* org, RGBQUAD* cus, bool trim)
{
	GetPacketInfo(buff, packetsize, datasize);

	Alloc(m_rect.width(), m_rect.height());

	m_pixels = m_buff[0];

	m_field = 0;
	m_aligned = 1;

	m_custompal = custompal;
	m_palette.org = org;
	m_palette.cus = cus;
	m_tridx = m_tridx;

	Vector2i p = m_rect.tl;

	DWORD offset[2] = {m_offset[0], m_offset[1]};
	DWORD end[2] = {m_offset[1], datasize};

	while(offset[m_field] < end[m_field])
	{
		DWORD code;

		if((code = GetNibble(buff, offset)) >= 0x4
		|| (code = (code << 4) | GetNibble(buff, offset)) >= 0x10
		|| (code = (code << 4) | GetNibble(buff, offset)) >= 0x40
		|| (code = (code << 4) | GetNibble(buff, offset)) >= 0x100)
		{
			DrawPixels(p, code >> 2, code & 3);

			if((p.x += code >> 2) < m_rect.right) 
			{
				continue;
			}
		}

		DrawPixels(p, m_rect.right - p.x, code & 3);

		if(!m_aligned) GetNibble(buff, offset); // align to byte

		p.x = m_rect.left;
		p.y++;

		m_field = 1 - m_field;
	}

	m_rect.bottom = std::min<int>(p.y, m_rect.bottom);

	if(trim) 
	{
		TrimSubImage();
	}

	return true;
}

void CVobSubImage::GetPacketInfo(BYTE* buff, int packetsize, int datasize)
{
//	delay = 0;

	int i, next = datasize;

	WORD pal = 0, tr = 0;

	do
	{
		i = next;

		int pts = (buff[i] << 8) | buff[i + 1]; i += 2;

		next = (buff[i] << 8) | buff[i + 1]; i += 2;

		if(next > packetsize || next < datasize)
		{
			return;
		}

		for(bool fBreak = false; !fBreak; )
		{
			int len = 0;

			switch(buff[i])
			{
				case 0x00: len = 0; break;
				case 0x01: len = 0; break;
				case 0x02: len = 0; break;
				case 0x03: len = 2; break;
				case 0x04: len = 2; break;
				case 0x05: len = 6; break;
				case 0x06: len = 4; break;
				case 0x07: len = 0; break; // TODO
				default: len = 0; break;
			}

			if(i + len >= packetsize)
			{
				break;
			}

			switch(buff[i++])
			{
				case 0x00: // forced start displaying
					m_forced = true;
					break;
				case 0x01: // start displaying
					m_forced = false;
					break;
				case 0x02: // stop displaying
					m_delay = 10240000i64 * pts / 90;
					break;
				case 0x03:
					pal = (buff[i] << 8) | buff[i+1]; i += 2;
					break;
				case 0x04:
					tr = (buff[i] << 8) | buff[i+1]; i += 2;
					break;
				case 0x05:
					m_rect = Vector4i(
						(buff[i + 0] << 4) + (buff[i + 1] >> 4), 
						(buff[i + 3] << 4) + (buff[i + 4] >> 4), 
						((buff[i + 1] & 0x0f) << 8) + buff[i + 2] + 1, 
						((buff[i + 4] & 0x0f) << 8) + buff[i + 5] + 1);
					i += 6;
					break;
				case 0x06:
					m_offset[0] = (buff[i] << 8) + buff[i + 1]; i += 2;
					m_offset[1] = (buff[i] << 8) + buff[i + 1]; i += 2;
					break;
				case 0xff: // end of ctrlblk
					fBreak = true;
					continue;
				default: // skip this ctrlblk
					fBreak = true;
					break;
			}
		}
	}
	while(i <= next && i < packetsize);

	for(i = 0; i < 4; i++) 
	{
		m_subpal[i].pal = (pal >> (i << 2)) & 0xf;
		m_subpal[i].tr = (tr >> (i << 2)) & 0xf;
	}
}

BYTE CVobSubImage::GetNibble(BYTE* buff, DWORD* offset)
{
	BYTE ret = (buff[offset[m_field]] >> (m_aligned << 2)) & 0x0f;
	offset[m_field] += 1 - m_aligned;
	m_aligned = !m_aligned;
	return ret;
}

void CVobSubImage::DrawPixels(Vector2i pt, int len, int color)
{
	Vector4i r = m_rect;

    if(pt.y < r.top || pt.y >= r.bottom) return;
	if(pt.x < r.left) {len -= r.left - pt.x; pt.x = r.left;}
	if(pt.x + len > r.right) len = r.right - pt.x;
	if(len <= 0 || pt.x >= r.right) return;

	RGBQUAD c;

	if(!m_custompal) 
	{
		c = m_palette.org[m_subpal[color].pal];

		c.rgbReserved = (m_subpal[color].tr << 4) | m_subpal[color].tr;
	}
	else
	{
		c = m_palette.cus[color];
	}

	c.rgbBlue = (c.rgbBlue * c.rgbReserved) >> 8;
	c.rgbGreen = (c.rgbGreen * c.rgbReserved) >> 8;
	c.rgbRed = (c.rgbRed * c.rgbReserved) >> 8;

	RGBQUAD* dst = &m_pixels[r.width() * (pt.y - r.top) + (pt.x - r.left)];

	while(len-- > 0) 
	{
		*dst++ = c;
	}
}

void CVobSubImage::TrimSubImage()
{
	Vector4i s = m_rect.rsize();
	Vector4i r = s.zwxy();
	
	RGBQUAD* ptr = m_buff[0];

	for(int j = 0; j < s.w; j++)
	{
		for(int i = 0; i < s.z; i++, ptr++)
		{
			if(ptr->rgbReserved)
			{
				if(r.top > j) r.top = j;
				if(r.bottom < j) r.bottom = j;
				if(r.left > i) r.left = i; 
				if(r.right < i) r.right = i; 
			}
		}
	}

	if(r.left > r.right || r.top > r.bottom)
	{
		return;
	}

	r = r.inflate(Vector4i(0, 0, 1, 1)).rintersect(s);

	int w = r.width();
	int h = r.height();

	DWORD offset = r.top * s.z + r.left;

	r = r.inflate(1);

	DWORD* src = (DWORD*)&m_buff[0][offset];
	DWORD* dst = (DWORD*)&m_buff[1][1 + w + 1];

	memset(m_buff[1], 0, (1 + w + 1) * sizeof(RGBQUAD));

	for(int i = h; i > 0; i--, src += s.z)
	{
		*dst++ = 0;
		memcpy(dst, src, w * sizeof(RGBQUAD)); dst += w; 
		*dst++ = 0;
	}

	memset(dst, 0, (1 + w + 1) * sizeof(RGBQUAD));

	m_pixels = m_buff[1];

	m_rect = r + m_rect.xyxy();
}

// CVobSub

CVobSubProvider::CVobSubProvider()
{
	m_size = Vector2i(720, 480);
	m_pos = Vector2i(0, 0);
	m_org = Vector2i(0, 0);
	m_scale = Vector2i(0, 0);
	m_alpha = 100;
	m_smooth = 0;
	m_fade.in = m_fade.out = 50;
	m_align.enabled = false;
	m_align.hor = m_align.ver = 0;
	m_offset = 0;
	m_forcedonly = false;
	m_custompal = false;
	m_tridx = 0;
	memset(&m_palette, 0, sizeof(m_palette));
	m_ss = &m_sss[0];
}

CVobSubProvider::~CVobSubProvider()
{
	RemoveAll();
}

STDMETHODIMP CVobSubProvider::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    *ppv = NULL;

    return 
		QI(ISubStream)
		QI(IVobSubStream)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

bool CVobSubProvider::Open(const std::list<std::wstring>& rows, FILE* sub)
{
	std::wstring::size_type ver;

	if(rows.empty() || (ver = rows.front().find(VOBSUBIDXSTR)) == std::wstring::npos)
	{
		return false;
	}

	if(_wtoi(rows.front().c_str() + ver + wcslen(VOBSUBIDXSTR)) > VOBSUBIDXVER)
	{
		return false;
	}

	int langidx = -1;
	SubStream* ss = NULL;
	
	std::wregex rx(L"([+-])?([0-9]+):([0-9]+):([0-9]+):([0-9]+).+filepos.+?([0-9a-zA-Z]+)");
	
	BYTE* buff = new BYTE[0x800];

	for(auto i = rows.begin(); i != rows.end(); i++)
	{
		std::list<std::wstring> cols;

		Util::Explode(*i, cols, L":", 2);

		if(cols.size() != 2) continue;
		
		std::wstring key = cols.front();
		std::wstring value = cols.back();

		if(key == L"size") 
		{
			swscanf(value.c_str(), L"%dx %d", &m_size.x, &m_size.y);
		}
		else if(key == L"org") 
		{
			swscanf(value.c_str(), L"%d, %d", &m_org.x, &m_org.y);
		}
		else if(key == L"scale") 
		{
			swscanf(value.c_str(), L"%d%%, %d%%", &m_scale.x, &m_scale.y);
		}
		else if(key == L"alpha") 
		{
			swscanf(value.c_str(), L"%d%%", &m_alpha);
		}
		else if(key == L"smooth")
		{
			m_smooth = 
				value == L"0" || value == L"OFF" ? 0 : 
				value == L"1" || value == L"ON" ? 1 : 
				value == L"2" || value == L"OLD" ? 2 : 
				0;
		}
		else if(key == L"align")
		{
			Util::Explode(value, cols, L" ", 2);

			if(cols.size() == 4) 
			{
				cols.erase(++cols.begin());
			}

			if(cols.size() == 3)
			{
				m_align.enabled = cols.front() == L"ON"; cols.pop_front();
				
				std::wstring hor = cols.front();
				std::wstring ver = cols.back();

				m_align.hor = hor == L"LEFT" ? 0 : hor == L"CENTER" ? 1 : hor == L"RIGHT" ? 2 : 1;
				m_align.ver = ver == L"TOP" ? 0 : ver == L"CENTER" ? 1 : ver == L"BOTTOM" ? 2 : 2;
			}
		}
		else if(key == L"fade in/out")
		{
			swscanf(value.c_str(), L"%d%%, %d%%", &m_fade.in, &m_fade.out);
		}
		else if(key == L"time offset")
		{
			m_offset = _wtoi(value.c_str());
		}
		else if(key == L"forced subs")
		{
			m_forcedonly = value == L"1" || value == L"ON";
		}
		else if(key == L"palette")
		{
			Util::Explode(value, cols, L",", 16);

			for(int i = 0; i < 16 && !cols.empty(); i++, cols.pop_front())
			{
				*(DWORD*)&m_palette.org[i] = wcstoul(cols.front().c_str(), NULL, 16);
			}
		}
		else if(key == L"custom colors")
		{
			m_custompal = Util::Explode(value, cols, L",", 3) == L"ON";

			if(cols.size() == 3)
			{
				cols.pop_front();

				std::list<std::wstring> sl;

				Util::Explode(cols.front(), sl, L":", 2);

				cols.pop_front();
				
				if(sl.front() == L"tridx")
				{
					sl.pop_front();

					wchar_t tr[4];
					
					swscanf(sl.front().c_str(), L"%c%c%c%c", &tr[0], &tr[1], &tr[2], &tr[3]);
					
					for(int i = 0; i < 4; i++)
					{
						m_tridx |= ((tr[i] == '1') ? 1 : 0) << i;
					}
				}

				Util::Explode(cols.front(), sl, L":", 2);
				
				if(sl.front() == L"colors")
				{
					sl.pop_front();

					value = sl.front();

					Util::Explode(value, sl, L",", 4);

					for(int i = 0; i < 4 && !sl.empty(); i++)
					{
						*(DWORD*)&m_palette.cus[i] = wcstoul(sl.front().c_str(), NULL, 16);
					}
				}
			}
		}
		else if(key == L"langidx")
		{
			langidx = _wtoi(value.c_str());
		}
		else if(key == L"id")
		{
			int id = (value[0] << 8) | value[1];
			int index = -1;

			std::wstring::size_type i = value.find(L"index:");

			if(i != std::wstring::npos)
			{
				index = _wtoi(value.substr(i + 6).c_str());
			}

			if(index >= 0 && index < sizeof(m_sss) / sizeof(m_sss[0]))
			{
				ss = &m_sss[index];

				ss->id = id;
				ss->name = Util::ISO6391ToLanguage(value.substr(0, 2).c_str());
			}
		}
		else if(key == L"alt")
		{
			if(ss != NULL && !value.empty())
			{
				ss->name = value;
			}
		}
		else if(key == L"timestamp")
		{
			if(ss != NULL)
			{
				std::wcmatch res;

				if(std::regex_match(value.c_str(), res, rx))
				{
					SubPic* sp = new SubPic();

					int h = _wtoi(res[2].first);
					int m = _wtoi(res[3].first);
					int s = _wtoi(res[4].first);
					int ms = _wtoi(res[5].first);

					sp->start = ((((REFERENCE_TIME)h * 60 + m) * 60 + s) * 1000 + ms) * 10000; 

					if(res[1].matched && res[1].first[0] == '-')
					{
						sp->start = -sp->start;
					}

					sp->stop = sp->start;

					sp->forced = false;

					__int64 filepos = _wcstoi64(res[6].first, NULL, 16);

					if(sub != NULL)
					{
						_fseeki64(sub, filepos, SEEK_SET);

						fread(buff, 1, 0x800, sub);

						BYTE o = buff[0x16];

						if(*(DWORD*)&buff[0x00] == 0xba010000
						|| *(DWORD*)&buff[0x0e] == 0xbd010000
						|| (buff[0x15] & 0x80) != 0
						|| (buff[0x17] & 0xf0) == 0x20
						|| (buff[o + 0x17] & 0xe0) == 0x20)
						{
							BYTE id = buff[o + 0x17] & 0x1f;

					        DWORD packetsize = (buff[o + 0x18] << 8) + buff[o + 0x19];
							DWORD datasize = (buff[o + 0x1a] << 8) + buff[o + 0x1b];

							sp->buff.resize(packetsize);

							memset(sp->buff.data(), 0, sp->buff.size());

							int i = 0;
							int remaining = packetsize;

							while(i < packetsize)
							{
								int hsize = 0x18 + buff[0x16];
								int size = std::min<int>(0x800 - hsize, remaining);

								memcpy(&sp->buff[i], &buff[hsize], size);

								if(size != remaining)
								{
									while(!feof(sub))
									{
										fread(buff, 1, 0x800, sub);

										if(buff[buff[0x16] + 0x17] == (0x20 | id))
										{
											break;
										}
									}
								}

								i += size;
								remaining -= size;
							}

							if(i != packetsize || remaining > 0)
							{
								sp->buff.clear();
							}

							m_img.GetPacketInfo(sp->buff.data(), packetsize, datasize);

							sp->stop = sp->start + m_img.m_delay;

							sp->forced = m_img.m_forced;
						}
					}

					ss->sps.push_back(sp);
				}
			}
		}
		// TODO: delay
	}

	delete [] buff;

	if(langidx >= 0 && langidx < sizeof(m_sss) / sizeof(m_sss[0]) && !m_sss[langidx].sps.empty())
	{
		m_ss = &m_sss[langidx];
	}

	for(int i = 0; i < 16; i++)
	{
		BYTE c = m_palette.org[i].rgbBlue;
		m_palette.org[i].rgbBlue = m_palette.org[i].rgbRed;
		m_palette.org[i].rgbRed = c;
	}

	for(int i = 0; i < 4; i++)
	{
		BYTE c = m_palette.cus[i].rgbBlue;
		m_palette.cus[i].rgbBlue = m_palette.cus[i].rgbRed;
		m_palette.cus[i].rgbRed = c;
	}

	return true;
}

bool CVobSubProvider::GetCustomPal(RGBQUAD* pal, int& tridx)
{
	memcpy(pal, m_palette.cus, sizeof(m_palette.cus)); 

	tridx = m_tridx;
	
	return m_custompal;
}

void CVobSubProvider::SetCustomPal(RGBQUAD* pal, int tridx) 
{
	memcpy(m_palette.cus, pal, sizeof(m_palette.cus)); 

	m_tridx = tridx & 0xf;

	for(int i = 0; i < 4; i++) 
	{
		m_palette.cus[i].rgbReserved = (tridx & (1 << i)) ? 0 : 0xff;
	}

	// TOOD: m_img.Invalidate();
}

void CVobSubProvider::GetDestrect(Vector4i& r)
{
	int w = MulDiv(m_img.m_rect.width(), m_scale.x, 100);
	int h = MulDiv(m_img.m_rect.height(), m_scale.y, 100);

	if(!m_align.enabled)
	{
		r.left = MulDiv(m_img.m_rect.left, m_scale.x, 100);
		r.right = MulDiv(m_img.m_rect.right, m_scale.x, 100);
		r.top = MulDiv(m_img.m_rect.top, m_scale.y, 100);
		r.bottom = MulDiv(m_img.m_rect.bottom, m_scale.y, 100);
	}
	else
	{
		switch(m_align.hor)
		{
		case 0: r.left = 0; r.right = w; break; // left
		case 1: r.left = -(w >> 1); r.right = -(w >> 1) + w; break; // center
		case 2: r.left = -w; r.right = 0; break; // right
		default:
			r.left = MulDiv(m_img.m_rect.left, m_scale.x, 100);
			r.right = MulDiv(m_img.m_rect.right, m_scale.x, 100);
			break;
		}
		
		switch(m_align.ver)
		{
		case 0: r.top = 0; r.bottom = h; break; // top
		case 1: r.top = -(h >> 1); r.bottom = -(h >> 1) + h; break; // center
		case 2: r.top = -h; r.bottom = 0; break; // bottom
		default:
			r.top = MulDiv(m_img.m_rect.top, m_scale.y, 100);
			r.bottom = MulDiv(m_img.m_rect.bottom, m_scale.y, 100);
			break;
		}
	}

	r += Vector4i(m_org).xyxy();
}

void CVobSubProvider::GetDestrect(const Vector2i& s, Vector4i& r)
{
	GetDestrect(r);

	r.left = MulDiv(r.left, s.x, m_size.x);
	r.top = MulDiv(r.top, s.y, m_size.y);
	r.right = MulDiv(r.right, s.x, m_size.x);
	r.bottom = MulDiv(r.bottom, s.y, m_size.y);
}

void CVobSubProvider::SetAlignment(bool align, int x, int y, int hor, int ver)
{
	m_align.enabled = align;

	if(align)
	{
		m_org.x = MulDiv(m_size.x, x, 100);
		m_org.y = MulDiv(m_size.y, y, 100);
		m_align.hor = std::min<int>(std::max<int>(hor, 0), 2);
		m_align.ver = std::min<int>(std::max<int>(ver, 0), 2);
	}
	else
	{
		m_org = m_pos;
	}
}

// ISubPicProvider
	
STDMETHODIMP_(void*) CVobSubProvider::GetStartPosition(REFERENCE_TIME rt, float fps)
{
	CAutoLock cAutoLock(this);

	for(int i = 0; i < m_ss->sps.size(); i++) // TODO: log2 search
	{
		SubPic* sp = m_ss->sps[i];

		if(sp->start <= rt && rt < sp->stop || rt < sp->start)
		{
			return (void*)(i + 1);
		}
	}

	return NULL;
}

STDMETHODIMP_(void*) CVobSubProvider::GetNext(void* pos)
{
	CAutoLock cAutoLock(this);

	int i = (int)pos - 1;

	return i >= 0 && i < m_ss->sps.size() ? (void*)(i + 2) : NULL;
}

STDMETHODIMP CVobSubProvider::GetTime(void* pos, float fps, REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	CAutoLock cAutoLock(this);

	int i = (int)pos - 1;

	if(i >= 0 && i < m_ss->sps.size())
	{
		SubPic* sp = m_ss->sps[i];

		start = sp->start;
		stop = sp->stop;

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP_(bool) CVobSubProvider::IsAnimated(void* pos)
{
	return false;
}

STDMETHODIMP CVobSubProvider::Render(SubPicDesc& desc, float fps)
{
	REFERENCE_TIME rt = (desc.start + desc.stop) / 2;

	for(int i = m_ss->sps.size() - 1; i >= 0; i--) // TODO: log2 search
	{
		SubPic* sp = m_ss->sps[i];

		if(sp->start <= rt && rt < sp->stop)
		{
			UINT_PTR cookie = (UINT_PTR)sp;

			if(m_img.m_cookie != cookie)
			{
				m_img.m_cookie = cookie;

				BYTE* p = sp->buff.data();
				
				m_img.Decode(p, (p[0] << 8) | p[1], (p[2] << 8) | p[3], m_custompal, m_tridx, m_palette.org, m_palette.cus, true);
			}

			Vector4i r;

			GetDestrect(desc.size, r);
			
			if(!r.rempty())
			{
				int srcpitch =  m_img.m_rect.width() * 4;

				BYTE* src = (BYTE*)m_img.m_pixels;
				BYTE* dst = desc.bits + desc.pitch * r.top + r.left * 4;

				DWORD w = r.width();
				DWORD h = r.height();

				DWORD w2 = m_img.m_rect.width();
				DWORD h2 = m_img.m_rect.height();

				DWORD xd = (w2 << 15) / w;
				DWORD yd = (h2 << 15) / h;

				for(DWORD j = 0, y = 0; j < h; j++, y += yd, dst += desc.pitch)
				{
					Vector4i yf = Vector4i::load(y & 0x7fff).xxxxl();

					DWORD y0 = y >> 15;
					DWORD y1 = std::min<DWORD>(y0 + 1, h2 - 1);

					DWORD* RESTRICT s0 = (DWORD*)(src + srcpitch * y0);
					DWORD* RESTRICT s1 = (DWORD*)(src + srcpitch * y1);
					DWORD* RESTRICT d = (DWORD*)dst;

					for(DWORD i = 0, x = 0; i < w; i++, x += xd)
					{
						Vector4i xf = Vector4i::load(x & 0x7fff).xxxxl();

						DWORD x0 = x >> 15;
						DWORD x1 = std::min<DWORD>(x0 + 1, w2 - 1);

						Vector4i v0 = Vector4i::load(s0[x0]).upl8();
						Vector4i v1 = Vector4i::load(s0[x1]).upl8();
						Vector4i v2 = Vector4i::load(s1[x0]).upl8();
						Vector4i v3 = Vector4i::load(s1[x1]).upl8();

						v0 = v0.lerp16<0>(v1, xf);
						v2 = v2.lerp16<0>(v3, xf);
						v0 = v0.lerp16<0>(v2, yf);

						d[i] = (DWORD)Vector4i::store(v0.pu16());
					}
				}
			}

			r = r.rintersect(Vector4i(0, 0, desc.size.x, desc.size.y));
			
			desc.bbox = r;
			
			return !r.rempty() ? S_OK : S_FALSE;
		}
	}

	return E_FAIL;
}

// ISubStream

STDMETHODIMP_(int) CVobSubProvider::GetStreamCount()
{
	CAutoLock cAutoLock(this);

	int count = 0;

	for(int i = 0; i < sizeof(m_sss) / sizeof(m_sss[0]); i++)
	{
		if(m_sss[i].id == 0) break;

		count++;
	}

	return count;
}

STDMETHODIMP CVobSubProvider::GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID)
{
	CAutoLock cAutoLock(this);

	if(ppName)
	{
		*ppName = (WCHAR*)CoTaskMemAlloc((m_ss->name.size() + 1) * sizeof(WCHAR));

		if(*ppName == NULL) return E_OUTOFMEMORY;
		
		wcscpy(*ppName, m_ss->name.c_str());
	}

	if(pLCID)
	{
		*pLCID = 0; // TODO
	}

	return S_OK;
}

STDMETHODIMP_(int) CVobSubProvider::GetStream()
{
	CAutoLock cAutoLock(this);

	return m_ss - &m_sss[0];
}

STDMETHODIMP CVobSubProvider::SetStream(int i)
{
	CAutoLock cAutoLock(this);

	if(m_sss[i].id != 0)
	{
		m_ss = &m_sss[i];

		m_img.Invalidate();

		return S_OK;
	}

	return E_INVALIDARG;
}

// IVobSubStream

STDMETHODIMP CVobSubProvider::OpenFile(LPCWSTR fn)
{
	CAutoLock cAutoLock(this);

	std::list<std::wstring> rows;

	FILE* fp = _wfopen(fn, L"r");

	if(fp == NULL)
	{
		return E_INVALIDARG;
	}

	bool valid = false;

	char buff[256];

	while(!feof(fp))
	{
		buff[0] = 0;

		if(fgets(buff, sizeof(buff) / sizeof(buff[0]), fp) == NULL)
		{
			break;
		}

		std::string s = buff;
		std::wstring ws = Util::Trim(std::wstring(s.begin(), s.end()));

		if(!ws.empty())
		{
			if(!valid)
			{
				if(wcsstr(ws.c_str(), VOBSUBIDXSTR) == NULL)
				{
					break;
				}

				valid = true;
			}
			else
			{
				if(buff[0] == '#')
				{
					continue;
				}
			}

			rows.push_back(ws);
		}
	}

	fclose(fp);

	HRESULT hr;

	fp = _wfopen((Util::RemoveFileExt(fn) + L".sub").c_str(), L"rb");

	if(fp != NULL)
	{
		hr = Open(rows, fp);

		fclose(fp);

		return hr;
	}

	// TODO: try (Util::RemoveFileExt(fn) + L".rar").c_str()

	return E_FAIL;
}

STDMETHODIMP CVobSubProvider::OpenIndex(LPCWSTR str, LPCWSTR name)
{
	CAutoLock cAutoLock(this);

	m_ss->id = 'xx';
	m_ss->name = name;

	std::list<std::wstring> rows;
	
	Util::Explode(std::wstring(str), rows, L"\n");

	rows.push_front(Util::Format(L"# %s%d\n", VOBSUBIDXSTR, VOBSUBIDXVER)); // just in case

	return Open(rows, NULL) ? S_OK : E_FAIL;
}

STDMETHODIMP CVobSubProvider::Append(REFERENCE_TIME start, REFERENCE_TIME stop, BYTE* data, int len)
{
	if(len <= 4 || ((data[0] << 8) | data[1]) != len)
	{
		return E_INVALIDARG;
	}

	CVobSubImage vsi;

	vsi.GetPacketInfo(data, (data[0] << 8) | data[1], (data[2] << 8) | data[3]);

	SubPic* p = new SubPic();

	p->start = start;
	p->stop = vsi.m_delay > 0 ? (start + vsi.m_delay) : stop;
	p->forced = vsi.m_forced;
	p->buff.resize(len);
	
	memcpy(p->buff.data(), data, len);

	CAutoLock cAutoLock(this);

	int i = m_ss->sps.size() - 1;

	while(i >= 0)
	{
		if(m_ss->sps[i]->start < start)
		{
			break;
		}

		i--;
	}

	if(++i < m_ss->sps.size())
	{
		for(int j = i; j < m_ss->sps.size(); j++)
		{
			delete m_ss->sps[j];
		}

		m_ss->sps.resize(i);
	}

	m_ss->sps.push_back(p);

	return S_OK;
}

STDMETHODIMP CVobSubProvider::RemoveAll()
{
	CAutoLock cAutoLock(this);

	for(auto i = m_ss->sps.begin(); i != m_ss->sps.end(); i++)
	{
		delete *i;
	}

	m_ss->sps.clear();

	return S_OK;
}
