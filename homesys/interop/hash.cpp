#include "stdafx.h"
#include "hash.h"
#include "convert.h"

using namespace System;
using namespace System::IO;
using namespace System::Diagnostics;
using namespace System::Security::Cryptography;

namespace Homesys
{
	std::wstring Hash::AsMD5(LPCWSTR str)
	{
		MD5^ md5 = MD5::Create();

		array<unsigned char>^ data = md5->ComputeHash(System::Text::Encoding::Default->GetBytes(Convert(str)));

		System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();

		for(int i = 0; i < data->Length; i++)
		{
			sb->Append(data[i].ToString("x2"));
		}

		return Convert(sb->ToString());
	}
}