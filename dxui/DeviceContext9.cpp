#pragma once 

#include "stdafx.h"
#include "DeviceContext9.h"
#include "Device9.h"

namespace DXUI
{
	DeviceContext9::DeviceContext9(Device* dev, ResourceLoader* loader)
		: DeviceContext(dev, loader)
	{
	}

	DeviceContext9::~DeviceContext9()
	{
	}

	void DeviceContext9::DrawTriangleList(const Vertex* v, int n, Texture* t, PixelShader* ps)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetBlendState(&dev->m_convert.bs[GetBlendStateIndex(t)], 0);
		dev->OMSetRenderTargets(m_rt ? m_rt : m_dev->GetBackbuffer(), NULL, GetScissor());

		// ia

		dev->IASetVertexBuffer(v, sizeof(v[0]), n * 3);
		dev->IASetInputLayout(dev->m_convert.il[0]); 
		dev->IASetPrimitiveTopology(D3DPT_TRIANGLELIST);

		// vs

		dev->VSSetShader(dev->m_convert.vs[0], NULL, 0);

		// ps

		if(ps != NULL)
		{
			dev->PSSetShader(*(PixelShader9*)ps, NULL, 0);
		}
		else
		{
			dev->PSSetShader(dev->m_convert.ps[GetShaderIndex(t)], NULL, 0);
		}

		dev->PSSetSamplerState(&dev->m_convert.ln);

		if(t != NULL && t->HasFlag(Texture::StereoImage))
		{
			Texture* t2 = GetTexture(L"tridelity-multi.png");

			dev->PSSetShaderResources(t, t2);
		}
		else
		{
			dev->PSSetShaderResources(t, NULL);
		}

		//

		dev->DrawPrimitive();

		//

		dev->EndScene();
	}

	void DeviceContext9::DrawLineList(const Vertex* v, int n)
	{
		Device9* dev = (Device9*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(&dev->m_convert.dss);
		dev->OMSetBlendState(&dev->m_convert.bs[1], 0);
		dev->OMSetRenderTargets(m_rt ? m_rt : m_dev->GetBackbuffer(), NULL, GetScissor());

		// ia

		dev->IASetVertexBuffer(v, sizeof(v[0]), n * 2);
		dev->IASetInputLayout(dev->m_convert.il[0]); 
		dev->IASetPrimitiveTopology(D3DPT_LINELIST);

		// vs

		dev->VSSetShader(dev->m_convert.vs[0], NULL, 0);

		// ps

		dev->PSSetShader(dev->m_convert.ps[0], NULL, 0);
		dev->PSSetSamplerState(&dev->m_convert.ln);
		dev->PSSetShaderResources(NULL, NULL);

		//

		dev->DrawPrimitive();

		//

		dev->EndScene();
	}
}