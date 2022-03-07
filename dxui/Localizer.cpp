#pragma once

#include "stdafx.h"
#include "Localizer.h"
#include "../util/String.h"

namespace DXUI
{
	Localizer::Localizer()
	{
		SetLanguage(L"hun");

		SetString("DAY_MON", L"H");
		SetString("DAY_TUE", L"K");
		SetString("DAY_WEN", L"Sze");
		SetString("DAY_THU", L"Cs");
		SetString("DAY_FRI", L"P");
		SetString("DAY_SAT", L"Szo");
		SetString("DAY_SUN", L"V");
		SetString("YES", L"Igen");
		SetString("NO", L"Nem");
		SetString("OFF", L"Ki");
		SetString("ON", L"Be");
		SetString("ERROR", L"Hiba");

		SetLanguage(L"eng");

		SetString("DAY_MON", L"Mon");
		SetString("DAY_TUE", L"Tue");
		SetString("DAY_WEN", L"Wen");
		SetString("DAY_THU", L"Thu");
		SetString("DAY_FRI", L"Fri");
		SetString("DAY_SAT", L"Sat");
		SetString("DAY_SUN", L"Sun");
		SetString("YES", L"Yes");
		SetString("NO", L"No");
		SetString("OFF", L"Off");
		SetString("ON", L"On");
		SetString("ERROR", L"Error");
	}

	Localizer::~Localizer()
	{
		for(auto i = m_l2m.begin(); i != m_l2m.end(); i++)
		{
			delete i->second;
		}
	}

	std::map<std::string, std::wstring>* Localizer::GetMap(LPCWSTR lang)
	{
		std::map<std::string, std::wstring>* m = m_map;

		if(lang != NULL)
		{
			auto i = m_l2m.find(lang);

			if(i == m_l2m.end())
			{
				m = new std::map<std::string, std::wstring>();

				m_l2m[lang] = m;
			}
			else
			{
				m = i->second;
			}
		}

		return m;
	}

	void Localizer::SetLanguage(LPCWSTR lang)
	{
		if(lang != NULL && wcslen(lang) == 3)
		{
			m_map = GetMap(lang);
		}
		else
		{
			ASSERT(0);
		}
	}

	void Localizer::SetString(LPCSTR id, LPCWSTR str, LPCWSTR lang)
	{
		std::map<std::string, std::wstring>* m = GetMap(lang);

		(*m)[id] = str;
	}

	LPCWSTR Localizer::GetString(LPCSTR id, LPCWSTR lang)
	{
		std::map<std::string, std::wstring>* m = GetMap(lang);

		auto i = m->find(id);

		if(i == m->end())
		{
			m = GetMap(L"eng");

			i = m->find(id);

			if(i == m->end())
			{
				//ASSERT(0);

				printf("Missing localized string %s\n", id);

				return L"";
			}
		}

		return i->second.c_str();
	}
}

