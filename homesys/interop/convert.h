#pragma once

#include "api.h"
#include <atltime.h>
#include <string>

using namespace System;

namespace Homesys
{
	extern String^ Convert(const std::wstring& str);
	extern std::wstring Convert(String^ str);
	extern Guid Convert(const GUID& guid);
	extern GUID Convert(Guid% guid);
	extern CTime Convert(DateTime src);
	extern DateTime Convert(const CTime& src);
	extern CTimeSpan Convert(TimeSpan src);
	extern TimeSpan Convert(const CTimeSpan& src);
	extern void Convert(array<unsigned char>^ src, std::vector<BYTE>& dst);
	extern array<unsigned char>^ Convert(const std::vector<BYTE>& src);
}
