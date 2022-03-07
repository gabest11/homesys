#pragma once 

#include "DeviceContext.h"
#include <string>
#include <list>
#include <map>

namespace DXUI
{
	class DeviceContext9 : public DeviceContext
	{
	public:
		DeviceContext9(Device* dev, ResourceLoader* loader);
		virtual ~DeviceContext9();

		void DrawTriangleList(const Vertex* v, int n, Texture* t, PixelShader* ps = NULL);
		void DrawLineList(const Vertex* v, int n);
	};
}