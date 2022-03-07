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
#include "Texture9.h"
#include "PixelShader9.h"
//#include <ddraw.h>
#include <d3d9.h>
#include <d3dx9.h>

namespace DXUI
{
	struct Direct3DSamplerState9
	{
	    D3DTEXTUREFILTERTYPE FilterMin[2];
	    D3DTEXTUREFILTERTYPE FilterMag[2];
	    D3DTEXTUREADDRESS AddressU;
	    D3DTEXTUREADDRESS AddressV;
	};

	struct Direct3DDepthStencilState9
	{
	    BOOL DepthEnable;
	    BOOL DepthWriteMask;
	    D3DCMPFUNC DepthFunc;
	    BOOL StencilEnable;
	    UINT8 StencilReadMask;
	    UINT8 StencilWriteMask;
	    D3DSTENCILOP StencilFailOp;
	    D3DSTENCILOP StencilDepthFailOp;
	    D3DSTENCILOP StencilPassOp;
	    D3DCMPFUNC StencilFunc;
		DWORD StencilRef;
	};

	struct Direct3DBlendState9
	{
	    BOOL BlendEnable;
	    D3DBLEND SrcBlend;
	    D3DBLEND DestBlend;
	    D3DBLENDOP BlendOp;
	    D3DBLEND SrcBlendAlpha;
	    D3DBLEND DestBlendAlpha;
	    D3DBLENDOP BlendOpAlpha;
	    UINT8 RenderTargetWriteMask;
	};

	class Device9 : public Device
	{
		Texture* Create(int type, int w, int h, bool msaa, int format);

		//

		D3DCAPS9 m_d3dcaps;
		D3DPRESENT_PARAMETERS m_pp;
		CComPtr<IDirect3D9> m_d3d;
		CComPtr<IDirect3DDevice9> m_dev;
		CComPtr<IDirect3DSwapChain9> m_swapchain;
		CComPtr<IDirect3DVertexBuffer9> m_vb;
		CComPtr<IDirect3DVertexBuffer9> m_vb_old;
		bool m_lost;
		int m_inscene;
		bool m_silent;

		struct
		{
			IDirect3DVertexBuffer9* vb;
			size_t vb_stride;
			IDirect3DVertexDeclaration9* layout;
			D3DPRIMITIVETYPE topology;
			IDirect3DTexture9* vs_srvs[2];
			IDirect3DVertexShader9* vs;
			float* vs_cb;
			int vs_cb_len;
			IDirect3DTexture9* ps_srvs[2];
			IDirect3DPixelShader9* ps;
			float* ps_cb;
			int ps_cb_len;
			Direct3DSamplerState9* ps_ss;
			Vector4i scissor;
			Direct3DDepthStencilState9* dss;
			Direct3DBlendState9* bs;
			DWORD bf;
			IDirect3DSurface9* rtv;
			IDirect3DSurface9* dsv;
		} m_state;

	public: // TODO

		struct
		{
			CComPtr<IDirect3DVertexDeclaration9> il[1];
			CComPtr<IDirect3DVertexShader9> vs[1];
			CComPtr<IDirect3DPixelShader9> ps[4];
			Direct3DSamplerState9 ln;
			Direct3DSamplerState9 pt;
			Direct3DDepthStencilState9 dss;
			Direct3DBlendState9 bs[3];
		} m_convert;

	public:
		Device9(bool silent = true);
		virtual ~Device9();

		bool Create(HWND wnd, bool vsync, bool perfhud);
		bool Reset(int mode);
		void Resize(int w, int h);
		bool IsLost(bool& canReset);
		void Flip(bool limit);

		void BeginScene();
		void DrawPrimitive();
		void EndScene();

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
		void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, bool linear = true);
		void StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, Direct3DBlendState9* bs, bool linear = true);

		void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
		void IASetVertexBuffer(IDirect3DVertexBuffer9* vb, size_t stride);
		void IASetInputLayout(IDirect3DVertexDeclaration9* layout);
		void IASetPrimitiveTopology(D3DPRIMITIVETYPE topology);
		void VSSetShaderResources(Texture* sr0, Texture* sr1);
		void VSSetShader(IDirect3DVertexShader9* vs, const float* vs_cb, int vs_cb_len);
		void PSSetShaderResources(Texture* sr0, Texture* sr1);
		void PSSetShader(IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len);
		void PSSetSamplerState(Direct3DSamplerState9* ss);
		void OMSetDepthStencilState(Direct3DDepthStencilState9* dss);
		void OMSetBlendState(Direct3DBlendState9* bs, DWORD bf);
		void OMSetRenderTargets(Texture* rt, Texture* ds, const Vector4i* scissor = NULL);

		IDirect3DDevice9* operator->() {return m_dev;}
		operator IDirect3DDevice9*() {return m_dev;}

		HRESULT CompileShader(LPCSTR source, LPCSTR entry, const D3DXMACRO* macro, IDirect3DVertexShader9** vs, const D3DVERTEXELEMENT9* layout, int count, IDirect3DVertexDeclaration9** il);
		HRESULT CompileShader(LPCSTR source, LPCSTR entry, const D3DXMACRO* macro, IDirect3DPixelShader9** ps);
	};
}