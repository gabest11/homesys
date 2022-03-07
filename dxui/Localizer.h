#pragma once

#include "Control.h"
#include <string>
#include <map>

namespace DXUI
{
	class Localizer
	{
		std::map<std::wstring, std::map<std::string, std::wstring>*> m_l2m;
		std::map<std::string, std::wstring>* m_map;

		std::map<std::string, std::wstring>* GetMap(LPCWSTR lang);

	public:
		Localizer();
		virtual ~Localizer();

		void SetLanguage(LPCWSTR lang);
		void SetString(LPCSTR id, LPCWSTR str, LPCWSTR lang = NULL);
		LPCWSTR GetString(LPCSTR id, LPCWSTR lang = NULL);
	};
}
