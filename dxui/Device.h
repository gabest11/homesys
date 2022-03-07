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

#pragma once

#include <list>
#include <vector>
#include <d3d11.h>
#include "Texture.h"
#include "PixelShader.h"
#include "../util/AlignedClass.h"

namespace DXUI
{
	#pragma pack(push, 1)

	struct Vertex
	{
		Vector4 p;
		Vector2 t;
		Vector2i c;
	};

	#pragma pack(pop)

	class Device : public AlignedClass<16>
	{
		std::list<Texture*> m_pool;

		Texture* Fetch(int type, int w, int h, bool msaa, int format);

	protected:
		HWND m_wnd;
		bool m_vsync;
		bool m_perfhud;
		bool m_rbswapped;
		Texture* m_backbuffer;
		struct {D3D_FEATURE_LEVEL level; std::string model, vs, gs, ps;} m_shader;
		struct {size_t stride, start, count, limit;} m_vertices;
		DWORD m_msaa;
		DXGI_SAMPLE_DESC m_msaa_desc;

		static LPCSTR m_convert_fx;

		virtual Texture* Create(int type, int w, int h, bool msaa, int format) = 0;

	public:
		Device();
		virtual ~Device();

		void Recycle(Texture* t);
		bool ResizeTexture(Texture** t, int w, int h);
		bool IsLost() {bool canReset; return IsLost(canReset);}
		bool IsRBSwapped() {return m_rbswapped;}
		Texture* GetBackbuffer() const {return m_backbuffer;}

		enum {Windowed, Fullscreen, DontCare};

		virtual bool Create(HWND wnd, bool vsync, bool perfhud);
		virtual bool Reset(int mode);
		virtual void Resize(int w, int h) {}
		virtual bool IsLost(bool& canReset) {return false;}
		virtual void Flip(bool limit) {}

		virtual void BeginScene() {}
		virtual void DrawPrimitive() {}
		virtual void EndScene();

		virtual void ClearRenderTarget(Texture* t, const Vector4& c) {}
		virtual void ClearRenderTarget(Texture* t, DWORD c) {}
		virtual void ClearDepth(Texture* t, float c) {}
		virtual void ClearStencil(Texture* t, BYTE c) {}

		virtual Texture* CreateRenderTarget(int w, int h, bool msaa, int format = 0);
		virtual Texture* CreateDepthStencil(int w, int h, bool msaa, int format = 0);
		virtual Texture* CreateTexture(int w, int h, int format = 0);
		virtual Texture* CreateOffscreen(int w, int h, int format = 0);

		virtual Texture* CreateTexture(const std::vector<BYTE>& buff) = 0;
		virtual PixelShader* CreatePixelShader(LPCSTR shader) = 0;

		virtual Texture* Resolve(Texture* t) {return NULL;}
		virtual Texture* CopyOffscreen(Texture* src, const Vector4& sr, int w, int h, int format = 0) {return NULL;}
		virtual void CopyRect(Texture* st, Texture* dt, const Vector4i& r, int x, int y) {}

		virtual void StretchRect(Texture* st, Texture* dt, const Vector4& dr, int shader = 0, bool linear = true);
		virtual void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, int shader = 0, bool linear = true) {}

		virtual void PSSetShaderResources(Texture* sr0, Texture* sr1) {}
		virtual void OMSetRenderTargets(Texture* rt, Texture* ds, const Vector4i* scissor = NULL) {}
		
		template<class T> void PrepareShaderMacro(std::vector<T>& dst, const T* src)
		{
			dst.clear();

			while(src && src->Definition && src->Name)
			{
				dst.push_back(*src++);
			}

			T m;
			
			m.Name = "SHADER_MODEL";
			m.Definition = m_shader.model.c_str();

			dst.push_back(m);

			m.Name = NULL;
			m.Definition = NULL;

			dst.push_back(m);
		}

		bool SetFeatureLevel(D3D_FEATURE_LEVEL level, bool compat_mode);
	};
}