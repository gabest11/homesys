#pragma once

#include "api.h"
#include <string>

namespace Homesys
{
	class INTEROP_API Base64
	{
	public:
		static std::wstring Encode(LPCWSTR str);
		static std::wstring Decode(LPCWSTR str);
	};
}
