#pragma once 

#include "stdafx.h"
#include "DeviceContext10.h"
#include "Device10.h"

namespace DXUI
{
	DeviceContext10::DeviceContext10(Device* dev, ResourceLoader* loader)
		: DeviceContext(dev, loader)
	{
	}

	DeviceContext10::~DeviceContext10()
	{
	}

	void DeviceContext10::DrawTriangleList(const Vertex* v, int n, Texture* t, PixelShader* ps)
	{
		Device10* dev = (Device10*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(dev->m_convert.dss, 0);
		dev->OMSetBlendState(dev->m_convert.bs[GetBlendStateIndex(t)], 0);
		dev->OMSetRenderTargets(m_rt ? m_rt : m_dev->GetBackbuffer(), NULL, GetScissor());

		// ia

		dev->IASetVertexBuffer(v, sizeof(v[0]), n * 3);
		dev->IASetInputLayout(dev->m_convert.il);
		dev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// vs

		dev->VSSetShader(dev->m_convert.vs, NULL);

		// gs

		dev->GSSetShader(NULL);

		// ps

		if(ps != NULL)
		{
			dev->PSSetShader(*(PixelShader10*)ps, NULL);
		}
		else
		{
			dev->PSSetShader(dev->m_convert.ps[GetShaderIndex(t)], NULL);
		}

		dev->PSSetSamplerState(dev->m_convert.ln, NULL);
		dev->PSSetShaderResources(t, NULL);

		//

		dev->DrawPrimitive();

		//

		dev->EndScene();

		dev->PSSetShaderResources(NULL, NULL);
	}

	void DeviceContext10::DrawLineList(const Vertex* v, int n)
	{
		Device10* dev = (Device10*)m_dev;

		dev->BeginScene();

		// om

		dev->OMSetDepthStencilState(dev->m_convert.dss, 0);
		dev->OMSetBlendState(dev->m_convert.bs[1], 0);
		dev->OMSetRenderTargets(m_rt ? m_rt : m_dev->GetBackbuffer(), NULL, GetScissor());

		// ia

		dev->IASetVertexBuffer(v, sizeof(v[0]), n * 2);
		dev->IASetInputLayout(dev->m_convert.il);
		dev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);

		// vs

		dev->VSSetShader(dev->m_convert.vs, NULL);

		// gs

		dev->GSSetShader(NULL);

		// ps

		dev->PSSetShader(dev->m_convert.ps[0], NULL);
		dev->PSSetSamplerState(dev->m_convert.ln, NULL);
		dev->PSSetShaderResources(NULL, NULL);

		//

		dev->DrawPrimitive();

		//

		dev->EndScene();
	}
}