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

#include "stdafx.h"
#include "Device9.h"
#include "../util/String.h"

namespace DXUI
{
	Device9::Device9(bool silent) 
		: m_lost(false)
		, m_inscene(0)
		, m_silent(silent)
	{
		m_rbswapped = true;

		memset(&m_pp, 0, sizeof(m_pp));
		memset(&m_d3dcaps, 0, sizeof(m_d3dcaps));

		memset(&m_state, 0, sizeof(m_state));

		m_state.bf = 0xffffffff;
	}

	Device9::~Device9()
	{
		if(m_state.vs_cb) _aligned_free(m_state.vs_cb);
		if(m_state.ps_cb) _aligned_free(m_state.ps_cb);
	}

	bool Device9::Create(HWND wnd, bool vsync, bool perfhud)
	{
		wprintf(L"Initalizing Direct3D9\n");

		if(!__super::Create(wnd, vsync, perfhud))
		{
			return false;
		}

		HRESULT hr;

		// d3d

		m_d3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));

		if(!m_d3d) 
		{
			std::wstring s = L"Cannot create Direct3D9";

			wprintf(L"%s\n", s.c_str());

			if(!m_silent) MessageBox(NULL, s.c_str(), NULL, MB_OK | MB_ICONEXCLAMATION);

			return false;
		}

		hr = m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8);

		if(FAILED(hr))
		{
			std::wstring s = L"Surface format is not available";

			wprintf(L"%s\n", s.c_str());

			if(!m_silent) MessageBox(NULL, s.c_str(), NULL, MB_OK | MB_ICONEXCLAMATION);

			return false;
		}

		hr = m_d3d->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_D24S8);

		if(FAILED(hr)) 
		{
			std::wstring s = L"Surface format is not available";

			wprintf(L"%s\n", s.c_str());

			if(!m_silent) MessageBox(NULL, s.c_str(), NULL, MB_OK | MB_ICONEXCLAMATION);

			return false;
		}

		memset(&m_d3dcaps, 0, sizeof(m_d3dcaps));

		m_d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dcaps);

		//

		wprintf(L"Vertex Shader version: %d.%d\n", D3DSHADER_VERSION_MAJOR(m_d3dcaps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(m_d3dcaps.VertexShaderVersion));
		wprintf(L"Pixel Shader version: %d.%d\n", D3DSHADER_VERSION_MAJOR(m_d3dcaps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(m_d3dcaps.PixelShaderVersion));


		if(m_d3dcaps.VertexShaderVersion < (m_d3dcaps.PixelShaderVersion & ~0x10000))
		{
			if(m_d3dcaps.VertexShaderVersion > D3DVS_VERSION(0, 0))
			{
				std::wstring s = L"Vertex Shader version too low";

				wprintf(L"%s\n", s.c_str());

				if(!m_silent) MessageBox(NULL, s.c_str(), NULL, MB_OK | MB_ICONEXCLAMATION);

				return false;
			}

			// else vertex shader should be emulated in software (gma950)
		}

		m_d3dcaps.VertexShaderVersion = m_d3dcaps.PixelShaderVersion & ~0x10000;

		if(m_d3dcaps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
		{
			SetFeatureLevel(D3D_FEATURE_LEVEL_9_3, false);
		}
		else if(m_d3dcaps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
		{
			SetFeatureLevel(D3D_FEATURE_LEVEL_9_2, false);
		}
		else
		{
			std::wstring s = L"Pixel Shader version too low";

			wprintf(L"%s\n", s.c_str());

			if(!m_silent) MessageBox(NULL, s.c_str(), NULL, MB_OK | MB_ICONEXCLAMATION);

			return false;
		}

		// msaa

		for(DWORD i = 2; i <= 16; i++)
		{
			DWORD quality[2] = {0, 0};

			if(SUCCEEDED(m_d3d->CheckDeviceMultiSampleType(m_d3dcaps.AdapterOrdinal, m_d3dcaps.DeviceType, D3DFMT_A8R8G8B8, TRUE, (D3DMULTISAMPLE_TYPE)i, &quality[0])) && quality[0] > 0
			&& SUCCEEDED(m_d3d->CheckDeviceMultiSampleType(m_d3dcaps.AdapterOrdinal, m_d3dcaps.DeviceType, D3DFMT_D24S8, TRUE, (D3DMULTISAMPLE_TYPE)i, &quality[1])) && quality[1] > 0)
			{
				m_msaa_desc.Count = i;
				m_msaa_desc.Quality = std::min<DWORD>(quality[0] - 1, quality[1] - 1);

				if(i >= m_msaa) break;
			}
		}

		//

		if(!Reset(Windowed)) 
		{
			wprintf(L"Reset device failed\n");

			return false;
		}

		m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

		// convert

		static const D3DVERTEXELEMENT9 il_convert[] =
		{
			{0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			{0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
			{0, 28, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
			D3DDECL_END()
		};

		for(int i = 0; i < sizeof(m_convert.vs) / sizeof(m_convert.vs[0]); i++)
		{
			CompileShader(m_convert_fx, Util::Format("vs_main%d", i).c_str(), NULL, &m_convert.vs[i], il_convert, sizeof(il_convert) / sizeof(il_convert[0]), &m_convert.il[i]);
		}

		for(int i = 0; i < sizeof(m_convert.ps) / sizeof(m_convert.ps[0]); i++)
		{
			CompileShader(m_convert_fx, Util::Format("ps_main%d", i).c_str(), NULL, &m_convert.ps[i]);
		}

		m_convert.dss.DepthEnable = false;
		m_convert.dss.StencilEnable = false;

		m_convert.bs[0].BlendEnable = false;
		m_convert.bs[0].RenderTargetWriteMask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
		m_convert.bs[1].BlendEnable = true;
		m_convert.bs[1].BlendOp = D3DBLENDOP_ADD;
		m_convert.bs[1].SrcBlend = D3DBLEND_SRCALPHA;
		m_convert.bs[1].DestBlend = D3DBLEND_INVSRCALPHA;
		m_convert.bs[1].BlendOpAlpha = D3DBLENDOP_ADD;
		m_convert.bs[1].SrcBlendAlpha = D3DBLEND_ONE;
		m_convert.bs[1].DestBlendAlpha = D3DBLEND_INVSRCALPHA;
		m_convert.bs[1].RenderTargetWriteMask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
		m_convert.bs[2].BlendEnable = true;
		m_convert.bs[2].BlendOp = D3DBLENDOP_ADD;
		m_convert.bs[2].SrcBlend = D3DBLEND_ONE;
		m_convert.bs[2].DestBlend = D3DBLEND_INVSRCALPHA;
		m_convert.bs[2].BlendOpAlpha = D3DBLENDOP_ADD;
		m_convert.bs[2].SrcBlendAlpha = D3DBLEND_ONE;
		m_convert.bs[2].DestBlendAlpha = D3DBLEND_ZERO;
		m_convert.bs[2].RenderTargetWriteMask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;

		m_convert.ln.FilterMin[0] = D3DTEXF_LINEAR;
		m_convert.ln.FilterMag[0] = D3DTEXF_LINEAR;
		m_convert.ln.FilterMin[1] = D3DTEXF_LINEAR;
		m_convert.ln.FilterMag[1] = D3DTEXF_LINEAR;
		m_convert.ln.AddressU = D3DTADDRESS_CLAMP;
		m_convert.ln.AddressV = D3DTADDRESS_CLAMP;

		m_convert.pt.FilterMin[0] = D3DTEXF_POINT;
		m_convert.pt.FilterMag[0] = D3DTEXF_POINT;
		m_convert.pt.FilterMin[1] = D3DTEXF_POINT;
		m_convert.pt.FilterMag[1] = D3DTEXF_POINT;
		m_convert.pt.AddressU = D3DTADDRESS_CLAMP;
		m_convert.pt.AddressV = D3DTADDRESS_CLAMP;

		//

		wprintf(L"Direct3D9 OK\n");

		return true;
	}

	bool Device9::Reset(int mode)
	{
		if(!__super::Reset(mode))
		{
			return false;
		}

		HRESULT hr;

		if(mode == DontCare)
		{
			mode = m_pp.Windowed ? Windowed : Fullscreen;
		}

		m_swapchain = NULL;

		m_vb = NULL;
		m_vb_old = NULL;

		m_vertices.start = 0;
		m_vertices.count = 0;

		if(m_state.vs_cb) _aligned_free(m_state.vs_cb);
		if(m_state.ps_cb) _aligned_free(m_state.ps_cb);

		memset(&m_state, 0, sizeof(m_state));

		m_state.bf = 0xffffffff;

		memset(&m_pp, 0, sizeof(m_pp));

		m_pp.Windowed = TRUE;
		m_pp.hDeviceWindow = m_wnd;
		m_pp.SwapEffect = D3DSWAPEFFECT_FLIP;
		m_pp.BackBufferFormat = D3DFMT_X8R8G8B8;
		m_pp.BackBufferWidth = 1;
		m_pp.BackBufferHeight = 1;
		m_pp.BackBufferCount = 2;
		m_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		if(m_vsync)
		{
			m_pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		}

		// m_pp.Flags |= D3DPRESENTFLAG_VIDEO; // enables tv-out (but I don't think anyone would still use a regular tv...)

		UINT adapter = D3DADAPTER_DEFAULT;
		D3DDEVTYPE devtype = D3DDEVTYPE_HAL;

		if(HMONITOR monitor = MonitorFromWindow(m_wnd, MONITOR_DEFAULTTONEAREST))
		{
			for(UINT i = 0, n = m_d3d->GetAdapterCount(); i < n; i++)
			{
				if(m_perfhud)
				{
					D3DADAPTER_IDENTIFIER9 id;

					if(SUCCEEDED(m_d3d->GetAdapterIdentifier(i, 0, &id)))
					{
						printf("%s\n", id.Description);

						if(strstr(id.Description, "PerfHUD") != NULL)
						{
							adapter = i;
							devtype = D3DDEVTYPE_REF;

							printf("PerfHUD adapter %d\n", adapter);

							break;
						}
					}
				}
				else
				{
					if(m_d3d->GetAdapterMonitor(i) == monitor)
					{
						adapter = i;

						break;
					}
				}
			}
		}

		if(mode == Fullscreen)
		{
			D3DDISPLAYMODE dm;

			memset(&dm, 0, sizeof(dm));

			m_d3d->GetAdapterDisplayMode(adapter, &dm);

			m_pp.Windowed = FALSE;
			m_pp.BackBufferWidth = dm.Width;
			m_pp.BackBufferHeight = dm.Height;
			// m_pp.FullScreen_RefreshRateInHz = 50;

			// m_wnd->HideFrame();
		}

		if(m_dev == NULL)
		{
			DWORD flags = m_d3dcaps.VertexProcessingCaps ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;

			flags |= D3DCREATE_MULTITHREADED;

			if(flags & D3DCREATE_HARDWARE_VERTEXPROCESSING)
			{
				flags |= D3DCREATE_PUREDEVICE;
			}

			hr = m_d3d->CreateDevice(adapter, devtype, m_wnd, flags, &m_pp, &m_dev);

			if(FAILED(hr)) 
			{
				return false;
			}
		}
		else
		{
			hr = m_dev->Reset(&m_pp);
			
			if(FAILED(hr))
			{
				m_lost = true;

				return false;
			}
		}

		if(m_pp.Windowed)
		{
			m_pp.BackBufferWidth = 1;
			m_pp.BackBufferHeight = 1;

			hr = m_dev->CreateAdditionalSwapChain(&m_pp, &m_swapchain);

			if(FAILED(hr)) 
			{
				return false;
			}
		}

		CComPtr<IDirect3DSurface9> backbuffer;

		if(m_swapchain)
		{
			hr = m_swapchain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
		}
		else
		{
			hr = m_dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
		}

		m_backbuffer = new Texture9(backbuffer);

		m_dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		m_dev->SetRenderState(D3DRS_LIGHTING, FALSE);
		m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

		m_lost = false;

		return true;
	}

	void Device9::Resize(int w, int h)
	{
		HRESULT hr;

		if(m_pp.Windowed && m_swapchain != NULL)
		{
			m_swapchain = NULL;

			delete m_backbuffer;

			m_backbuffer = NULL;

			m_pp.BackBufferWidth = w;
			m_pp.BackBufferHeight = h;

			hr = m_dev->CreateAdditionalSwapChain(&m_pp, &m_swapchain);

			if(SUCCEEDED(hr)) 
			{
				CComPtr<IDirect3DSurface9> backbuffer;

				hr = m_swapchain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);

				m_backbuffer = new Texture9(backbuffer);
			}
			else
			{
				wprintf(L"* CreateAdditionalSwapChain %08x\n", hr);

				if(0) if(hr == D3DERR_DEVICELOST) 
				{
					m_lost = true;
				}
			}
		}
	}

	bool Device9::IsLost(bool& canReset)
	{
		canReset = false;

		if(m_lost)
		{
			HRESULT hr = m_dev->TestCooperativeLevel();

			wprintf(L"TestCooperativeLevel %08x\n", hr);

			canReset = /*hr == S_OK ||*/ hr == D3DERR_DEVICENOTRESET;
		}

		return m_lost;
	}
/*
UINT64 clk_start = __rdtsc();
UINT64 clk_total = 0;
UINT64 clk1 = 0, clk2 = 0;
*/
	void Device9::Flip(bool limit)
	{
		// m_dev->EndScene();

		HRESULT hr;
/*
clk1 = __rdtsc();
*/
		if(m_swapchain)
		{
			hr = m_swapchain->Present(NULL, NULL, NULL, NULL, 0);
		}
		else
		{
			hr = m_dev->Present(NULL, NULL, NULL, NULL);
		}
/*
clk2 = __rdtsc();

clk_total += clk2 - clk1;

clk2 = __rdtsc() - clk_start;

if(clk2 > 3000000000ui64)
{
	printf("%I64d%% (%I64d %I64d)\n", clk_total * 100 / clk2, clk_total, clk2);

	clk_start = __rdtsc();
	clk_total = 0;
}
*/
		// m_dev->BeginScene();

		if(FAILED(hr))
		{
			wprintf(L"Present %08x\n", hr);

			if(hr == D3DERR_DEVICELOST) 
			{
				m_lost = true;
			}
		}

		if(0)
		if(FILE* fp = fopen("d:\\shader.fx", "r"))
		{
			std::string s;

			char buff[256];

			while(!feof(fp))
			{
				buff[0] = 0;

				fgets(buff, sizeof(buff) / sizeof(buff[0]), fp);

				s += buff;
			}

			CComPtr<IDirect3DPixelShader9> ps;

			if(SUCCEEDED(CompileShader(s.c_str(), "ps_main3", NULL, &ps)))
			{
				m_convert.ps[3] = ps;
			}
			else
			{
				printf("%s\n", s.c_str());
			}

			fclose(fp);
		}
	}

	void Device9::BeginScene()
	{
		/**/
		if(m_inscene++ == 0)
		{
			m_dev->BeginScene();
		}
		
	}

	void Device9::DrawPrimitive()
	{
		int prims = 0;

		switch(m_state.topology)
		{
	    case D3DPT_TRIANGLELIST:
			prims = m_vertices.count / 3;
			break;
	    case D3DPT_LINELIST:
			prims = m_vertices.count / 2;
			break;
	    case D3DPT_POINTLIST:
			prims = m_vertices.count;
			break;
	    case D3DPT_TRIANGLESTRIP:
	    case D3DPT_TRIANGLEFAN:
			prims = m_vertices.count - 2;
			break;
	    case D3DPT_LINESTRIP:
			prims = m_vertices.count - 1;
			break;
		}

		m_dev->DrawPrimitive(m_state.topology, m_vertices.start, prims);
	}

	void Device9::EndScene()
	{
		/**/
		ASSERT(m_inscene > 0);

		if(--m_inscene == 0)
		{
			m_dev->EndScene();
		}
		
		__super::EndScene();
	}

	void Device9::ClearRenderTarget(Texture* t, const Vector4& c)
	{
		ClearRenderTarget(t, (c * 255 + 0.5f).zyxw().rgba32());
	}

	void Device9::ClearRenderTarget(Texture* rt, DWORD c)
	{
		if(rt == NULL) return;

		CComPtr<IDirect3DSurface9> surface;
		m_dev->GetRenderTarget(0, &surface);
		m_dev->SetRenderTarget(0, *(Texture9*)rt);
		m_dev->Clear(0, NULL, D3DCLEAR_TARGET, c, 0, 0);
		m_dev->SetRenderTarget(0, surface);
	}

	void Device9::ClearDepth(Texture* t, float c)
	{
		if(t == NULL) return;

		Texture* rt = CreateRenderTarget(t->GetWidth(), t->GetHeight(), t->IsMSAA());

		CComPtr<IDirect3DSurface9> rtsurface;
		CComPtr<IDirect3DSurface9> dssurface;

		m_dev->GetRenderTarget(0, &rtsurface);
		m_dev->GetDepthStencilSurface(&dssurface);

		m_dev->SetRenderTarget(0, *(Texture9*)rt);
		m_dev->SetDepthStencilSurface(*(Texture9*)t);

		m_dev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, c, 0);

		m_dev->SetRenderTarget(0, rtsurface);
		m_dev->SetDepthStencilSurface(dssurface);

		Recycle(rt);
	}

	void Device9::ClearStencil(Texture* t, BYTE c)
	{
		if(t == NULL) return;

		Texture* rt = CreateRenderTarget(t->GetWidth(), t->GetHeight(), t->IsMSAA());

		CComPtr<IDirect3DSurface9> rtsurface;
		CComPtr<IDirect3DSurface9> dssurface;

		m_dev->GetRenderTarget(0, &rtsurface);
		m_dev->GetDepthStencilSurface(&dssurface);

		m_dev->SetRenderTarget(0, *(Texture9*)rt);
		m_dev->SetDepthStencilSurface(*(Texture9*)t);

		m_dev->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 0, c);

		m_dev->SetRenderTarget(0, rtsurface);
		m_dev->SetDepthStencilSurface(dssurface);

		Recycle(rt);
	}

	Texture* Device9::Create(int type, int w, int h, bool msaa, int format)
	{
		HRESULT hr;

		CComPtr<IDirect3DTexture9> texture;
		CComPtr<IDirect3DSurface9> surface;

		switch(type)
		{
		case Texture::RenderTarget:
			if(msaa) hr = m_dev->CreateRenderTarget(w, h, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)m_msaa_desc.Count, m_msaa_desc.Quality, FALSE, &surface, NULL);
			else hr = m_dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, (D3DFORMAT)format, D3DPOOL_DEFAULT, &texture, NULL);
			break;
		case Texture::DepthStencil:
			if(msaa) hr = m_dev->CreateDepthStencilSurface(w, h, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)m_msaa_desc.Count, m_msaa_desc.Quality, FALSE, &surface, NULL);
			else hr = m_dev->CreateDepthStencilSurface(w, h, (D3DFORMAT)format, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
			break;
		case Texture::Video:
			hr = m_dev->CreateTexture(w, h, 1, 0, (D3DFORMAT)format, D3DPOOL_MANAGED, &texture, NULL);
			break;
		case Texture::System:
			hr = m_dev->CreateOffscreenPlainSurface(w, h, (D3DFORMAT)format, D3DPOOL_SYSTEMMEM, &surface, NULL);
			break;
		}

		Texture9* t = NULL;

		if(surface)
		{
			t = new Texture9(surface);
		}

		if(texture)
		{
			t = new Texture9(texture);
		}

		if(t)
		{
			switch(type)
			{
			case Texture::RenderTarget:
				ClearRenderTarget(t, 0);
				break;
			case Texture::DepthStencil:
				ClearDepth(t, 0);
				break;
			}
		}

		return t;
	}

	Texture* Device9::CreateRenderTarget(int w, int h, bool msaa, int format)
	{
		return __super::CreateRenderTarget(w, h, msaa, format ? format : D3DFMT_A8R8G8B8);
	}

	Texture* Device9::CreateDepthStencil(int w, int h, bool msaa, int format)
	{
		return __super::CreateDepthStencil(w, h, msaa, format ? format : D3DFMT_D24S8);
	}

	Texture* Device9::CreateTexture(int w, int h, int format)
	{
		return __super::CreateTexture(w, h, format ? format : D3DFMT_A8R8G8B8);
	}

	Texture* Device9::CreateOffscreen(int w, int h, int format)
	{
		return __super::CreateOffscreen(w, h, format ? format : D3DFMT_A8R8G8B8);
	}

	Texture* Device9::CreateTexture(const std::vector<BYTE>& buff)
	{
		HRESULT hr;

		D3DXIMAGE_INFO info;

		memset(&info, 0, sizeof(info));

		hr = D3DXGetImageInfoFromFileInMemory(buff.data(), buff.size(), &info);

		if(SUCCEEDED(hr))
		{
			CComPtr<IDirect3DTexture9> texture;

			hr = D3DXCreateTextureFromFileInMemoryEx(
				m_dev, buff.data(), buff.size(),
				info.Width, info.Height, 1, 0, 
				D3DFMT_UNKNOWN, D3DPOOL_MANAGED, 
				D3DX_DEFAULT, D3DX_DEFAULT, 0,
				NULL, NULL, &texture);

			if(SUCCEEDED(hr))
			{
				return new Texture9(texture);
			}
		}

		return NULL;
	}

	PixelShader* Device9::CreatePixelShader(LPCSTR shader)
	{
		std::string s = m_convert_fx;

		s += "\n";
		s += shader;

		CComPtr<IDirect3DPixelShader9> ps;

		if(SUCCEEDED(CompileShader(s.c_str(), "main", NULL, &ps)))
		{
			return new PixelShader9(ps);
		}

		return NULL;
	}

	Texture* Device9::Resolve(Texture* t)
	{
		ASSERT(t != NULL && t->IsMSAA());

		if(Texture* dst = CreateRenderTarget(t->GetWidth(), t->GetHeight(), false, t->GetFormat()))
		{
			dst->SetScale(t->GetScale());

			m_dev->StretchRect(*(Texture9*)t, NULL, *(Texture9*)dst, NULL, D3DTEXF_POINT);

			return dst;
		}

		return NULL;
	}

	Texture* Device9::CopyOffscreen(Texture* src, const Vector4& sr, int w, int h, int format)
	{
		Texture* dst = NULL;

		if(format == 0)
		{
			format = D3DFMT_A8R8G8B8;
		}

		if(format != D3DFMT_A8R8G8B8)
		{
			ASSERT(0);

			return false;
		}

		if(Texture* rt = CreateRenderTarget(w, h, false, format))
		{
			Vector4 dr(0, 0, w, h);

			if(Texture* src2 = src->IsMSAA() ? Resolve(src) : src)
			{
				ASSERT(0); // m_convert.ps is obsolite

				StretchRect(src2, sr, rt, dr, m_convert.ps[1], NULL, 0);

				if(src2 != src) Recycle(src2);
			}

			dst = CreateOffscreen(w, h, format);

			if(dst)
			{
				m_dev->GetRenderTargetData(*(Texture9*)rt, *(Texture9*)dst);
			}

			Recycle(rt);
		}

		return dst;
	}

	void Device9::CopyRect(Texture* st, Texture* dt, const Vector4i& r, int x, int y)
	{
		Vector4i dr = Vector4i(x, y, x + r.width(), y + r.height());

		m_dev->StretchRect(*(Texture9*)st, r, *(Texture9*)dt, dr, D3DTEXF_POINT);
	}

	void Device9::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, int shader, bool linear)
	{
		StretchRect(st, sr, dt, dr, m_convert.ps[shader], NULL, 0, linear);
	}

	void Device9::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, bool linear)
	{
		StretchRect(st, sr, dt, dr, ps, ps_cb, ps_cb_len, &m_convert.bs[0], linear);
	}

	void Device9::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, Direct3DBlendState9* bs, bool linear)
	{
		BeginScene();

		// om

		OMSetDepthStencilState(&m_convert.dss);
		OMSetBlendState(bs, 0);
		OMSetRenderTargets(dt, NULL);

		// ia

		Vector4 a = Vector4(dt->GetSize()).zwxy();
		Vector4 b(0.5f, 1.0f);

		Vertex v[4] = 
		{
			{a.xyxy(b), Vector2(sr.x, sr.y), Vector2i(0xffffffff, 0)},
			{a.zyxy(b), Vector2(sr.z, sr.y), Vector2i(0xffffffff, 0)},
			{a.xwxy(b), Vector2(sr.x, sr.w), Vector2i(0xffffffff, 0)},
			{a.zwxy(b), Vector2(sr.z, sr.w), Vector2i(0xffffffff, 0)},
		};

		IASetVertexBuffer(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]));
		IASetInputLayout(m_convert.il[0]);
		IASetPrimitiveTopology(D3DPT_TRIANGLESTRIP);

		// vs

		VSSetShader(m_convert.vs[0], NULL, 0);

		// ps

		PSSetShader(ps, ps_cb, ps_cb_len);
		PSSetSamplerState(linear ? &m_convert.ln : &m_convert.pt);
		PSSetShaderResources(st, NULL);

		//

		DrawPrimitive();

		//

		EndScene();
	}

	void Device9::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
	{
		ASSERT(m_vertices.count == 0);

		if(count * stride > m_vertices.limit * m_vertices.stride)
		{
			m_vb_old = m_vb;
			m_vb = NULL;

			m_vertices.start = 0;
			m_vertices.count = 0;
			m_vertices.limit = std::max<int>(count * 3 / 2, 10000);
		}

		if(m_vb == NULL)
		{
			HRESULT hr;
			
			hr = m_dev->CreateVertexBuffer(m_vertices.limit * stride, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_vb, NULL);

			if(FAILED(hr)) return;
		}

		DWORD flags = D3DLOCK_NOOVERWRITE;

		if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
		{
			m_vertices.start = 0;

			flags = D3DLOCK_DISCARD;
		}

		size_t size = count * stride;

		void* v = NULL;

		if(SUCCEEDED(m_vb->Lock(m_vertices.start * stride, size, &v, flags)))
		{
			if(((UINT_PTR)v & 0x1f) == 0)
			{
				Vector4i::storent(v, vertices, size);
			}
			else
			{
				memcpy(v, vertices, size);
			}

			m_vb->Unlock();
		}

		m_vertices.count = count;
		m_vertices.stride = stride;

		IASetVertexBuffer(m_vb, stride);
	}

	void Device9::IASetVertexBuffer(IDirect3DVertexBuffer9* vb, size_t stride)
	{
		if(m_state.vb != vb || m_state.vb_stride != stride)
		{
			m_state.vb = vb;
			m_state.vb_stride = stride;

			m_dev->SetStreamSource(0, vb, 0, stride);
		}
	}

	void Device9::IASetInputLayout(IDirect3DVertexDeclaration9* layout)
	{
		if(m_state.layout != layout)
		{
			m_state.layout = layout;

			m_dev->SetVertexDeclaration(layout);
		}
	}

	void Device9::IASetPrimitiveTopology(D3DPRIMITIVETYPE topology)
	{
		m_state.topology = topology;
	}

	void Device9::VSSetShaderResources(Texture* sr0, Texture* sr1)
	{
		IDirect3DTexture9* srv0 = NULL;
		IDirect3DTexture9* srv1 = NULL;
		
		if(sr0) srv0 = *(Texture9*)sr0;
		if(sr1) srv1 = *(Texture9*)sr1;

		if(m_state.vs_srvs[0] != srv0)
		{
			m_state.vs_srvs[0] = srv0;

			m_dev->SetTexture(D3DVERTEXTEXTURESAMPLER0, srv0);

			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		}

		if(m_state.vs_srvs[1] != srv1)
		{
			m_state.vs_srvs[1] = srv1;

			m_dev->SetTexture(D3DVERTEXTEXTURESAMPLER1, srv1);

			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			//m_dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		}
	}

	void Device9::VSSetShader(IDirect3DVertexShader9* vs, const float* vs_cb, int vs_cb_len)
	{
		if(m_state.vs != vs)
		{
			m_state.vs = vs;

			m_dev->SetVertexShader(vs);
		}

		if(vs_cb && vs_cb_len > 0)
		{
			int size = vs_cb_len * sizeof(float) * 4;
			
			if(m_state.vs_cb_len != vs_cb_len || m_state.vs_cb == NULL || memcmp(m_state.vs_cb, vs_cb, size))
			{
				if(m_state.vs_cb == NULL || m_state.vs_cb_len < vs_cb_len)
				{
					if(m_state.vs_cb) _aligned_free(m_state.vs_cb);

					m_state.vs_cb = (float*)_aligned_malloc(size, 16);
				}

				m_state.vs_cb_len = vs_cb_len;

				memcpy(m_state.vs_cb, vs_cb, size);

				m_dev->SetVertexShaderConstantF(2, vs_cb, vs_cb_len);
			}
		}
	}
	
	void Device9::PSSetShaderResources(Texture* sr0, Texture* sr1)
	{
		IDirect3DTexture9* srv0 = NULL;
		IDirect3DTexture9* srv1 = NULL;
		
		if(sr0) srv0 = *(Texture9*)sr0;
		if(sr1) srv1 = *(Texture9*)sr1;

		if(m_state.ps_srvs[0] != srv0)
		{
			m_state.ps_srvs[0] = srv0;

			m_dev->SetTexture(0, srv0);

			if(sr0 != NULL)
			{
				Vector4 v = Vector4::zero();

				v.x = v.z = sr0->GetWidth();
				v.y = v.w = sr0->GetHeight();
				v = v.xyzw(v.rcpnr());

				m_dev->SetPixelShaderConstantF(0, (float*)&v, 4);
			}
		}

		if(m_state.ps_srvs[1] != srv1)
		{
			m_state.ps_srvs[1] = srv1;

			m_dev->SetTexture(1, srv1);
		}
	}

	void Device9::PSSetShader(IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len)
	{
		if(m_state.ps != ps)
		{
			m_state.ps = ps;

			m_dev->SetPixelShader(ps);

			m_state.ps_cb_len = 0;
		}
		
		if(ps_cb && ps_cb_len > 0)
		{
			int size = ps_cb_len * sizeof(float);
			
			if(m_state.ps_cb_len != ps_cb_len || m_state.ps_cb == NULL || memcmp(m_state.ps_cb, ps_cb, size))
			{
				if(m_state.ps_cb == NULL || m_state.ps_cb_len < ps_cb_len)
				{
					if(m_state.ps_cb) _aligned_free(m_state.ps_cb);

					m_state.ps_cb = (float*)_aligned_malloc(size, 16);
				}

				m_state.ps_cb_len = ps_cb_len;

				memcpy(m_state.ps_cb, ps_cb, size);

				m_dev->SetPixelShaderConstantF(1, ps_cb, ps_cb_len);
			}
		}
	}

	void Device9::PSSetSamplerState(Direct3DSamplerState9* ss)
	{
		if(ss && m_state.ps_ss != ss)
		{
			m_state.ps_ss = ss;

			m_dev->SetSamplerState(0, D3DSAMP_ADDRESSU, ss->AddressU);
			m_dev->SetSamplerState(0, D3DSAMP_ADDRESSV, ss->AddressV);
			m_dev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			m_dev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			m_dev->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			m_dev->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			m_dev->SetSamplerState(3, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			m_dev->SetSamplerState(3, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, ss->FilterMin[0]);
			m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, ss->FilterMag[0]);
			m_dev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		}
	}

	void Device9::OMSetDepthStencilState(Direct3DDepthStencilState9* dss)
	{
		if(m_state.dss != dss)
		{
			m_state.dss = dss;

			m_dev->SetRenderState(D3DRS_ZENABLE, dss->DepthEnable);
			m_dev->SetRenderState(D3DRS_ZWRITEENABLE, dss->DepthWriteMask);
			
			if(dss->DepthEnable)
			{
				m_dev->SetRenderState(D3DRS_ZFUNC, dss->DepthFunc);
			}

			m_dev->SetRenderState(D3DRS_STENCILENABLE, dss->StencilEnable);

			if(dss->StencilEnable)
			{
				m_dev->SetRenderState(D3DRS_STENCILMASK, dss->StencilReadMask);
				m_dev->SetRenderState(D3DRS_STENCILWRITEMASK, dss->StencilWriteMask);	
				m_dev->SetRenderState(D3DRS_STENCILFUNC, dss->StencilFunc);
				m_dev->SetRenderState(D3DRS_STENCILPASS, dss->StencilPassOp);
				m_dev->SetRenderState(D3DRS_STENCILFAIL, dss->StencilFailOp);
				m_dev->SetRenderState(D3DRS_STENCILZFAIL, dss->StencilDepthFailOp);
				m_dev->SetRenderState(D3DRS_STENCILREF, dss->StencilRef);
			}
		}
	}

	void Device9::OMSetBlendState(Direct3DBlendState9* bs, DWORD bf)
	{
		if(m_state.bs != bs || m_state.bf != bf)
		{
			m_state.bs = bs;
			m_state.bf = bf;

			m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, bs->BlendEnable);

			if(bs->BlendEnable)
			{
				m_dev->SetRenderState(D3DRS_BLENDOP, bs->BlendOp);
				m_dev->SetRenderState(D3DRS_SRCBLEND, bs->SrcBlend);
				m_dev->SetRenderState(D3DRS_DESTBLEND, bs->DestBlend);
				m_dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
				m_dev->SetRenderState(D3DRS_BLENDOPALPHA, bs->BlendOpAlpha);
				m_dev->SetRenderState(D3DRS_SRCBLENDALPHA, bs->SrcBlendAlpha);
				m_dev->SetRenderState(D3DRS_DESTBLENDALPHA, bs->DestBlendAlpha);
				m_dev->SetRenderState(D3DRS_BLENDFACTOR, bf);
			}

			m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, bs->RenderTargetWriteMask);
		}
	}

	void Device9::OMSetRenderTargets(Texture* rt, Texture* ds, const Vector4i* scissor)
	{
		IDirect3DSurface9* rtv = NULL;
		IDirect3DSurface9* dsv = NULL;

		if(rt) rtv = *(Texture9*)rt;
		if(ds) dsv = *(Texture9*)ds;

		if(m_state.rtv != rtv)
		{
			m_state.rtv = rtv;

			m_dev->SetRenderTarget(0, rtv);

			Vector4 v[2];

			v[0].x = 2.0f / rt->GetWidth();
			v[0].y = -2.0f / rt->GetHeight();
			v[0].z = 1.0f;
			v[0].w = 1.0f;
			v[1].x = -1.0f - v[0].x * 0.5f;
			v[1].y = 1.0f - v[0].y * 0.5f;
			v[1].z = 0;
			v[1].w = 0;

			m_dev->SetVertexShaderConstantF(0, (float*)&v[0], 8);
		}

		if(m_state.dsv != dsv)
		{
			m_state.dsv = dsv;

			m_dev->SetDepthStencilSurface(dsv);
		}

		Vector4i r = scissor ? *scissor : Vector4i(rt->GetSize()).zwxy();

		if(!m_state.scissor.eq(r))
		{
			m_state.scissor = r;

			m_dev->SetScissorRect(r);
		}
	}

	HRESULT Device9::CompileShader(LPCSTR source, LPCSTR entry, const D3DXMACRO* macro, IDirect3DVertexShader9** vs, const D3DVERTEXELEMENT9* layout, int count, IDirect3DVertexDeclaration9** il)
	{
		std::vector<D3DXMACRO> m;

		PrepareShaderMacro(m, macro);

		HRESULT hr;

		CComPtr<ID3DXBuffer> shader, error;

		hr = D3DXCompileShader(source, strlen(source), &m[0], NULL, entry, m_shader.vs.c_str(), 0, &shader, &error, NULL);

		if(SUCCEEDED(hr))
		{
			hr = m_dev->CreateVertexShader((DWORD*)shader->GetBufferPointer(), vs);
		}
		else if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		ASSERT(SUCCEEDED(hr));

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreateVertexDeclaration(layout, il);

		if(FAILED(hr))
		{
			return hr;
		}

		return S_OK;
	}

	HRESULT Device9::CompileShader(LPCSTR source, LPCSTR entry, const D3DXMACRO* macro, IDirect3DPixelShader9** ps)
	{
		DWORD flags = 0;

		if(m_shader.level >= D3D_FEATURE_LEVEL_9_3)
		{
			flags |= D3DXSHADER_AVOID_FLOW_CONTROL;
		}
		else
		{
			flags |= D3DXSHADER_SKIPVALIDATION;
		}

		std::vector<D3DXMACRO> m;

		PrepareShaderMacro(m, macro);

		HRESULT hr;

		CComPtr<ID3DXBuffer> shader, error;

		hr = D3DXCompileShader(source, strlen(source), &m[0], NULL, entry, m_shader.ps.c_str(), flags, &shader, &error, NULL);

		if(SUCCEEDED(hr))
		{
			/*
			CComPtr<ID3DXBuffer> disasm;

			hr = D3DXDisassembleShader((DWORD*)shader->GetBufferPointer(), FALSE, NULL, &disasm);

			if(SUCCEEDED(hr) && disasm != NULL) 
			{
				std::string s = std::string((char*)disasm->GetBufferPointer(), disasm->GetBufferSize());

				s = s;
			}
			*/

			hr = m_dev->CreatePixelShader((DWORD*)shader->GetBufferPointer(), ps);
		}
		else if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		ASSERT(SUCCEEDED(hr));

		if(FAILED(hr))
		{
			return hr;
		}

		return S_OK;
	}
}