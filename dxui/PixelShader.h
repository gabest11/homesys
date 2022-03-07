#pragma once

#include "../util/Vector.h"
#include <vector>
#include <atlcomcli.h>

namespace DXUI
{
	class PixelShader
	{
	public:
		PixelShader() {}
		virtual ~PixelShader() {}

		virtual operator bool() {ASSERT(0); return false;}
	};
}