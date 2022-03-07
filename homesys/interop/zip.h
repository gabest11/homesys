#pragma once

#include "api.h"
#include <vector>

namespace Homesys
{
	class INTEROP_API ZipDecompressor
	{
	public:
		ZipDecompressor();

		bool Decompress(LPCWSTR src, LPCWSTR dst);
		bool Decompress(LPCWSTR src, LPCWSTR file, std::vector<BYTE>& data);
	};
}
