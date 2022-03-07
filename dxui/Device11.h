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
#include "Texture11.h"
#include "PixelShader11.h"

#define D3D11_SHADER_MACRO D3D10_SHADER_MACRO
#define ID3D11Blob ID3D10Blob

namespace DXUI
{
	class Device11 : public Device
	{
		Texture* Create(int type, int w, int h, bool msaa, int format);

		//

		CComPtr<ID3D11Device> m_dev;
		CComPtr<ID3D11DeviceContext> m_ctx;
		CComPtr<IDXGISwapChain> m_swapchain;
		CComPtr<ID3D11Buffer> m_vb;
		CComPtr<ID3D11Buffer> m_vb_old;
		CComPtr<ID3D11Buffer> m_ps_cb;

		struct
		{
			ID3D11Buffer* vb;
			size_t vb_stride;
			ID3D11InputLayout* layout;
			D3D11_PRIMITIVE_TOPOLOGY topology;
			ID3D11VertexShader* vs;
			ID3D11Buffer* vs_cb;
			ID3D11GeometryShader* gs;
			ID3D11ShaderResourceView* ps_srv[2];
			ID3D11PixelShader* ps;
			ID3D11Buffer* ps_cb;
			ID3D11SamplerState* ps_ss[2];
			Vector2i viewport;
			Vector4i scissor;
			ID3D11DepthStencilState* dss;
			BYTE sref;
			ID3D11BlendState* bs;
			float bf;
			ID3D11RenderTargetView* rtv;
			ID3D11DepthStencilView* dsv;
		} m_state;

	public: // TODO
		CComPtr<ID3D11RasterizerState> m_rs;

		struct
		{
			CComPtr<ID3D11InputLayout> il;
			CComPtr<ID3D11VertexShader> vs;
			CComPtr<ID3D11Buffer> vs_cb;
			CComPtr<ID3D11PixelShader> ps[4];
			CComPtr<ID3D11SamplerState> ln;
			CComPtr<ID3D11SamplerState> pt;
			CComPtr<ID3D11DepthStencilState> dss;
			CComPtr<ID3D11BlendState> bs[3];
		} m_convert;

	public:
		Device11();
		virtual ~Device11();

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
		void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, bool linear = true);
		void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, ID3D11BlendState* bs, bool linear = true);

		void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
		void IASetVertexBuffer(ID3D11Buffer* vb, size_t stride);
		void IASetInputLayout(ID3D11InputLayout* layout);
		void IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);
		void VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb);
		void GSSetShader(ID3D11GeometryShader* gs);
		void PSSetShaderResources(Texture* sr0, Texture* sr1);
		void PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb);
		void PSSetSamplerState(ID3D11SamplerState* ss0, ID3D11SamplerState* ss1);
		void OMSetDepthStencilState(ID3D11DepthStencilState* dss, BYTE sref);
		void OMSetBlendState(ID3D11BlendState* bs, float bf);
		void OMSetRenderTargets(Texture* rt, Texture* ds, const Vector4i* scissor = NULL);

		ID3D11Device* operator->() {return m_dev;}
		operator ID3D11Device*() {return m_dev;}
		operator ID3D11DeviceContext*() {return m_ctx;}

		HRESULT CompileShader(LPCSTR source, LPCSTR entry, D3D11_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il);
		HRESULT CompileShader(LPCSTR source, LPCSTR entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs);
		HRESULT CompileShader(LPCSTR source, LPCSTR entry, D3D11_SHADER_MACRO* macro, ID3D11PixelShader** ps);
	};
}