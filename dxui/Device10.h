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

#include "Device.h"
#include "Texture10.h"
#include "PixelShader10.h"

namespace DXUI
{
	class Device10 : public Device
	{
		Texture* Create(int type, int w, int h, bool msaa, int format);

		//

		CComPtr<ID3D10Device1> m_dev;
		CComPtr<IDXGISwapChain> m_swapchain;
		CComPtr<ID3D10Buffer> m_vb;
		CComPtr<ID3D10Buffer> m_vb_old;
		CComPtr<ID3D10Buffer> m_ps_cb;

		struct
		{
			ID3D10Buffer* vb;
			size_t vb_stride;
			ID3D10InputLayout* layout;
			D3D10_PRIMITIVE_TOPOLOGY topology;
			ID3D10VertexShader* vs;
			ID3D10Buffer* vs_cb;
			ID3D10GeometryShader* gs;
			ID3D10ShaderResourceView* ps_srv[2];
			ID3D10PixelShader* ps;
			ID3D10Buffer* ps_cb;
			ID3D10SamplerState* ps_ss[2];
			Vector2i viewport;
			Vector4i scissor;
			ID3D10DepthStencilState* dss;
			BYTE sref;
			ID3D10BlendState* bs;
			float bf;
			ID3D10RenderTargetView* rtv;
			ID3D10DepthStencilView* dsv;
		} m_state;

	public: // TODO
		CComPtr<ID3D10RasterizerState> m_rs;

		struct
		{
			CComPtr<ID3D10InputLayout> il;
			CComPtr<ID3D10VertexShader> vs;
			CComPtr<ID3D10Buffer> vs_cb;
			CComPtr<ID3D10PixelShader> ps[4];
			CComPtr<ID3D10SamplerState> ln;
			CComPtr<ID3D10SamplerState> pt;
			CComPtr<ID3D10DepthStencilState> dss;
			CComPtr<ID3D10BlendState> bs[3];
		} m_convert;

	public:
		Device10();
		virtual ~Device10();

		bool Create(HWND wnd, bool vsync, bool perfhud);
		bool Reset(int mode);
		void Resize(int w, int h);
		void Flip(bool limit);

		void DrawPrimitive();

		void ClearRenderTarget(Texture* t, const Vector4& c);
		void ClearRenderTarget(Texture* t, DWORD c);
		void ClearDepth(Texture* t, float c);
		void ClearStencil(Texture* t, BYTE c);

		Texture* CreateRenderTarget(int w, int h, bool msaa, int format = 0);
		Texture* CreateDepthStencil(int w, int h, bool msaa, int format = 0);
		Texture* CreateTexture(int w, int h, int format = 0);
		Texture* CreateOffscreen(int w, int h, int format = 0);

		Texture* CreateTexture(const std::vector<BYTE>& buff);
		PixelShader* CreatePixelShader(LPCSTR shader);

		Texture* Resolve(Texture* t);
		Texture* CopyOffscreen(Texture* src, const Vector4& sr, int w, int h, int format = 0);
		void CopyRect(Texture* st, Texture* dt, const Vector4i& r, int x, int y);

		void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, int shader = 0, bool linear = true);
		void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, bool linear = true);
		void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, ID3D10BlendState* bs, bool linear = true);

		void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
		void IASetVertexBuffer(ID3D10Buffer* vb, size_t stride);
		void IASetInputLayout(ID3D10InputLayout* layout);
		void IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY topology);
		void VSSetShader(ID3D10VertexShader* vs, ID3D10Buffer* vs_cb);
		void GSSetShader(ID3D10GeometryShader* gs);
		void PSSetShaderResources(Texture* sr0, Texture* sr1);
		void PSSetShader(ID3D10PixelShader* ps, ID3D10Buffer* ps_cb);
		void PSSetSamplerState(ID3D10SamplerState* ss0, ID3D10SamplerState* ss1);
		void OMSetDepthStencilState(ID3D10DepthStencilState* dss, BYTE sref);
		void OMSetBlendState(ID3D10BlendState* bs, float bf);
		void OMSetRenderTargets(Texture* rt, Texture* ds, const Vector4i* scissor = NULL);

		ID3D10Device* operator->() {return m_dev;}
		operator ID3D10Device*() {return m_dev;}

		HRESULT CompileShader(LPCSTR source, LPCSTR entry, D3D10_SHADER_MACRO* macro, ID3D10VertexShader** vs, D3D10_INPUT_ELEMENT_DESC* layout, int count, ID3D10InputLayout** il);
		HRESULT CompileShader(LPCSTR source, LPCSTR entry, D3D10_SHADER_MACRO* macro, ID3D10GeometryShader** gs);
		HRESULT CompileShader(LPCSTR source, LPCSTR entry, D3D10_SHADER_MACRO* macro, ID3D10PixelShader** ps);
	};
}