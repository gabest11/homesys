/* 
 *	Copyright (C) 2007-2009 Gabest
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
#include "Device.h"

namespace DXUI
{
	#include "convert.fx"

	Device::Device() 
		: m_wnd(NULL)
		, m_rbswapped(false)
		, m_backbuffer(NULL)
	{
		memset(&m_vertices, 0, sizeof(m_vertices));

		m_msaa = 0; // TODO

		m_msaa_desc.Count = 1;
		m_msaa_desc.Quality = 0;
	}

	Device::~Device() 
	{
		for(auto i = m_pool.begin(); i != m_pool.end(); i++) delete *i;

		delete m_backbuffer;
	}

	bool Device::Create(HWND wnd, bool vsync, bool perfhud)
	{
		m_wnd = wnd;
		m_vsync = vsync;
		m_perfhud = perfhud;

		return true;
	}

	bool Device::Reset(int mode)
	{
		for(auto i = m_pool.begin(); i != m_pool.end(); i++) delete *i;

		m_pool.clear();
		
		delete m_backbuffer;

		m_backbuffer = NULL;

		return true;
	}

	Texture* Device::Fetch(int type, int w, int h, bool msaa, int format)
	{
		if(m_msaa < 2)
		{
			msaa = false;
		}

		Vector2i size(w, h);

		for(std::list<Texture*>::iterator i = m_pool.begin(); i != m_pool.end(); i++)
		{
			Texture* t = *i;

			if(t->GetType() == type && t->GetFormat() == format && t->GetSize() == size && t->IsMSAA() == msaa)
			{
				m_pool.erase(i);

				t->ClearFlag(-1);

				return t;
			}
		}

		return Create(type, w, h, msaa, format);
	}

	void Device::EndScene()
	{
		m_vertices.start += m_vertices.count;
		m_vertices.count = 0;
	}

	void Device::Recycle(Texture* t)
	{
		if(t)
		{
			m_pool.push_front(t);

			while(m_pool.size() > 600)
			{
				delete m_pool.back();

				m_pool.pop_back();
			}
		}
	}

	Texture* Device::CreateRenderTarget(int w, int h, bool msaa, int format)
	{
		return Fetch(Texture::RenderTarget, w, h, msaa, format);
	}

	Texture* Device::CreateDepthStencil(int w, int h, bool msaa, int format)
	{
		return Fetch(Texture::DepthStencil, w, h, msaa, format);
	}

	Texture* Device::CreateTexture(int w, int h, int format)
	{
		return Fetch(Texture::Video, w, h, false, format);
	}

	Texture* Device::CreateOffscreen(int w, int h, int format)
	{
		return Fetch(Texture::System, w, h, false, format);
	}

	void Device::StretchRect(Texture* st, Texture* dt, const Vector4& dr, int shader, bool linear)
	{
		StretchRect(st, Vector4(0, 0, 1, 1), dt, dr, shader, linear);
	}

	bool Device::ResizeTexture(Texture** t, int w, int h)
	{
		if(t == NULL) {ASSERT(0); return false;}

		Texture* t2 = *t;

		if(t2 == NULL || t2->GetWidth() != w || t2->GetHeight() != h)
		{
			delete t2;

			t2 = CreateTexture(w, h);

			*t = t2;
		}

		return t2 != NULL;
	}

	bool Device::SetFeatureLevel(D3D_FEATURE_LEVEL level, bool compat_mode)
	{
		m_shader.level = level;

		switch(level)
		{
		case D3D_FEATURE_LEVEL_9_1:
		case D3D_FEATURE_LEVEL_9_2:
			m_shader.model = "0x200";
			m_shader.vs = compat_mode ? "vs_4_0_level_9_1" : "vs_2_0";
			m_shader.ps = compat_mode ? "ps_4_0_level_9_1" : "ps_2_0";
			break;
		case D3D_FEATURE_LEVEL_9_3:
			m_shader.model = "0x300";
			m_shader.vs = compat_mode ? "vs_4_0_level_9_3" : "vs_3_0";
			m_shader.ps = compat_mode ? "ps_4_0_level_9_3" : "ps_3_0";
			break;
		case D3D_FEATURE_LEVEL_10_0:
			m_shader.model = "0x400";
			m_shader.vs = "vs_4_0";
			m_shader.gs = "gs_4_0";
			m_shader.ps = "ps_4_0";
			break;
		case D3D_FEATURE_LEVEL_10_1:
			m_shader.model = "0x401";
			m_shader.vs = "vs_4_1";
			m_shader.gs = "gs_4_1";
			m_shader.ps = "ps_4_1";
			break;
		case D3D_FEATURE_LEVEL_11_0:
			m_shader.model = "0x500";
			m_shader.vs = "vs_5_0";
			m_shader.gs = "gs_5_0";
			m_shader.ps = "ps_5_0";
			break;
		default:
			ASSERT(0);
			return false;
		}

		return true;
	}
}