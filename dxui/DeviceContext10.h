#pragma once 

#include "DeviceContext.h"
#include <string>
#include <list>
#include <map>

namespace DXUI
{
	class DeviceContext10 : public DeviceContext
	{
	public:
		DeviceContext10(Device* dev, ResourceLoader* loader);
		virtual ~DeviceContext10();

		void DrawTriangleList(const Vertex* v, int n, Texture* t, PixelShader* ps = NULL);
		void DrawLineList(const Vertex* v, int n);
	};
}