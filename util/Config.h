#pragma once

#include <atlbase.h>
#include <string>

namespace Util
{
	class Config
	{
		CRegKey m_key;

	public:
		Config(LPCWSTR app, LPCWSTR section);
		virtual ~Config();

		void SetInt(LPCWSTR name, int value);
		int GetInt(LPCWSTR name, int def = 0);

		void SetString(LPCWSTR name, LPCWSTR value);
		std::wstring GetString(LPCWSTR name, LPCWSTR def = L"");
	};
}