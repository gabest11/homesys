#pragma once

#include "MenuEPGSearch.dxui.h"

namespace DXUI
{
	class MenuEPGSearch : public MenuEPGSearchDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnSortClicked(Control* c);
		bool OnListPaintItem(Control* c, PaintItemParam* p);
		bool OnListClicked(Control* c, int index);
		bool OnThreadTimer(Control* c);

		std::vector<Homesys::ProgramSearch::Type> m_search_types;
		std::vector<Homesys::ProgramSearchResult> m_search_results;
		std::map<int, Homesys::Channel> m_channels;
		Thread m_thread;
		bool m_changed;

		static bool CompareByTitle(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b);
		static bool CompareByChannel(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b);
		static bool CompareByTime(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b);
		static bool CompareByQuery(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b);

	public:
		MenuEPGSearch();
	};
}
