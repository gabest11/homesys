#include "stdafx.h"
#include "base64.h"
#include "convert.h"

using namespace System;
using namespace System::Text;
using namespace System::Diagnostics;

namespace Homesys
{
	std::wstring Base64::Encode(LPCWSTR str)
	{
		System::Text::ASCIIEncoding^ enc = gcnew System::Text::ASCIIEncoding();

		return Convert(System::Convert::ToBase64String(enc->GetBytes(Convert(str))));
	}

	std::wstring Base64::Decode(LPCWSTR str)
	{
		System::Text::ASCIIEncoding^ enc = gcnew System::Text::ASCIIEncoding();

		return Convert(enc->GetString(System::Convert::FromBase64String(Convert(str))));
	}
}