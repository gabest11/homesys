#pragma once 

#include "stdafx.h"
#include "DeviceContext.h"
#include "Common.h"
#include "../subtitle/SubtitleFile.h"
#include "../subtitle/Renderer.h"
#include "../util/String.h"
#include <algorithm>

namespace DXUI
{
	DeviceContext::DeviceContext(Device* dev, ResourceLoader* loader)
		: m_dev(dev)
		, m_loader(loader)
		, m_rt(NULL)
		, m_ps(NULL)
		, m_scissor(-1)
	{
		m_ssf = new SSF::Renderer();

		TextureColor0 = 0xffffff;
		TextureColor1 = 0;

		TextAlign = Align::Left | Align::Middle;
		TextColor = Color::White;
		TextFace = L"Arial";
		TextHeight = 30;
	}

	DeviceContext::~DeviceContext()
	{
		IncAge(0);

		delete m_ssf;
	}

	void DeviceContext::IncAge(int limit)
	{
		for(auto i = m_texture.begin(); i != m_texture.end(); )
		{
			auto j = i++;

			TextureEntry* te = j->second;

			if(te->n++ >= limit)
			{
				// wprintf(L"DT %s\n", j->first.c_str());

				delete te;

				m_texture.erase(j);
			}
		}

		for(auto i = m_text.begin(); i != m_text.end(); )
		{
			auto j = i++;

			TextEntry* te = j->second;

			if(te->n++ >= limit)
			{
				delete te;

				m_text.erase(j);
			}
		}

		for(auto i = m_pixelshader.begin(); i != m_pixelshader.end(); )
		{
			auto j = i++;

			PixelShaderEntry* e = j->second;

			if(e->n++ >= limit)
			{
				delete e;

				m_pixelshader.erase(j);
			}
		}
	}

	void DeviceContext::Flip()
	{
		m_dev->Flip(false);

		IncAge(m_dev->IsLost() ? 0 : 3);
	}

	Texture* DeviceContext::GetTexture(LPCWSTR id, bool priority, bool refresh)
	{
		auto i = m_texture.find(id);

		if(i != m_texture.end())
		{
			if(refresh)
			{
				delete i->second;

				m_texture.erase(i);
			}
			else
			{
				i->second->n = 0;

				return i->second->t;
			}
		}

		// if(id != NULL && id[0] != 0) wprintf(L"CT %s\n", id);

		std::vector<BYTE> buff;

		if(m_loader->GetResource(id, buff, priority, refresh))
		{
			Texture* t = m_dev->CreateTexture(buff);

			if(t != NULL)
			{
				TextureEntry* te = new TextureEntry();
				
				te->n = 0;
				te->t = t;

				m_texture[id] = te;

				return t;
			}

			m_loader->IgnoreResource(id);
		}

		return NULL;
	}

	PixelShader* DeviceContext::GetPixelShader(LPCWSTR id)
	{
		auto i = m_pixelshader.find(id);

		if(i != m_pixelshader.end())
		{
			i->second->n = 0;

			return i->second->ps;
		}

		std::vector<BYTE> buff;

		if(m_loader->GetResource(id, buff))
		{
			std::string s((LPCSTR)buff.data(), buff.size());

			PixelShader* ps = m_dev->CreatePixelShader(s.c_str());

			if(ps != NULL)
			{
				PixelShaderEntry* e = new PixelShaderEntry();
				
				e->n = 0;
				e->ps = ps;

				m_pixelshader[id] = e;

				return ps;
			}

			m_loader->IgnoreResource(id);
		}

		return NULL;
	}

	Texture* DeviceContext::CreateTexture(const BYTE* p, int w, int h, int pitch, bool alpha)
	{
		if(p == NULL || w <= 0 || h <= 0 || pitch < w * 4)
		{
			return NULL;
		}

		Texture* t = m_dev->CreateTexture(w, h);

		if(t == NULL)
		{
			return NULL;
		}

		Vector4i r(0, 0, w, h);

		if(!alpha)
		{
			int pitch2 = ((w + 3) & ~3) * sizeof(DWORD);

			BYTE* tmp = (BYTE*)_aligned_malloc(pitch2 * h, 16);

			BYTE* p2 = tmp;

			for(int j = 0; j < h; j++, p += pitch, p2 += pitch2)
			{
				for(int i = 0; i < w; i++)
				{
					((DWORD*)p2)[i] = ((DWORD*)p)[i] | 0xff000000;
				}
			}

			t->Update(r, tmp, pitch);

			_aligned_free(tmp);
		}
		else
		{
			t->Update(r, p, pitch);
		}

		return t;
	}

	PixelShader* DeviceContext::SetPixelShader(PixelShader* ps)
	{
		PixelShader* old = m_ps;

		m_ps = ps;

		return old;
	}

	Texture* DeviceContext::SetTarget(Texture* rt)
	{
		Texture* old = m_rt;

		m_rt = rt;

		return old;
	}

	void DeviceContext::SetScissor(const Vector4i* r)
	{
		m_scissor = r != NULL ? *r : Vector4i(-1);
	}

	Vector4i* DeviceContext::GetScissor()
	{
		return !m_scissor.eq(Vector4i(-1)) ? &m_scissor : NULL;
	}

	int DeviceContext::GetBlendStateIndex(Texture* t)
	{
		int index = 1;

		if(t != NULL)
		{
			index = t->HasFlag(Texture::AlphaComposed) ? 2 : t->HasFlag(Texture::NotFullRange) ? 0 : 1;
		}

		return index;
	}

	int DeviceContext::GetShaderIndex(Texture* t)
	{
		int index = 0;

		if(t != NULL)
		{
			index = t->HasFlag(Texture::NotFullRange) ? 2 : 1;
		}

		return index;
	}

	void DeviceContext::Draw(const Vector4i& r, DWORD c)
	{
		Vector4 d(r);
		Vector4 p(0.5f, 1.0f);
		Vector2 st = Vector2(0, 0);

		Vertex v[6] =
		{
			{d.xyxy(p), st, Vector2i(c, 0)},
			{d.zyxy(p), st, Vector2i(c, 0)},
			{d.xwxy(p), st, Vector2i(c, 0)},
			{d.zwxy(p), st, Vector2i(c, 0)},
		};

		v[4] = v[1];
		v[5] = v[2];

		DrawTriangleList(v, 2, NULL, m_ps);
	}

	void DeviceContext::Draw(Texture* t, const Vector4i& r, float alpha)
	{
		if(t == NULL) {ASSERT(0); return;}

		Draw(t, r, Vector4i(Vector2i(0, 0), t->GetSize()), alpha);
	}

	void DeviceContext::Draw(Texture* t, const Vector4i& r, const Vector4i& src, float alpha)
	{
		if(t == NULL) {ASSERT(0); return;}

		Vector4 d(r);
		Vector4 s(src);
		Vector4 p(0.5f, 1.0f);
		Vector4 st = s / Vector4(t->GetSize()).xyxy();

		DWORD c0 = (TextureColor0 & 0xffffff) | (std::max<DWORD>(std::min<DWORD>((DWORD)(alpha * 0xff), 0xff), 0) << 24);
		DWORD c1 = (TextureColor1 & 0xffffff);

		Vector2i c(c0, c1);

		Vertex v[6] =
		{
			{d.xyxy(p), Vector2(st.x, st.y), c},
			{d.zyxy(p), Vector2(st.z, st.y), c},
			{d.xwxy(p), Vector2(st.x, st.w), c},
			{d.zwxy(p), Vector2(st.z, st.w), c},
		};

		v[4] = v[1];
		v[5] = v[2];

		DrawTriangleList(v, 2, t, m_ps);
	}

	void DeviceContext::Draw9x9(Texture* t, const Vector4i& r, const Vector2i& margin, float alpha)
	{
		Draw9x9(t, r, Vector4i(margin, margin), alpha);
	}

	void DeviceContext::Draw9x9(Texture* t, const Vector4i& r, const Vector4i& margin, float alpha)
	{
		if(t == NULL) {ASSERT(0); return;}

		Vector4i m = margin;

		if(m.left + m.right > r.width())
		{
			m.left = m.right = r.width() / 2;
		}

		if(m.top + m.bottom > r.height())
		{
			m.top = m.bottom = r.height() / 2;
		}

		Vector4 po = Vector4(r);
		Vector4 pi = Vector4(r.deflate(m));

		Vector4i ts = Vector4i(t->GetSize());
		Vector4 to = Vector4(0, 0, 1, 1);
		Vector4 ti = Vector4(ts.zwxy().deflate(m)) / Vector4(ts.xyxy());

		Vertex v[54];

		int i = 0;

		if(m.top > 0)
		{
			if(m.left > 0) 
			{
				v[i + 0].p = Vector4(po.x, po.y);
				v[i + 0].t = Vector2(to.x, to.y);
				v[i + 1].p = Vector4(pi.x, po.y);
				v[i + 1].t = Vector2(ti.x, to.y);
				v[i + 2].p = Vector4(po.x, pi.y);
				v[i + 2].t = Vector2(to.x, ti.y);
				v[i + 3].p = Vector4(pi.x, pi.y);
				v[i + 3].t = Vector2(ti.x, ti.y);
				v[i + 4] = v[i + 1];
				v[i + 5] = v[i + 2];

				i += 6;
			}

			v[i + 0].p = Vector4(pi.x, po.y);
			v[i + 0].t = Vector2(ti.x, to.y);
			v[i + 1].p = Vector4(pi.z, po.y);
			v[i + 1].t = Vector2(ti.z, to.y);
			v[i + 2].p = Vector4(pi.x, pi.y);
			v[i + 2].t = Vector2(ti.x, ti.y);
			v[i + 3].p = Vector4(pi.z, pi.y);
			v[i + 3].t = Vector2(ti.z, ti.y);
			v[i + 4] = v[i + 1];
			v[i + 5] = v[i + 2];

			i += 6;

			if(m.right > 0) 
			{
				v[i + 0].p = Vector4(pi.z, po.y);
				v[i + 0].t = Vector2(ti.z, to.y);
				v[i + 1].p = Vector4(po.z, po.y);
				v[i + 1].t = Vector2(to.z, to.y);
				v[i + 2].p = Vector4(pi.z, pi.y);
				v[i + 2].t = Vector2(ti.z, ti.y);
				v[i + 3].p = Vector4(po.z, pi.y);
				v[i + 3].t = Vector2(to.z, ti.y);
				v[i + 4] = v[i + 1];
				v[i + 5] = v[i + 2];

				i += 6;
			}
		}

		if(m.top + m.bottom < r.height())
		{
			if(m.left > 0) 
			{
				v[i + 0].p = Vector4(po.x, pi.y);
				v[i + 0].t = Vector2(to.x, ti.y);
				v[i + 1].p = Vector4(pi.x, pi.y);
				v[i + 1].t = Vector2(ti.x, ti.y);
				v[i + 2].p = Vector4(po.x, pi.w);
				v[i + 2].t = Vector2(to.x, ti.w);
				v[i + 3].p = Vector4(pi.x, pi.w);
				v[i + 3].t = Vector2(ti.x, ti.w);
				v[i + 4] = v[i + 1];
				v[i + 5] = v[i + 2];

				i += 6;
			}

			v[i + 0].p = Vector4(pi.x, pi.y);
			v[i + 0].t = Vector2(ti.x, ti.y);
			v[i + 1].p = Vector4(pi.z, pi.y);
			v[i + 1].t = Vector2(ti.z, ti.y);
			v[i + 2].p = Vector4(pi.x, pi.w);
			v[i + 2].t = Vector2(ti.x, ti.w);
			v[i + 3].p = Vector4(pi.z, pi.w);
			v[i + 3].t = Vector2(ti.z, ti.w);
			v[i + 4] = v[i + 1];
			v[i + 5] = v[i + 2];

			i += 6;

			if(m.right > 0) 
			{
				v[i + 0].p = Vector4(pi.z, pi.y);
				v[i + 0].t = Vector2(ti.z, ti.y);
				v[i + 1].p = Vector4(po.z, pi.y);
				v[i + 1].t = Vector2(to.z, ti.y);
				v[i + 2].p = Vector4(pi.z, pi.w);
				v[i + 2].t = Vector2(ti.z, ti.w);
				v[i + 3].p = Vector4(po.z, pi.w);
				v[i + 3].t = Vector2(to.z, ti.w);
				v[i + 4] = v[i + 1];
				v[i + 5] = v[i + 2];

				i += 6;
			}
		}

		if(m.bottom > 0)
		{
			if(m.left > 0) 
			{
				v[i + 0].p = Vector4(po.x, pi.w);
				v[i + 0].t = Vector2(to.x, ti.w);
				v[i + 1].p = Vector4(pi.x, pi.w);
				v[i + 1].t = Vector2(ti.x, ti.w);
				v[i + 2].p = Vector4(po.x, po.w);
				v[i + 2].t = Vector2(to.x, to.w);
				v[i + 3].p = Vector4(pi.x, po.w);
				v[i + 3].t = Vector2(ti.x, to.w);
				v[i + 4] = v[i + 1];
				v[i + 5] = v[i + 2];

				i += 6;
			}

			v[i + 0].p = Vector4(pi.x, pi.w);
			v[i + 0].t = Vector2(ti.x, ti.w);
			v[i + 1].p = Vector4(pi.z, pi.w);
			v[i + 1].t = Vector2(ti.z, ti.w);
			v[i + 2].p = Vector4(pi.x, po.w);
			v[i + 2].t = Vector2(ti.x, to.w);
			v[i + 3].p = Vector4(pi.z, po.w);
			v[i + 3].t = Vector2(ti.z, to.w);
			v[i + 4] = v[i + 1];
			v[i + 5] = v[i + 2];

			i += 6;

			if(m.right > 0) 
			{
				v[i + 0].p = Vector4(pi.z, pi.w);
				v[i + 0].t = Vector2(ti.z, ti.w);
				v[i + 1].p = Vector4(po.z, pi.w);
				v[i + 1].t = Vector2(to.z, ti.w);
				v[i + 2].p = Vector4(pi.z, po.w);
				v[i + 2].t = Vector2(ti.z, to.w);
				v[i + 3].p = Vector4(po.z, po.w);
				v[i + 3].t = Vector2(to.z, to.w);
				v[i + 4] = v[i + 1];
				v[i + 5] = v[i + 2];

				i += 6;
			}
		}

		Vector4 p(0.5f, 1.0f);

		DWORD c0 = (TextureColor0 & 0xffffff) | (std::max<DWORD>(std::min<DWORD>((DWORD)(alpha * 0xff), 0xff), 0) << 24);
		DWORD c1 = (TextureColor1 & 0xffffff);

		Vector2i c(c0, c1);

		for(int j = 0; j < i; j++)
		{
			v[j].p = v[j].p.xyxy(p);
			v[j].c = c;
		}

		DrawTriangleList(v, i / 3, t, m_ps);
	}

	void DeviceContext::Draw(const Vector2i& p0, const Vector2i& p1, DWORD c0, DWORD c1)
	{
		Vector4 d(p0.x, p0.y, p1.x, p1.y);
		Vector4 p(0.5f, 1.0f);

		Vertex v[2] =
		{
			{d.xyxy(p), Vector2(0, 0), Vector2i((int)c0, 0)},
			{d.zwxy(p), Vector2(0, 0), Vector2i((int)c1, 0)},
		};

		DrawLineList(v, 1);
	}

	void DeviceContext::Draw(const Vector2i& p0, const Vector2i& p1, DWORD c)
	{
		Draw(p0, p1, c, c);
	}

	TextEntry* DeviceContext::Draw(LPCWSTR str, const Vector4i& rect, bool measure)
	{
		return Draw(str, rect, Vector2i(0, 0), measure);
	}

	TextEntry* DeviceContext::Draw(LPCWSTR str, const Vector4i& rect, Vector2i offset, bool measure)
	{
		std::wstring s = str;

		if((TextStyle & TextStyles::Escaped) == 0)
		{
			if((TextStyle & TextStyles::Wrap) == 0)
			{
				// if(!measure) ???
				Util::Replace(s, L" ", L"\u00a0");
			}
			else
			{
				for(int i = 0, j = s.size(); i < j && s[i] == ' '; i++) s[i] = L'\u00a0';
				for(int i = 0, j = s.size() - 1; i <= j && s[j] == ' '; j--) s[j] = L'\u00a0';
			}
		}

		Vector4i r = rect;

		if(r.rempty() || s.empty())
		{
			return false;
		}

		TextEntry* te = NULL;

		int align = TextAlign;
		DWORD color = TextColor;
		std::wstring face = TextFace;
		int height = TextHeight;
		int style = TextStyle;

		if(measure)
		{
			r.top = 0;
			r.bottom = INT_MAX >> 7;

			color = 0;
		}

		std::wstring id = Util::Format(L"%d,%d:%d:%08x:%s:%d:%d:", r.width(), r.height(), align, color, face.c_str(), height, style) + s;

		auto i = m_text.find(id);

		if(i != m_text.end())
		{
			te = i->second;

			te->n = 0;

			if(measure)
			{
				return te;
			}
			else
			{
				if(offset != te->offset)
				{
					delete te;

					te = NULL;

					m_text.erase(i);
				}
			}
		}

		if(te == NULL)
		{
			te = new TextEntry();

			te->s = s;
			te->align = align;
			te->color = color;
			te->face = face;
			te->height = height;
			te->style = style;
			te->rect = r;
			te->bbox = Vector4i::zero();
			te->offset = offset;

			m_text[id] = te;
		}

		m_ssf->Reset();
		
		std::list<SSF::Subtitle*> subs;

		Vector4i vr = r.rsize();
		Vector2i vs = vr.br;

		Vector4i bbox = Vector4i::zero();

		if(!measure)
		{
			if(te->t == NULL)
			{
				std::wstring ssf = Util::Format(
					L"file#file {format: 'ssf'; version: 1;};\n"
					L"subtitle#subtitle {\n"
					L" frame {reference: 'video'; resolution: {cx: %d; cy: %d;};};\n"
					L" style {\n"
					L"  linebreak: '%s';\n"
					L"  placement {align: {h: %f; v: %f}; margin {t: %d; r: %d; b: %d; l: %d;};};\n"
					L"  font {face: '%s'; size: %d; weight: '%s'; italic: %d; underline: %d; color {r: %d; g: %d; b: %d; a: %d;};};\n"
					L"  background.size: %f;\n"
					L"  shadow.depth: %f;\n"
					L" };\n"
					L"};\n"
					L"subtitle {\n"
					L" time {start: 0; stop: 1;};\n"
					L" @ {%s}\n"
					L"}; ",
					r.width(), r.height(),
					(style & TextStyles::Wrap) ? L"word" : L"none",
					(align & Align::Left) ? 0.0f : (align & Align::Right) ? 1.0f : 0.5f,
					(align & Align::Top) ? 0.0f : (align & Align::Bottom) ? 1.0f : 0.5f,
					offset.y, (align & Align::Center) ? -offset.x : offset.x, 
					-offset.y, (align & Align::Center) ? offset.x : -offset.x, 
					face.c_str(), 
					height, 
					(style & TextStyles::Bold) ? L"bold" : L"normal", 
					(style & TextStyles::Italic) ? 1 : 0, 
					(style & TextStyles::Underline) ? 1 : 0, 
					(color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, 255,
					(style & TextStyles::Outline) ? (float)height / 20 : 0.0f, 
					(style & TextStyles::Shadow) ? (float)height / 5 : 0.0f,
					(style & TextStyles::Escaped) ? s.c_str() : Escape(s.c_str()).c_str()
					);

				try
				{
					SSF::SubtitleFile sf;
					SSF::WCharInputStream stream(ssf.c_str());

					sf.Parse(stream);
					sf.Lookup(0.5f, subs);
				}
				catch(SSF::Exception e)
				{
					wprintf(L"%s\n", (LPCWSTR)e);

					ASSERT(0);
				}

				SSF::Bitmap bm;

				bm.w = (r.width() + 3) & ~3;
				bm.h = r.height();
				bm.pitch = ((bm.w + 3) & ~3) << 2;
				bm.bits = _aligned_malloc(bm.pitch * bm.h, 16);

				if(bm.bits != NULL)
				{
					memset(bm.bits, 0, bm.h * bm.pitch);

					for(auto i = subs.begin(); i != subs.end(); i++)
					{
						SSF::Subtitle* s = *i;
						
						if(SSF::RenderedSubtitle* rs = m_ssf->Lookup(s, vs, vr))
						{
							Vector4i r = rs->Draw(bm);

							bbox = bbox.runion(r);
						}

						delete *i;
					}

					bbox.left &= ~3;
					bbox.right = (bbox.right + 3) & ~3;

					te->bbox = bbox;

					int w = std::max<int>(bbox.width(), 1);
					int h = std::max<int>(bbox.height(), 1);

					Texture* t = m_dev->CreateTexture(w, h);

					if(t != NULL)
					{
						Texture::MemMap m;

						if(t->Map(m))
						{
							bm.Normalize(bbox, m.bits, m.pitch);

							t->Unmap();
						}
						else
						{
							bm.Normalize(bbox);

							t->Update(bbox.rsize(), (BYTE*)bm.bits + bbox.top * bm.pitch + bbox.left * 4, bm.pitch);
						}

						te->t = t;
					}

					_aligned_free(bm.bits);
				}
			}

			if(te->t != NULL)
			{
				Draw(te->t, te->bbox + r.xyxy(), (float)((color >> 24) & 0xff) / 255);
			}
		}
		else
		{
			std::wstring ssf = Util::Format(
				L"file#file {format: 'ssf'; version: 1;};\n"
				L"subtitle#subtitle {\n"
				L" clip: 'false';\n"
				L" frame {reference: 'video'; resolution: {cx: %d; cy: %d;};};\n"
				L" style {\n"
				L"  linebreak: '%s';\n"
				L"  placement {align: {h: %f; v: %f}; margin {t: 0; r: 0; b: 0; l: 0;};};\n"
				L"  font {face: '%s'; size: %d; weight: '%s'; italic: %d; underline: %d; color {b: 0; g: 0; r: 0; a: 0;};};\n"
				L"  background {type: 'box'; size: 0;};\n"
				L"  shadow.depth: 0;\n"
				L" };\n"
				L"};\n"
				L"subtitle {\n"
				L" time {start: 0; stop: 1;};\n"
				L" @ {%s}\n"
				L"};\n",
				r.width(), r.height(),
				(style & TextStyles::Wrap) ? L"word" : L"none",
				(align & Align::Left) ? 0.0f : (align & Align::Right) ? 1.0f : 0.5f,
				(align & Align::Top) ? 0.0f : (align & Align::Bottom) ? 1.0f : 0.5f,
				face.c_str(), 
				height, 
				(style & TextStyles::Bold) ? L"bold" : L"normal", 
				(style & TextStyles::Italic) ? 1 : 0, 
				(style & TextStyles::Underline) ? 1 : 0, 
				(style & TextStyles::Escaped) ? s.c_str() : Escape(s.c_str()).c_str()
				);

			try
			{
				SSF::SubtitleFile sf;
				SSF::WCharInputStream stream(ssf.c_str());

				sf.Parse(stream);
				sf.Lookup(0, subs);
			}
			catch(SSF::Exception e)
			{
				wprintf(L"%s\n", (LPCWSTR)e);

				ASSERT(0);
			}

			if(subs.empty())
			{
				bbox = Vector4i(r.left, r.top, r.left + 1, r.bottom);
			}

			for(auto i = subs.begin(); i != subs.end(); i++)
			{
				SSF::Subtitle* s = *i;
				
				if(SSF::RenderedSubtitle* rs = m_ssf->Lookup(s, vs, vr, false))
				{
					Vector4i r = rs->Measure();

					bbox = bbox.runion(r);
				}

				delete *i;
			}

			te->bbox = bbox;
		}

		return te;
	}

	std::wstring DeviceContext::Escape(LPCWSTR str)
	{
		return SSF::SubtitleFile::Escape(str);
	}

	//

	Vector4i DeviceContext::FitRect(const Vector4i& rect, const Vector2i& size, bool inside)
	{
		int dw = size.x;
		int dh = size.y;

		int sw = rect.width();
		int sh = rect.height();

		Vector4i r = Vector4i::zero();

		if(sh > 0 && sw > 0)
		{
			if((dw * sh > dh * sw) ^ !inside)
			{
				sw = sw * dh / sh;
				sh = dh;

				r.left = (dw - sw) / 2;
				r.right = r.left + sw;
				r.top = 0;
				r.bottom = sh;
			}
			else
			{
				sh = sh * dw / sw;
				sw = dw;

				r.top = (dh - sh) / 2;
				r.bottom = r.top + sh;
				r.left = 0;
				r.right = sw;
			}
		}

		return r;
	}

	Vector4i DeviceContext::FitRect(const Vector4i& rect, const Vector4i& frame, bool inside)
	{
		return frame.xyxy() + FitRect(rect, frame.rsize().br, inside);
	}

	Vector4i DeviceContext::FitRect(const Vector2i& rect, const Vector2i& size, bool inside)
	{
		return FitRect(Vector4i(Vector2i(0, 0), rect), size, inside);
	}

	Vector4i DeviceContext::FitRect(const Vector2i& rect, const Vector4i& frame, bool inside)
	{
		return frame.xyxy() + FitRect(Vector4i(Vector2i(0, 0), rect), frame.rsize().br, inside);
	}

	Vector4i DeviceContext::CenterRect(const Vector4i& rect, const Vector2i& size)
	{
		int w = rect.width();
		int h = rect.height();

		Vector4i r;
		
		r.left = (size.x - w) / 2;
		r.right = r.left + w;
		r.top = (size.y - h) / 2;
		r.bottom = r.top + h;

		return r;
	}

	Vector4i DeviceContext::CenterRect(const Vector4i& rect, const Vector4i& frame)
	{
		return frame.xyxy() + CenterRect(rect, frame.rsize().br);
	}

}