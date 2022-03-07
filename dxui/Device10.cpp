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
#include "Device10.h"
#include "../util/String.h"

namespace DXUI
{
	Device10::Device10()
	{
		memset(&m_state, 0, sizeof(m_state));

		m_state.topology = D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
		m_state.bf = -1;
	}

	Device10::~Device10()
	{
	}

	bool Device10::Create(HWND wnd, bool vsync, bool perfhud)
	{
		if(!__super::Create(wnd, vsync, perfhud))
		{
			return false;
		}

		HRESULT hr = E_FAIL;

		DXGI_SWAP_CHAIN_DESC scd;
		D3D10_SAMPLER_DESC sd;
		D3D10_DEPTH_STENCIL_DESC dsd;
		D3D10_RASTERIZER_DESC rd;
		D3D10_BLEND_DESC bsd;
		D3D10_BUFFER_DESC bd;
	
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

		DWORD flags = D3D10_CREATE_DEVICE_SINGLETHREADED;

		#ifdef DEBUG
		flags |= D3D10_CREATE_DEVICE_DEBUG;
		#endif

		D3D10_FEATURE_LEVEL1 levels[] = 
		{
			D3D10_FEATURE_LEVEL_10_1, 
			D3D10_FEATURE_LEVEL_10_0
		};

		for(int i = 0; i < sizeof(levels) / sizeof(levels[0]); i++)
		{
			hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, flags, levels[i], D3D10_1_SDK_VERSION, &scd, &m_swapchain, &m_dev);

			if(SUCCEEDED(hr))
			{
				if(!SetFeatureLevel((D3D_FEATURE_LEVEL)levels[i], true))
				{
					return false;
				}

				break;
			}
		}

		if(FAILED(hr)) return false;

		// msaa

		for(DWORD i = 2; i <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; i++)
		{
			UINT quality[2] = {0, 0};

			if(SUCCEEDED(m_dev->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, i, &quality[0])) && quality[0] > 0
			&& SUCCEEDED(m_dev->CheckMultisampleQualityLevels(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, i, &quality[1])) && quality[1] > 0)
			{
				m_msaa_desc.Count = i;
				m_msaa_desc.Quality = std::min<DWORD>(quality[0] - 1, quality[1] - 1);

				if(i >= m_msaa) break;
			}
		}

		memset(&bd, 0, sizeof(bd));

		bd.ByteWidth = sizeof(Vector4) * 1;
		bd.Usage = D3D10_USAGE_DEFAULT;
		bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

		hr = m_dev->CreateBuffer(&bd, NULL, &m_ps_cb);

		// convert

		D3D10_INPUT_ELEMENT_DESC il_convert[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 28, D3D10_INPUT_PER_VERTEX_DATA, 0},
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

		bsd.BlendEnable[0] = false;
		bsd.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

		hr = m_dev->CreateBlendState(&bsd, &m_convert.bs[0]);

		bsd.BlendEnable[0] = true;
		bsd.BlendOp = D3D10_BLEND_OP_ADD;
		bsd.SrcBlend = D3D10_BLEND_SRC_ALPHA;
		bsd.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
		bsd.BlendOpAlpha = D3D10_BLEND_OP_ADD;
		bsd.SrcBlendAlpha = D3D10_BLEND_ONE;
		bsd.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
		bsd.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

		hr = m_dev->CreateBlendState(&bsd, &m_convert.bs[1]);

		bsd.BlendEnable[0] = true;
		bsd.BlendOp = D3D10_BLEND_OP_ADD;
		bsd.SrcBlend = D3D10_BLEND_ONE;
		bsd.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
		bsd.BlendOpAlpha = D3D10_BLEND_OP_ADD;
		bsd.SrcBlendAlpha = D3D10_BLEND_ONE;
		bsd.DestBlendAlpha = D3D10_BLEND_ZERO;
		bsd.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

		hr = m_dev->CreateBlendState(&bsd, &m_convert.bs[2]);

		memset(&bd, 0, sizeof(bd));

		bd.ByteWidth = sizeof(Vector4) * 2;
		bd.Usage = D3D10_USAGE_DEFAULT;
		bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

		hr = m_dev->CreateBuffer(&bd, NULL, &m_convert.vs_cb);

		//

		memset(&rd, 0, sizeof(rd));

		rd.FillMode = D3D10_FILL_SOLID;
		rd.CullMode = D3D10_CULL_NONE;
		rd.FrontCounterClockwise = false;
		rd.DepthBias = false;
		rd.DepthBiasClamp = 0;
		rd.SlopeScaledDepthBias = 0;
		rd.DepthClipEnable = false; // ???
		rd.ScissorEnable = true;
		rd.MultisampleEnable = true;
		rd.AntialiasedLineEnable = false;

		hr = m_dev->CreateRasterizerState(&rd, &m_rs);

		m_dev->RSSetState(m_rs);

		//

		memset(&sd, 0, sizeof(sd));

		sd.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
		sd.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
		sd.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
		sd.MaxLOD = FLT_MAX;
		sd.MaxAnisotropy = 16; 
		sd.ComparisonFunc = D3D10_COMPARISON_NEVER;

		hr = m_dev->CreateSamplerState(&sd, &m_convert.ln);

		sd.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;

		hr = m_dev->CreateSamplerState(&sd, &m_convert.pt);

		//

		Reset(Windowed);

		//

		return true;
	}

	bool Device10::Reset(int mode)
	{
		if(!__super::Reset(mode))
			return false;

		// TODO

		Resize(1, 1);

		return true;
	}

	void Device10::Resize(int w, int h)
	{
		if(m_swapchain != NULL)
		{
			delete m_backbuffer;

			DXGI_SWAP_CHAIN_DESC scd;
			memset(&scd, 0, sizeof(scd));
			m_swapchain->GetDesc(&scd);
			m_swapchain->ResizeBuffers(scd.BufferCount, w, h, scd.BufferDesc.Format, 0);
			
			CComPtr<ID3D10Texture2D> backbuffer;
			m_swapchain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&backbuffer);
			m_backbuffer = new Texture10(backbuffer);
		}

		m_state.rtv = NULL;
		m_state.dsv = NULL;
	}

	void Device10::Flip(bool limit)
	{
		m_swapchain->Present(m_vsync && limit ? 1 : 0, 0);
	}

	void Device10::DrawPrimitive()
	{
		m_dev->Draw(m_vertices.count, m_vertices.start);
	}

	void Device10::ClearRenderTarget(Texture* t, const Vector4& c)
	{
		if(t == NULL) return;

		m_dev->ClearRenderTargetView(*(Texture10*)t, c.v);
	}

	void Device10::ClearRenderTarget(Texture* t, DWORD c)
	{
		if(t == NULL) return;

		Vector4 color = Vector4(c) * (1.0f / 255);

		m_dev->ClearRenderTargetView(*(Texture10*)t, color.v);
	}

	void Device10::ClearDepth(Texture* t, float c)
	{
		if(t == NULL) return;

		m_dev->ClearDepthStencilView(*(Texture10*)t, D3D10_CLEAR_DEPTH, c, 0);
	}

	void Device10::ClearStencil(Texture* t, BYTE c)
	{
		if(t == NULL) return;

		m_dev->ClearDepthStencilView(*(Texture10*)t, D3D10_CLEAR_STENCIL, 0, c);
	}

	Texture* Device10::Create(int type, int w, int h, bool msaa, int format)
	{
		HRESULT hr;

		D3D10_TEXTURE2D_DESC desc;

		memset(&desc, 0, sizeof(desc));

		desc.Width = w;
		desc.Height = h;
		desc.Format = (DXGI_FORMAT)format;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D10_USAGE_DEFAULT;

		if(msaa) 
		{
			desc.SampleDesc = m_msaa_desc;
		}

		switch(type)
		{
		case Texture::RenderTarget:
			desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
			break;
		case Texture::DepthStencil:
			desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;// | D3D10_BIND_SHADER_RESOURCE;
			break;
		case Texture::Video:
			desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
			break;
		case Texture::System:
			desc.Usage = D3D10_USAGE_STAGING;
			desc.CPUAccessFlags |= D3D10_CPU_ACCESS_READ | D3D10_CPU_ACCESS_WRITE;
			break;
		}

		Texture10* t = NULL;

		CComPtr<ID3D10Texture2D> texture;

		hr = m_dev->CreateTexture2D(&desc, NULL, &texture);

		if(SUCCEEDED(hr))
		{
			t = new Texture10(texture);

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

	Texture* Device10::CreateRenderTarget(int w, int h, bool msaa, int format)
	{
		return __super::CreateRenderTarget(w, h, msaa, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	Texture* Device10::CreateDepthStencil(int w, int h, bool msaa, int format)
	{
		return __super::CreateDepthStencil(w, h, msaa, format ? format : DXGI_FORMAT_D32_FLOAT_S8X24_UINT); // DXGI_FORMAT_R32G8X24_TYPELESS
	}

	Texture* Device10::CreateTexture(int w, int h, int format)
	{
		return __super::CreateTexture(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	Texture* Device10::CreateOffscreen(int w, int h, int format)
	{
		return __super::CreateOffscreen(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	Texture* Device10::CreateTexture(const std::vector<BYTE>& buff)
	{
		HRESULT hr;

		D3DX10_IMAGE_LOAD_INFO info;

		memset(&info, 0, sizeof(info));

		D3DX10_IMAGE_INFO src_info;

		memset(&src_info, 0, sizeof(src_info));

		hr = D3DX10GetImageInfoFromMemory(buff.data(), buff.size(), NULL, &src_info, NULL);

		info.Width = src_info.Width;
		info.Height = src_info.Height;
    	info.Depth = D3DX10_DEFAULT;
    	info.FirstMipLevel = D3DX10_DEFAULT;
    	info.MipLevels = 1;
    	info.Usage = D3D10_USAGE_DEFAULT;
    	info.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    	info.CpuAccessFlags = 0;
    	info.MiscFlags = 0;
    	info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    	info.Filter = D3DX10_DEFAULT;
    	info.MipFilter = D3DX10_DEFAULT;
    	info.pSrcInfo = &src_info;

		memset(&src_info, 0, sizeof(src_info));

		CComPtr<ID3D10Resource> texture;

		hr = D3DX10CreateTextureFromMemory(m_dev, buff.data(), buff.size(), &info, NULL, &texture, NULL);

		if(SUCCEEDED(hr))
		{
			return new Texture10(CComQIPtr<ID3D10Texture2D>(texture));
		}

		return NULL;
	}

	PixelShader* Device10::CreatePixelShader(LPCSTR shader)
	{
		std::string s = m_convert_fx;

		s += "\n";
		s += shader;

		CComPtr<ID3D10PixelShader> ps;

		if(SUCCEEDED(CompileShader(s.c_str(), "main", NULL, &ps)))
		{
			return new PixelShader10(ps);
		}

		return NULL;
	}

	Texture* Device10::Resolve(Texture* t)
	{
		ASSERT(t != NULL && t->IsMSAA());

		if(Texture* dst = CreateRenderTarget(t->GetWidth(), t->GetHeight(), false, t->GetFormat()))
		{
			dst->SetScale(t->GetScale());

			m_dev->ResolveSubresource(*(Texture10*)dst, 0, *(Texture10*)t, 0, (DXGI_FORMAT)t->GetFormat());

			return dst;
		}

		return NULL;
	}

	Texture* Device10::CopyOffscreen(Texture* src, const Vector4& sr, int w, int h, int format)
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
				m_dev->CopyResource(*(Texture10*)dst, *(Texture10*)rt);
			}

			Recycle(rt);
		}

		return dst;
	}

	void Device10::CopyRect(Texture* st, Texture* dt, const Vector4i& r, int x, int y)
	{
		D3D10_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};

		m_dev->CopySubresourceRegion(*(Texture10*)dt, 0, x, y, 0, *(Texture10*)st, 0, &box);
	}

	void Device10::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, int shader, bool linear)
	{
		StretchRect(st, sr, dt, dr, m_convert.ps[shader], NULL, linear);
	}

	void Device10::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, bool linear)
	{
		StretchRect(st, sr, dt, dr, ps, ps_cb, m_convert.bs[0], linear);
	}

	void Device10::StretchRect(Texture* st, const Vector4& sr, Texture* dt, const Vector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, ID3D10BlendState* bs, bool linear)
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
		IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

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

	void Device10::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
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
			D3D10_BUFFER_DESC bd;

			memset(&bd, 0, sizeof(bd));

			bd.Usage = D3D10_USAGE_DYNAMIC;
			bd.ByteWidth = m_vertices.limit * stride;
			bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

			HRESULT hr;
			
			hr = m_dev->CreateBuffer(&bd, NULL, &m_vb);

			if(FAILED(hr)) return;
		}

		D3D10_MAP type = D3D10_MAP_WRITE_NO_OVERWRITE;

		if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
		{
			m_vertices.start = 0;

			type = D3D10_MAP_WRITE_DISCARD;
		}


		void* v = NULL;

		if(SUCCEEDED(m_vb->Map(type, 0, &v)))
		{
			BYTE* dst = (BYTE*)v + m_vertices.start * stride;

			if(((UINT_PTR)dst & 0x1f) == 0)
			{
				Vector4i::storent(dst, vertices, count * stride);
			}
			else
			{
				memcpy(dst, vertices, count * stride);
			}

			m_vb->Unmap();
		}

		m_vertices.count = count;
		m_vertices.stride = stride;

		IASetVertexBuffer(m_vb, stride);
	}

	void Device10::IASetVertexBuffer(ID3D10Buffer* vb, size_t stride)
	{
		if(m_state.vb != vb || m_state.vb_stride != stride)
		{
			m_state.vb = vb;
			m_state.vb_stride = stride;

			UINT offset = 0;

			m_dev->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		}
	}

	void Device10::IASetInputLayout(ID3D10InputLayout* layout)
	{
		if(m_state.layout != layout)
		{
			m_state.layout = layout;

			m_dev->IASetInputLayout(layout);
		}
	}

	void Device10::IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY topology)
	{
		if(m_state.topology != topology)
		{
			m_state.topology = topology;

			m_dev->IASetPrimitiveTopology(topology);
		}
	}

	void Device10::VSSetShader(ID3D10VertexShader* vs, ID3D10Buffer* vs_cb)
	{
		if(m_state.vs != vs)
		{
			m_state.vs = vs;

			m_dev->VSSetShader(vs);
		}
		
		// TODO

		m_dev->VSSetConstantBuffers(0, 1, &m_convert.vs_cb.p);

		//

		if(m_state.vs_cb != vs_cb)
		{
			m_state.vs_cb = vs_cb;

			m_dev->VSSetConstantBuffers(1, 1, &vs_cb);
		}
	}

	void Device10::GSSetShader(ID3D10GeometryShader* gs)
	{
		if(m_state.gs != gs)
		{
			m_state.gs = gs;

			m_dev->GSSetShader(gs);
		}
	}

	void Device10::PSSetShaderResources(Texture* sr0, Texture* sr1)
	{
		ID3D10ShaderResourceView* srv0 = NULL;
		ID3D10ShaderResourceView* srv1 = NULL;

		if(sr0) srv0 = *(Texture10*)sr0;
		if(sr1) srv1 = *(Texture10*)sr1;

		if(m_state.ps_srv[0] != srv0 || m_state.ps_srv[1] != srv1)
		{
			m_state.ps_srv[0] = srv0;
			m_state.ps_srv[1] = srv1;

			ID3D10ShaderResourceView* srvs[] = {srv0, srv1};
		
			m_dev->PSSetShaderResources(0, 2, srvs);

			if(sr0 != NULL)	
			{
				Vector4 v = Vector4::zero();

				v.x = sr0->GetWidth();
				v.y = sr0->GetHeight();

				m_dev->UpdateSubresource(m_ps_cb, 0, NULL, &v, 0, 0);
			}
		}
	}

	void Device10::PSSetShader(ID3D10PixelShader* ps, ID3D10Buffer* ps_cb)
	{
		if(m_state.ps != ps)
		{
			m_state.ps = ps;

			m_dev->PSSetShader(ps);
		}
		
		if(m_state.ps_cb != ps_cb)
		{
			m_state.ps_cb = ps_cb;

			m_dev->PSSetConstantBuffers(1, 1, &ps_cb);
		}
	}

	void Device10::PSSetSamplerState(ID3D10SamplerState* ss0, ID3D10SamplerState* ss1)
	{
		if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1)
		{
			m_state.ps_ss[0] = ss0;
			m_state.ps_ss[1] = ss1;

			ID3D10SamplerState* sss[] = {ss0, ss1};

			m_dev->PSSetSamplers(0, 2, sss);
		}
	}

	void Device10::OMSetDepthStencilState(ID3D10DepthStencilState* dss, BYTE sref)
	{
		if(m_state.dss != dss || m_state.sref != sref)
		{
			m_state.dss = dss;
			m_state.sref = sref;

			m_dev->OMSetDepthStencilState(dss, sref);
		}
	}

	void Device10::OMSetBlendState(ID3D10BlendState* bs, float bf)
	{
		if(m_state.bs != bs || m_state.bf != bf)
		{
			m_state.bs = bs;
			m_state.bf = bf;

			float BlendFactor[] = {bf, bf, bf, 0};

			m_dev->OMSetBlendState(bs, BlendFactor, 0xffffffff);
		}
	}

	void Device10::OMSetRenderTargets(Texture* rt, Texture* ds, const Vector4i* scissor)
	{
		ID3D10RenderTargetView* rtv = NULL;
		ID3D10DepthStencilView* dsv = NULL;

		if(rt) rtv = *(Texture10*)rt;
		if(ds) dsv = *(Texture10*)ds;

		if(m_state.rtv != rtv || m_state.dsv != dsv)
		{
			m_state.rtv = rtv;
			m_state.dsv = dsv;

			m_dev->OMSetRenderTargets(1, &rtv, dsv);
		}

		if(m_state.viewport != rt->GetSize())
		{
			m_state.viewport = rt->GetSize();

			D3D10_VIEWPORT vp;

			memset(&vp, 0, sizeof(vp));
			
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			vp.Width = rt->GetWidth();
			vp.Height = rt->GetHeight();
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;

			m_dev->RSSetViewports(1, &vp);

			Vector4 v[2];

			v[0].x = 2.0f / m_state.viewport.x;
			v[0].y = -2.0f / m_state.viewport.y;
			v[0].z = 1.0f;
			v[0].w = 1.0f;
			v[1].x = -1.0f;
			v[1].y = 1.0f;
			v[1].z = 0;
			v[1].w = 0;

			m_dev->UpdateSubresource(m_convert.vs_cb, 0, NULL, &v[0], 0, 0);
		}

		Vector4i r = scissor ? *scissor : Vector4i(rt->GetSize()).zwxy();

		if(!m_state.scissor.eq(r))
		{
			m_state.scissor = r;

			m_dev->RSSetScissorRects(1, r);
		}
	}

	HRESULT Device10::CompileShader(LPCSTR source, LPCSTR entry, D3D10_SHADER_MACRO* macro, ID3D10VertexShader** vs, D3D10_INPUT_ELEMENT_DESC* layout, int count, ID3D10InputLayout** il)
	{
		HRESULT hr;

		std::vector<D3D10_SHADER_MACRO> m;

		PrepareShaderMacro(m, macro);

		CComPtr<ID3D10Blob> shader, error;

	    hr = D3DX10CompileFromMemory(source, strlen(source), NULL, &m[0], NULL, entry, m_shader.vs.c_str(), 0, 0, NULL, &shader, &error, NULL);
		
		if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreateVertexShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), vs);

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

	HRESULT Device10::CompileShader(LPCSTR source, LPCSTR entry, D3D10_SHADER_MACRO* macro, ID3D10GeometryShader** gs)
	{
		HRESULT hr;

		std::vector<D3D10_SHADER_MACRO> m;

		PrepareShaderMacro(m, macro);

		CComPtr<ID3D10Blob> shader, error;

		hr = D3DX10CompileFromMemory(source, strlen(source), NULL, &m[0], NULL, entry, m_shader.gs.c_str(), 0, 0, NULL, &shader, &error, NULL);
		
		if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreateGeometryShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), gs);

		if(FAILED(hr))
		{
			return hr;
		}

		return hr;
	}

	HRESULT Device10::CompileShader(LPCSTR source, LPCSTR entry, D3D10_SHADER_MACRO* macro, ID3D10PixelShader** ps)
	{
		HRESULT hr;

		std::vector<D3D10_SHADER_MACRO> m;

		PrepareShaderMacro(m, macro);

		CComPtr<ID3D10Blob> shader, error;

		hr = D3DX10CompileFromMemory(source, strlen(source), NULL, &m[0], NULL, entry, m_shader.ps.c_str(), 0, 0, NULL, &shader, &error, NULL);
		
		if(error)
		{
			printf("%s\n", (const char*)error->GetBufferPointer());
		}

		if(FAILED(hr))
		{
			return hr;
		}

		hr = m_dev->CreatePixelShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), ps);

		if(FAILED(hr))
		{
			return hr;
		}

		return hr;
	}
}