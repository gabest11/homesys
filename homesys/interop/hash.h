#pragma once

#include "api.h"
#include <string>

namespace Homesys
{
	class INTEROP_API Hash
	{
	public:
 		static std::wstring AsMD5(LPCWSTR str);
	};
}
