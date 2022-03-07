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
#include "Device11.h"
#include "../util/String.h"

namespace DXUI
{
	Device11::Device11()
	{
		memset(&m_state, 0, sizeof(m_state));

		m_state.topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		m_state.bf = -1;
	}

	Device11::~Device11()
	{
	}

	bool Device11::Create(HWND wnd, bool vsync, bool perfhud)
	{
		if(!__super::Create(wnd, vsync, perfhud))
		{
			return false;
		}

		HRESULT hr = E_FAIL;

		DXGI_SWAP_CHAIN_DESC scd;
		D3D11_SAMPLER_DESC sd;
		D3D11_DEPTH_STENCIL_DESC dsd;
		D3D11_RASTERIZER_DESC rd;
		D3D11_BLEND_DESC bsd;
		D3D11_BUFFER_DESC bd;
		memset(&scd, 0, sizeof(scd));

		scd.BufferCount = 2;
		scd.BufferDesc.Width = 1;
		scd.BufferDesc.Height = 1;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//scd.BufferDesc.RefreshRate.Numerator = 60;
		//scd.BufferDesc.RefreshRate.Denominator = 1;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.OutputWindow = m_wnd;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		scd.Windowed = TRUE;

		DWORD flags = D3D11_CREATE_DEVICE_SINGLETHREADED;

		#ifdef DEBUG
		flags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif

		D3D_FEATURE_LEVEL levels[] = 
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};

		D3D_FEATURE_LEVEL level;

		hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, sizeof(levels) / sizeof(levels[0]), D3D11_SDK_VERSION, &scd, &m_swapchain, &m_dev, &level, &m_ctx);
		// hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &scd, &m_swapchain, &m_dev, &level, &m_ctx);

		if(FAILED(hr)) return false;

		if(!SetFeatureLevel(level, true))
		{
			return false;
		}

		D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS options;
		
		hr = m_dev->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &options, sizeof(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS));

		// msaa

		for(DWORD i = 2; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i++)
		{
			UINT quality[2] = {0, 0};

			if(SUCCEEDED(m_dev->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, i, &quality[0])) && quality[0] > 0
			&& SUCCEEDED(m_dev->CheckMultisampleQualityLevels(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, i, &quality[1])) && quality[1] > 0)
			{
				m_msaa_desc.Count = i;
				m_msaa_desc.Quality = std::min<UINT>(quality[0] - 1, quality[1] - 1);

				if(i >= m_msaa) break;
			}
		}

		memset(&bd, 0, sizeof(bd));

		bd.ByteWidth = sizeof(Vector4) * 1;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		hr = m_dev->CreateBuffer(&bd, NULL, &m_ps_cb);

		// convert

		D3D11_INPUT_ELEMENT_DESC il_convert[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		hr = CompileShader(m_convert_fx, "vs_main0", NULL, &m_convert.vs, il_convert, sizeof(il_convert) / sizeof(il_convert[0]), &m_convert.il);

		for(int i = 0; i < sizeof(m_convert.ps) / sizeof(m_convert.ps[0]); i++)
		{
			hr = CompileShader(m_convert_fx, Util::Format("ps_main%d", i).c_str(), NULL, &m_convert.ps[i]);
		}

		memset(&dsd, 0, sizeof(dsd));

		dsd.DepthEnable = false;
		dsd.StencilEnable = false;

		hr = m_dev->CreateDepthStencilState(&dsd, &m_convert.dss);

		memset(&bsd, 0, sizeof(bsd));

		bsd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		hr = m_dev->CreateBlendState(&bsd, &m_convert.bs[0]);

		bsd.RenderTarget[0].BlendEnable = true;
		bsd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bsd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bsd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bsd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bsd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bsd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		bsd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		hr = m_dev->CreateBlendState(&bsd, &m_convert.bs[1]);

		bsd.RenderTarget[0].BlendEnable = true;
		bsd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bsd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		bsd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bsd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bsd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bsd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bsd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		hr = m_dev->CreateBlendState(&bsd, &m_convert.bs[2]);

		memset(&bd, 0, sizeof(bd));

		bd.ByteWidth = sizeof(Vector4) * 2;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		hr = m_dev->CreateBuffer(&bd, NULL, &m_convert.vs_cb);

		//

		memset(&rd, 0, sizeof(rd));

		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.FrontCounterClockwise = false;
		rd.DepthBias = false;
		rd.DepthBiasClamp = 0;
		rd.SlopeScaledDepthBias = 0;
		rd.DepthClipEnable = false; // ???
		rd.ScissorEnable = true;
		rd.MultisampleEnable = true;
		rd.AntialiasedLineEnable = false;

		hr = m_dev->CreateRasterizerState(&rd, &m_rs);

		m_ctx->RSSetState(m_rs);

		//

		memset(&sd, 0, sizeof(sd));

		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.MaxLOD = FLT_MAX;
		sd.MaxAnisotropy = 16; 
		sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

		hr = m_dev->CreateSamplerState(&sd, &m_convert.ln);

		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

		hr = m_dev->CreateSamplerState(&sd, &m_convert.pt);

		//

		Reset(Windowed);

		//

		return true;
	}

	bool Device11::Reset(int mode)
	{
		if(!__super::Reset(mode))
			return false;

		// TODO

		Resize(1, 1);

		return true;
	}

	void Device11::Resize(int w, int h)
	{
		if(m_swapchain != NULL)
		{
			delete m_backbuffer;

			DXGI_SWAP_CHAIN_DESC scd;
			memset(&scd, 0, sizeof(scd));
			m_swapchain->GetDesc(&scd);
			m_swapchain->ResizeBuffers(scd.BufferCount, w, h, scd.BufferDesc.Format, 0);
			
			CComPtr<ID3D11Texture2D> backbuffer;
			m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
			m_backbuffer = new Texture11(backbuffer);
		}

		m_state.rtv = NULL;
		m_state.dsv = NULL;
	}

	void Device11::Flip(bool limit)
	{
		m_swapchain->Present(m_vsync && limit ? 1 : 0, 0);
	}

	void Device11::DrawPrimitive()
	{
		m_ctx->Draw(m_vertices.count, m_vertices.start);
	}

	void Device11::ClearRenderTarget(Texture* t, const Vector4& c)
	{
		if(t == NULL) return;

		m_ctx->ClearRenderTargetView(*(Texture11*)t, c.v);
	}

	void Device11::ClearRenderTarget(Texture* t, DWORD c)
	{
		if(t == NULL) return;

		Vector4 color = Vector4(c) * (1.0f / 255);

		m_ctx->ClearRenderTargetView(*(Texture11*)t, color.v);
	}

	void Device11::ClearDepth(Texture* t, float c)
	{
		if(t == NULL) return;

		m_ctx->ClearDepthStencilView(*(Texture11*)t, D3D11_CLEAR_DEPTH, c, 0);
	}

	void Device11::ClearStencil(Texture* t, BYTE c)
	{
		if(t == NULL) return;

		m_ctx->ClearDepthStencilView(*(Texture11*)t, D3D11_CLEAR_STENCIL, 0, c);
	}

	Texture* Device11::Create(int type, int w, int h, bool msaa, int format)
	{
		HRESULT hr;

		D3D11_TEXTURE2D_DESC desc;

		memset(&desc, 0, sizeof(desc));

		desc.Width = w;
		desc.Height = h;
		desc.Format = (DXGI_FORMAT)format;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;

		if(msaa) 
		{
			desc.SampleDesc = m_msaa_desc;
		}

		switch(type)
		{
		case Texture::RenderTarget:
			desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			break;
		case Texture::DepthStencil:
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			break;
		case Texture::Video:
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			break;
		case Texture::System:
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
			break;
		}

		Texture11* t = NULL;

		CComPtr<ID3D11Texture2D> texture;

		hr = m_dev->CreateTexture2D(&desc, NULL, &texture);

		if(SUCCEEDED(hr))
		{
			t = new Texture11(texture);

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

	Texture* Device11::CreateRenderTarget(int w, int h, bool msaa, int format)
	{
		return __super::CreateRenderTarget(w, h, msaa, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	Texture* Device11::CreateDepthStencil(int w, int h, bool msaa, int format)
	{
		return __super::CreateDepthStencil(w, h, msaa, format ? format : DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
	}

	Texture* Device11::CreateTexture(int w, int h, int format)
	{
		return __super::CreateTexture(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	Texture* Device11::CreateOffscreen(int w, int h, int format)
	{
		return __super::CreateOffscreen(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
	}
	
	Texture* Device11::CreateTexture(const std::vector<BYTE>& buff)
	{
		HRESULT hr;

		D3DX11_IMAGE_LOAD_INFO info;

		memset(&info, 0, sizeof(info));

		D3DX11_IMAGE_INFO src_info;

		memset(&src_info, 0, sizeof(src_info));

		hr = D3DX11GetImageInfoFromMemory(buff.data(), buff.size(), NULL, &src_info, NULL);

		info.Width = src_info.Width;
		info.Height = src_info.Height;
    	info.Depth = D3DX11_DEFAULT;
    	info.FirstMipLevel = D3DX11_DEFAULT;
    	info.MipLevels = 1;
    	info.Usage = D3D11_USAGE_DEFAULT;
    	info.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    	info.CpuAccessFlags = 0;
    	info.MiscFlags = 0;
    	info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    	info.Filter = D3DX11_DEFAULT;
    	info.MipFilter = D3DX11_DEFAULT;
    	info.pSrcInfo = &src_info;

		memset(&src_info, 0, sizeof(src_info));

		CComPtr<ID3D11Resource> texture;

		hr = D3DX11CreateTextureFromMemory(m_dev, buff.data(), buff.size(), &info, NULL, &texture, NULL);

		if(SUCCEEDED(hr))
		{
			return new Texture11(CComQIPtr<ID3D11Texture2D>(texture));
		}

		return NULL;
	}

	PixelShader* Device11::CreatePixelShader(LPCSTR shader)
	{
		std::string s = m_convert_fx;

		s += "\n";
		s += shader;

		CComPtr<ID3D11PixelShader> ps;

		if(SUCCEEDED(CompileShader(s.c_str(), "main", NULL, &ps)))
		{
			return new PixelShader11(ps);
		}

		return NULL;
	}

	Texture* Device11::Resolve(Texture* t)
	{
		ASSERT(t != NULL && t->IsMSAA());

		if(Texture* dst = CreateRenderTarget(t->GetWidth(), t->GetHeight(), false, t->GetFormat()))
		{
			dst->SetScale(t->GetScale());

			m_ctx->ResolveSubresource(*(Texture11*)dst, 0, *(Texture11*)t, 0, (DXGI_FORMAT)t->GetFormat());

			return dst;
		}

		return NULL;
	}

	Texture* Device11::CopyOffscreen(Texture* src, const Vector4& sr, int w, int h, int format)
	{
		Texture* dst = NULL;

		if(format == 0)
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		if(format != DXGI_FORMAT_R8G8B8A8_UNORM && format != DXGI_FORMAT_R16_UINT)
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

				StretchRect(src2, sr, rt, dr, m_convert.ps[format == DXGI_FORMAT_R16_UINT ? 1 : 0], NULL);

				if(src2 != src) Recycle(src2);
			}

			dst = CreateOffscreen(w, h, format);

			if(dst)
			{
				m_ctx->CopyResource(*(Texture11*)dst, *(Texture11*)rt);
			}

			Recycle(rt);
		}

		return dst;
	}

	void Device11::CopyRect(Texture* st, Texture* dt, const Vector4i& r, int x, int y)
	{
		D3D11_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};

		m_ctx->CopySubresourceRegion(*(Texture11*)dt, 0, x, y, 0, *(Texture11*)st, 0, &box);
	}

	void Device11::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, int shader, bool linear)
	{
		StretchRect(st, sr, dt, dr, m_convert.ps[shader], NULL, linear);
	}

	void Device11::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, bool linear)
	{
		StretchRect(st, sr, dt, dr, ps, ps_cb, m_convert.bs[0], linear);
	}

	void Device11::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, ID3D11BlendState* bs, bool linear)
	{
		BeginScene();

		Vector2i ds = dt->GetSize();

		// om

		OMSetDepthStencilState(m_convert.dss, 0);
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
		IASetInputLayout(m_convert.il);
		IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// vs

		VSSetShader(m_convert.vs, NULL);

		// gs

		GSSetShader(NULL);

		// ps

		PSSetShader(ps, ps_cb);
		PSSetSamplerState(linear ? m_convert.ln : m_convert.pt, NULL);
		PSSetShaderResources(st, NULL);

		//

		DrawPrimitive();

		//

		EndScene();

		PSSetShaderResources(NULL, NULL);
	}

	void Device11::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
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
			D3D11_BUFFER_DESC bd;

			memset(&bd, 0, sizeof(bd));

			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = m_vertices.limit * stride;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			HRESULT hr;
			
			hr = m_dev->CreateBuffer(&bd, NULL, &m_vb);

			if(FAILED(hr)) return;
		}

		D3D11_MAP type = D3D11_MAP_WRITE_NO_OVERWRITE;

		if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
		{
			m_vertices.start = 0;

			type = D3D11_MAP_WRITE_DISCARD;
		}

		D3D11_MAPPED_SUBRESOURCE m;

		if(SUCCEEDED(m_ctx->Map(m_vb, 0, type, 0, &m)))
		{
			BYTE* dst = (BYTE*)m.pData + m_vertices.start * stride;

			if(((UINT_PTR)dst & 0x1f) == 0)
			{
				Vector4i::storent(dst, vertices, count * stride);
			}
			else
			{
				memcpy(dst, vertices, count * stride);
			}

			m_ctx->Unmap(m_vb, 0);
		}

		m_vertices.count = count;
		m_vertices.stride = stride;

		IASetVertexBuffer(m_vb, stride);
	}

	void Device11::IASetVertexBuffer(ID3D11Buffer* vb, size_t stride)
	{
		if(m_state.vb != vb || m_state.vb_stride != stride)
		{
			m_state.vb = vb;
			m_state.vb_stride = stride;

			UINT offset = 0;

			m_ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		}
	}

	void Device11::IASetInputLayout(ID3D11InputLayout* layout)
	{
		if(m_state.layout != layout)
		{
			m_state.layout = layout;

			m_ctx->IASetInputLayout(layout);
		}
	}

	void Device11::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
	{
		if(m_state.topology != topology)
		{
			m_state.topology = topology;

			m_ctx->IASetPrimitiveTopology(topology);
		}
	}

	void Device11::VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb)
	{
		if(m_state.vs != vs)
		{
			m_state.vs = vs;

			m_ctx->VSSetShader(vs, NULL, 0);
		}
		
		// TODO

		m_ctx->VSSetConstantBuffers(0, 1, &m_convert.vs_cb);

		//

		if(m_state.vs_cb != vs_cb)
		{
			m_state.vs_cb = vs_cb;

			m_ctx->VSSetConstantBuffers(1, 1, &vs_cb);
		}
	}

	void Device11::GSSetShader(ID3D11GeometryShader* gs)
	{
		if(m_state.gs != gs)
		{
			m_state.gs = gs;

			m_ctx->GSSetShader(gs, NULL, 0);
		}
	}

	void Device11::PSSetShaderResources(Texture* sr0, Texture* sr1)
	{
		ID3D11ShaderResourceView* srv0 = NULL;
		ID3D11ShaderResourceView* srv1 = NULL;
		
		if(sr0) srv0 = *(Texture11*)sr0;
		if(sr1) srv1 = *(Texture11*)sr1;

		if(m_state.ps_srv[0] != srv0 || m_state.ps_srv[1] != srv1)
		{
			m_state.ps_srv[0] = srv0;
			m_state.ps_srv[1] = srv1;

			ID3D11ShaderResourceView* srvs[] = {srv0, srv1};
		
			m_ctx->PSSetShaderResources(0, 2, srvs);

			if(sr0 != NULL)	
			{
				Vector4 v = Vector4::zero();

				v.x = sr0->GetWidth();
				v.y = sr0->GetHeight();

				m_ctx->UpdateSubresource(m_ps_cb, 0, NULL, &v, 0, 0);
			}
		}
	}

	void Device11::PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb)
	{
		if(m_state.ps != ps)
		{
			m_state.ps = ps;

			m_ctx->PSSetShader(ps, NULL, 0);
		}
		
		if(m_state.ps_cb != ps_cb)
		{
			m_state.ps_cb = ps_cb;

			m_ctx->PSSetConstantBuffers(1, 1, &ps_cb);
		}
	}

	void Device11::PSSetSamplerState(ID3D11SamplerState* ss0, ID3D11SamplerState* ss1)
	{
		if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1)
		{
			m_state.ps_ss[0] = ss0;
			m_state.ps_ss[1] = ss1;

			ID3D11SamplerState* sss[] = {ss0, ss1};

			m_ctx->PSSetSamplers(0, 2, sss);
		}
	}

	void Device11::OMSetDepthStencilState(ID3D11DepthStencilState* dss, BYTE sref)
	{
		if(m_state.dss != dss || m_state.sref != sref)
		{
			m_state.dss = dss;
			m_state.sref = sref;

			m_ctx->OMSetDepthStencilState(dss, sref);
		}
	}

	void Device11::OMSetBlendState(ID3D11BlendState* bs, float bf)
	{
		if(m_state.bs != bs || m_state.bf != bf)
		{
			m_state.bs = bs;
			m_state.bf = bf;

			float BlendFactor[] = {bf, bf, bf, 0};

			m_ctx->OMSetBlendState(bs, BlendFactor, 0xffffffff);
		}
	}

	void Device11::OMSetRenderTargets(Texture* rt, Texture* ds, const Vector4i* scissor)
	{
		ID3D11RenderTargetView* rtv = NULL;
		ID3D11DepthStencilView* dsv = NULL;

		if(rt) rtv = *(Texture11*)rt;
		if(ds) dsv = *(Texture11*)ds;

		if(m_state.rtv != rtv || m_state.dsv != dsv)
		{
			m_state.rtv = rtv;
			m_state.dsv = dsv;

			m_ctx->OMSetRenderTargets(1, &rtv, dsv);
		}

		if(m_state.viewport != rt->GetSize())
		{
			m_state.viewport = rt->GetSize();

			D3D11_VIEWPORT vp;

			memset(&vp, 0, sizeof(vp));
			
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			vp.Width = (float)rt->GetWidth();
			vp.Height = (float)rt->GetHeight();
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;

			m_ctx->RSSetViewports(1, &vp);

			Vector4 v[2];

			v[0].x = 2.0f / m_state.viewport.x;
			v[0].y = -2.0f / m_state.viewport.y;
			v[0].z = 1.0f;
			v[0].w = 1.0f;
			v[1].x = -1.0f;
			v[1].y = 1.0f;
			v[1].z = 0;
			v[1].w = 0;

			m_ctx->UpdateSubresource(m_convert.vs_cb, 0, NULL, &v[0], 0, 0);
		}

		Vector4i r = scissor ? *scissor : Vector4i(rt->GetSize()).zwxy();

		if(!m_state.scissor.eq(r))
		{
			m_state.scissor = r;

			m_ctx->RSSetScissorRects(1, r);
		}
	}

	HRESULT Device11::CompileShader(LPCSTR source, LPCSTR entry, D3D11_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il)
	{
		HRESULT hr;

		std::vector<D3D11_SHADER_MACRO> m;

		PrepareShaderMacro(m, macro);

		CComPtr<ID3D11Blob> shader, error;

	    hr = D3DX11CompileFromMemory(source, strlen(source), NULL, &m[0], NULL, entry, m_shader.vs.c_str(), 0, 0, NULL, &shader, &error, NULL);
		
		if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreateVertexShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), NULL, vs);

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreateInputLayout(layout, count, shader->GetBufferPointer(), shader->GetBufferSize(), il);

		if(FAILED(hr))
		{
			return hr;
		}

		return hr;
	}

	HRESULT Device11::CompileShader(LPCSTR source, LPCSTR entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs)
	{
		HRESULT hr;

		std::vector<D3D11_SHADER_MACRO> m;

		PrepareShaderMacro(m, macro);

		CComPtr<ID3D11Blob> shader, error;

		hr = D3DX11CompileFromMemory(source, strlen(source), NULL, &m[0], NULL, entry, m_shader.gs.c_str(), 0, 0, NULL, &shader, &error, NULL);
		
		if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreateGeometryShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), NULL, gs);

		if(FAILED(hr))
		{
			return hr;
		}

		return hr;
	}

	HRESULT Device11::CompileShader(LPCSTR source, LPCSTR entry, D3D11_SHADER_MACRO* macro, ID3D11PixelShader** ps)
	{
		HRESULT hr;

		std::vector<D3D11_SHADER_MACRO> m;

		PrepareShaderMacro(m, macro);

		CComPtr<ID3D10Blob> shader, error;

		hr = D3DX11CompileFromMemory(source, strlen(source), NULL, &m[0], NULL, entry, m_shader.ps.c_str(), 0, 0, NULL, &shader, &error, NULL);
		
		if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreatePixelShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(),NULL,  ps);

		if(FAILED(hr))
		{
			return hr;
		}

		return hr;
	}
}