#pragma once

#include "MenuTuning.dxui.h"

namespace DXUI
{
	class MenuTuning : public MenuTuningDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnPaintPresetListItem(Control* c, PaintItemParam* p);
		bool OnScan(Control* c);
		bool OnNextClicked(Control* c);
		bool OnTunerSelected(Control* c, int index);
		bool OnThreadTimer(Control* c, UINT64 n);
		bool OnYellow(Control* c);

		std::vector<Homesys::TunerReg> m_tuners;
		std::vector<Homesys::TuneRequestPackage> m_packages;
		std::vector<Homesys::Preset> m_presets;
		Thread m_thread;
		bool m_scanning;
		Homesys::TunerStat m_stat;

		std::vector<std::pair<int, LPCWSTR>> m_fec;
		std::vector<std::pair<int, LPCWSTR>> m_fec_rate;
		std::vector<std::pair<int, LPCWSTR>> m_modulation;
		std::vector<std::pair<int, LPCWSTR>> m_halpha;
		std::vector<std::pair<int, LPCWSTR>> m_guard;
		std::vector<std::pair<int, LPCWSTR>> m_transmission;
		std::vector<std::pair<int, LPCWSTR>> m_polarisation;
		std::vector<std::pair<int, LPCWSTR>> m_lnb_source;
		std::vector<std::pair<int, LPCWSTR>> m_standard;
		std::vector<std::pair<int, LPCWSTR>> m_cable;
		std::vector<std::pair<int, std::wstring>> m_connector;

		void SwitchToManualScan(bool manual);

	public:
		MenuTuning();
	};
}
