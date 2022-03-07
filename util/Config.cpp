#include "stdafx.h"
#include "Config.h"
#include "String.h"

namespace Util
{
	Config::Config(LPCWSTR app, LPCWSTR section)
	{
		std::wstring s = Format(L"Software\\%s\\%s", app, section);

		m_key.Create(HKEY_CURRENT_USER, s.c_str());
	}

	Config::~Config()
	{
	}

	void Config::SetInt(LPCWSTR name, int value)
	{
		m_key.SetDWORDValue(name, (DWORD)value);
	}

	int Config::GetInt(LPCWSTR name, int def)
	{
		int ret = def;

		DWORD dw;
		
		if(ERROR_SUCCESS == m_key.QueryDWORDValue(name, dw))
		{
			ret = (int)dw;
		}

		return ret;
	}

	void Config::SetString(LPCWSTR name, LPCWSTR value)
	{
		m_key.SetStringValue(name, value);
	}

	std::wstring Config::GetString(LPCWSTR name, LPCWSTR def)
	{
		std::wstring ret = def;

		ULONG chars = 0;

		if(ERROR_SUCCESS == m_key.QueryStringValue(name, NULL, &chars))
		{
			wchar_t* buff = new wchar_t[chars + 1];

			if(ERROR_SUCCESS == m_key.QueryStringValue(name, buff, &chars))
			{
				ret = buff;
			}

			delete [] buff;
		}

		return ret;
	}
}
