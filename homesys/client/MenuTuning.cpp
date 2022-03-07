#include "stdafx.h"
#include "MenuTuning.h"
#include "client.h"
#include "../../DirectShow/Tuners.h"

namespace DXUI
{
	MenuTuning::MenuTuning()
		: m_scanning(false)
	{
		// TODO: m_scan.Text.Get(... m_scanning ...)

		ActivatedEvent.Add(&MenuTuning::OnActivated, this);
		DeactivatedEvent.Add(&MenuTuning::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		YellowEvent.Add0(&MenuTuning::OnYellow, this);
		m_scan.ClickedEvent.Add0(&MenuTuning::OnScan, this);
		m_scan.Text.Get([&] (std::wstring& value) {value = Control::GetString(m_scanning ? "ABORT_SCAN" : "SEARCH");});
		m_append.Visible.Get([&] (bool& value) {value = !m_scanning;});
		m_next.ClickedEvent.Add0(&MenuTuning::OnNextClicked, this);
		m_next.Enabled.Get([&] (bool& value) {value = !m_scanning && !m_presets.empty();});
		
		m_tuner_select.Enabled.Get([&] (bool& value) {value = !m_scanning;});
		m_tuner_select.Selected.ChangedEvent.Add(&MenuTuning::OnTunerSelected, this);
		m_tuner_select.m_list.ItemCount.Get([&] (int& count) {count = m_tuners.size();});
		m_tuner_select.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_tuners[p->index].name; return true;});
		
		m_package_select.Enabled.Get([&] (bool& value) {value = !m_scanning;});
		m_package_select.m_list.ItemCount.Get([&] (int& count) {count = m_packages.size();});
		m_package_select.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			Homesys::TuneRequestPackage& package = m_packages[p->index];

			p->text = package.name;

			if(!package.provider.empty()) 
			{
				p->text = package.provider + L": " + p->text; 
			}

			return true;
		});

		m_preset_list.Enabled.Get([&] (bool& value) {value = !m_scanning;});
		m_preset_list.RedEvent.Add0([&] (Control* c) -> bool 
		{
			bool disabled = true;

			for(auto i = m_presets.begin(); i != m_presets.end(); i++) 
			{
				if(i->enabled) {disabled = false; break;} 
			}
			
			for(auto i = m_presets.begin(); i != m_presets.end(); i++) 
			{
				i->enabled = disabled;
			}
			
			return true;
		});
		m_preset_list.ClickedEvent.Add([&] (Control* c, int index) -> bool {m_presets[index].enabled = !m_presets[index].enabled; return true;});
		m_preset_list.PaintItemEvent.Add(&MenuTuning::OnPaintPresetListItem, this, true);
		m_preset_list.ItemCount.Get([&] (int& count) {count = m_presets.size();});
		m_preset_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			Homesys::Preset& preset = m_presets[p->index];

			p->text = preset.name;

			if(!preset.provider.empty()) 
			{
				p->text += L" (" + preset.provider + L")"; 
			}

			if(preset.scrambled)
			{
				p->text = L"* " + p->text;
			}

			return true;
		});

		m_package_select.Visible.Get([&] (bool& value) 
		{
			int i = m_tuner_select.Selected; 
			value = i >= 0 && i < m_tuners.size() ? m_tuners[i].type != Homesys::TunerDevice::DVBF : true;
		});

		m_path_edit.Visible.Get([&] (bool& value) 
		{
			value = !m_package_select.Visible;
		});

		m_fec.push_back(std::make_pair(BDA_FEC_METHOD_NOT_SET, L"NOT_SET"));
		m_fec.push_back(std::make_pair(BDA_FEC_METHOD_NOT_DEFINED, L"NOT_DEFINED"));
		m_fec.push_back(std::make_pair(BDA_FEC_VITERBI, L"VITERBI"));
		m_fec.push_back(std::make_pair(BDA_FEC_RS_204_188, L"RS_204_188"));
		m_fec.push_back(std::make_pair(BDA_FEC_LDPC, L"LDPC"));
		m_fec.push_back(std::make_pair(BDA_FEC_BCH, L"BCH"));
		m_fec.push_back(std::make_pair(BDA_FEC_RS_147_130, L"RS_147_130"));

		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_NOT_SET, L"NOT_SET"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_NOT_DEFINED, L"NOT_DEFINED"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_1_2, L"1_2"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_2_3, L"2_3"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_3_4, L"3_4"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_3_5, L"3_5"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_4_5, L"4_5"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_5_6, L"5_6"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_5_11, L"5_11"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_7_8, L"7_8"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_1_4, L"1_4"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_1_3, L"1_3"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_2_5, L"2_5"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_6_7, L"6_7"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_8_9, L"8_9"));
		m_fec_rate.push_back(std::make_pair(BDA_BCC_RATE_9_10, L"9_10"));

		m_modulation.push_back(std::make_pair(BDA_MOD_NOT_SET, L"NOT_SET"));
		m_modulation.push_back(std::make_pair(BDA_MOD_NOT_DEFINED, L"NOT_DEFINED"));
		m_modulation.push_back(std::make_pair(BDA_MOD_16QAM, L"16QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_32QAM, L"32QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_64QAM, L"64QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_80QAM, L"80QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_96QAM, L"96QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_112QAM, L"112QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_128QAM, L"128QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_160QAM, L"160QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_192QAM, L"192QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_224QAM, L"224QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_256QAM, L"256QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_320QAM, L"320QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_384QAM, L"384QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_448QAM, L"448QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_512QAM, L"512QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_640QAM, L"640QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_768QAM, L"768QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_896QAM, L"896QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_1024QAM, L"1024QAM"));
		m_modulation.push_back(std::make_pair(BDA_MOD_QPSK, L"QPSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_BPSK, L"BPSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_OQPSK, L"OQPSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_8VSB, L"8VSB"));
		m_modulation.push_back(std::make_pair(BDA_MOD_16VSB, L"16VSB"));
		m_modulation.push_back(std::make_pair(BDA_MOD_ANALOG_AMPLITUDE, L"ANALOG_AMPLITUDE"));
		m_modulation.push_back(std::make_pair(BDA_MOD_ANALOG_FREQUENCY, L"ANALOG_FREQUENCY"));
		m_modulation.push_back(std::make_pair(BDA_MOD_8PSK, L"8PSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_RF, L"RF"));
		m_modulation.push_back(std::make_pair(BDA_MOD_16APSK, L"16APSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_32APSK, L"32APSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_NBC_QPSK, L"NBC_QPSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_NBC_8PSK, L"NBC_8PSK"));
		m_modulation.push_back(std::make_pair(BDA_MOD_DIRECTV, L"DIRECTV"));

		m_halpha.push_back(std::make_pair(BDA_HALPHA_NOT_SET, L"NOT_SET"));
		m_halpha.push_back(std::make_pair(BDA_HALPHA_NOT_DEFINED, L"NOT_DEFINED"));
		m_halpha.push_back(std::make_pair(BDA_HALPHA_1, L"1"));
		m_halpha.push_back(std::make_pair(BDA_HALPHA_2, L"2"));
		m_halpha.push_back(std::make_pair(BDA_HALPHA_4, L"4"));

		m_guard.push_back(std::make_pair(BDA_GUARD_NOT_SET, L"NOT_SET"));
		m_guard.push_back(std::make_pair(BDA_GUARD_NOT_DEFINED, L"NOT_SET"));
		m_guard.push_back(std::make_pair(BDA_GUARD_1_32, L"1_32"));
		m_guard.push_back(std::make_pair(BDA_GUARD_1_16, L"1_16"));
		m_guard.push_back(std::make_pair(BDA_GUARD_1_8, L"1_8"));
		m_guard.push_back(std::make_pair(BDA_GUARD_1_4, L"1_4"));

		m_transmission.push_back(std::make_pair(BDA_XMIT_MODE_NOT_SET, L"NOT_SET"));
		m_transmission.push_back(std::make_pair(BDA_XMIT_MODE_NOT_DEFINED, L"NOT_SET"));
		m_transmission.push_back(std::make_pair(BDA_XMIT_MODE_2K, L"2K"));
		m_transmission.push_back(std::make_pair(BDA_XMIT_MODE_8K, L"8K"));
		m_transmission.push_back(std::make_pair(BDA_XMIT_MODE_4K, L"4K"));
		m_transmission.push_back(std::make_pair(BDA_XMIT_MODE_2K_INTERLEAVED, L"2K_INTERLEAVED"));
		m_transmission.push_back(std::make_pair(BDA_XMIT_MODE_4K_INTERLEAVED, L"4K_INTERLEAVED"));

		m_polarisation.push_back(std::make_pair(BDA_POLARISATION_NOT_SET, L"NOT_SET"));
		m_polarisation.push_back(std::make_pair(BDA_POLARISATION_NOT_DEFINED, L"NOT_SET"));
		m_polarisation.push_back(std::make_pair(BDA_POLARISATION_LINEAR_H, L"LINEAR_H"));
		m_polarisation.push_back(std::make_pair(BDA_POLARISATION_LINEAR_V, L"LINEAR_V"));
		m_polarisation.push_back(std::make_pair(BDA_POLARISATION_CIRCULAR_L, L"CIRCULAR_L"));
		m_polarisation.push_back(std::make_pair(BDA_POLARISATION_CIRCULAR_R, L"CIRCULAR_R"));

		m_lnb_source.push_back(std::make_pair(-1, L"NOT_SET"));
		m_lnb_source.push_back(std::make_pair(0, L"1"));
		m_lnb_source.push_back(std::make_pair(1, L"2"));
		m_lnb_source.push_back(std::make_pair(2, L"3"));
		m_lnb_source.push_back(std::make_pair(3, L"4"));
		m_lnb_source.push_back(std::make_pair(4, L"5"));
		m_lnb_source.push_back(std::make_pair(5, L"6"));
		m_lnb_source.push_back(std::make_pair(6, L"7"));
		m_lnb_source.push_back(std::make_pair(7, L"8"));

		m_standard.push_back(std::make_pair(AnalogVideoMask_MCE_PAL, L"PAL"));
		m_standard.push_back(std::make_pair(AnalogVideoMask_MCE_NTSC, L"NTSC"));
		m_standard.push_back(std::make_pair(AnalogVideoMask_MCE_SECAM, L"SECAM"));

		m_cable.push_back(std::make_pair(0, L"Antenna"));
		m_cable.push_back(std::make_pair(1, L"Cable"));

		m_dvbt_ifec.m_list.ItemCount.Get([&] (int& count) {count = m_fec.size();});
		m_dvbt_ifec.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec[p->index].second; return true;});
		m_dvbt_ifec_rate.m_list.ItemCount.Get([&] (int& count) {count = m_fec_rate.size();});
		m_dvbt_ifec_rate.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec_rate[p->index].second; return true;});
		m_dvbt_ofec.m_list.ItemCount.Get([&] (int& count) {count = m_fec.size();});
		m_dvbt_ofec.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec[p->index].second; return true;});
		m_dvbt_ofec_rate.m_list.ItemCount.Get([&] (int& count) {count = m_fec_rate.size();});
		m_dvbt_ofec_rate.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec_rate[p->index].second; return true;});
		m_dvbt_modulation.m_list.ItemCount.Get([&] (int& count) {count = m_modulation.size();});
		m_dvbt_modulation.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_modulation[p->index].second; return true;});
		m_dvbt_halpha.m_list.ItemCount.Get([&] (int& count) {count = m_halpha.size();});
		m_dvbt_halpha.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_halpha[p->index].second; return true;});
		m_dvbt_guard.m_list.ItemCount.Get([&] (int& count) {count = m_guard.size();});
		m_dvbt_guard.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_guard[p->index].second; return true;});
		m_dvbt_transmission.m_list.ItemCount.Get([&] (int& count) {count = m_transmission.size();});
		m_dvbt_transmission.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_transmission[p->index].second; return true;});
		m_dvbt_lpifec.m_list.ItemCount.Get([&] (int& count) {count = m_fec.size();});
		m_dvbt_lpifec.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec[p->index].second; return true;});
		m_dvbt_lpifec_rate.m_list.ItemCount.Get([&] (int& count) {count = m_fec_rate.size();});
		m_dvbt_lpifec_rate.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec_rate[p->index].second; return true;});

		m_dvbc_ifec.m_list.ItemCount.Get([&] (int& count) {count = m_fec.size();});
		m_dvbc_ifec.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec[p->index].second; return true;});
		m_dvbc_ifec_rate.m_list.ItemCount.Get([&] (int& count) {count = m_fec_rate.size();});
		m_dvbc_ifec_rate.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec_rate[p->index].second; return true;});
		m_dvbc_ofec.m_list.ItemCount.Get([&] (int& count) {count = m_fec.size();});
		m_dvbc_ofec.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec[p->index].second; return true;});
		m_dvbc_ofec_rate.m_list.ItemCount.Get([&] (int& count) {count = m_fec_rate.size();});
		m_dvbc_ofec_rate.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec_rate[p->index].second; return true;});
		m_dvbc_modulation.m_list.ItemCount.Get([&] (int& count) {count = m_modulation.size();});
		m_dvbc_modulation.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_modulation[p->index].second; return true;});

		m_dvbs_ifec.m_list.ItemCount.Get([&] (int& count) {count = m_fec.size();});
		m_dvbs_ifec.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec[p->index].second; return true;});
		m_dvbs_ifec_rate.m_list.ItemCount.Get([&] (int& count) {count = m_fec_rate.size();});
		m_dvbs_ifec_rate.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec_rate[p->index].second; return true;});
		m_dvbs_ofec.m_list.ItemCount.Get([&] (int& count) {count = m_fec.size();});
		m_dvbs_ofec.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec[p->index].second; return true;});
		m_dvbs_ofec_rate.m_list.ItemCount.Get([&] (int& count) {count = m_fec_rate.size();});
		m_dvbs_ofec_rate.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_fec_rate[p->index].second; return true;});
		m_dvbs_modulation.m_list.ItemCount.Get([&] (int& count) {count = m_modulation.size();});
		m_dvbs_modulation.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_modulation[p->index].second; return true;});
		m_dvbs_polarisation.m_list.ItemCount.Get([&] (int& count) {count = m_polarisation.size();});
		m_dvbs_polarisation.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_polarisation[p->index].second; return true;});
		m_dvbs_lnb_source.m_list.ItemCount.Get([&] (int& count) {count = m_lnb_source.size();});
		m_dvbs_lnb_source.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_lnb_source[p->index].second; return true;});

		m_analog_standard.m_list.ItemCount.Get([&] (int& count) {count = m_standard.size();});
		m_analog_standard.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_standard[p->index].second; return true;});
		m_analog_cable.m_list.ItemCount.Get([&] (int& count) {count = m_cable.size();});
		m_analog_cable.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_cable[p->index].second; return true;});
		m_analog_connector.m_list.ItemCount.Get([&] (int& count) {count = m_connector.size();});
		m_analog_connector.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_connector[p->index].second; return true;});

		m_thread.TimerEvent.Add(&MenuTuning::OnThreadTimer, this);
		m_thread.TimerPeriod = 1000;

		m_signal_good.Visible.Get([&] (bool& visible) -> void {visible = m_scanning && m_stat.quality > 0;});
		m_signal_bad.Visible.Get([&] (bool& visible) -> void {visible = m_scanning && m_stat.quality <= 0;});
		m_signal_text.Visible.Get([&] (bool& visible) -> void {visible = m_scanning;});
		m_signal_text.Text.Get([&] (std::wstring& s) -> void {s = Util::Format(L"%d KHz", m_stat.freq);});

		m_signal_present_good.Visible.Get([&] (bool& visible) -> void {visible = m_scanning && m_stat.present;});
		m_signal_present_bad.Visible.Get([&] (bool& visible) -> void {visible = m_scanning && !m_stat.present;});
		m_signal_locked_good.Visible.Get([&] (bool& visible) -> void {visible = m_scanning && m_stat.locked;});
		m_signal_locked_bad.Visible.Get([&] (bool& visible) -> void {visible = m_scanning && !m_stat.locked;});
		m_signal_sqr.Visible.Get([&] (bool& visible) -> void {visible = m_scanning;});
		m_signal_sqr.Text.Get([&] (std::wstring& s) -> void 
		{
			s = Util::Format(L"F: %d KHz, S: %d, Q: %d", m_stat.freq, m_stat.strength, m_stat.quality);
			if(m_stat.received >= 0) s += Util::Format(L", R: %lld KB", m_stat.received >> 10);
		});
	}

	bool MenuTuning::OnActivated(Control* c, int dir)
	{
		m_scanning = !g_env->svc.IsTunerScanDone();

		m_thread.Create();

		if(dir >= 0)
		{
			std::list<Homesys::TunerReg> tuners;

			if(g_env->svc.GetTuners(tuners))
			{
				m_tuners.insert(m_tuners.end(), tuners.begin(), tuners.end());
			}

			m_tuner_select.Selected = 0;

			m_append.Checked = true;

			m_dvbt_ifec.Selected = 2;
			m_dvbt_ifec_rate.Selected = 4;
			m_dvbt_ofec.Selected = 3;
			m_dvbt_ofec_rate.Selected = 0;
			m_dvbt_modulation.Selected = 4;
			m_dvbt_frequency.Text = L"";
			m_dvbt_symbol_rate.TextAsInt = -1;
			m_dvbt_bandwidth.TextAsInt = 8;
			m_dvbt_halpha.Selected = 0;
			m_dvbt_guard.Selected = 4;
			m_dvbt_transmission.Selected = 3;
			m_dvbt_lpifec.Selected = 0;
			m_dvbt_lpifec_rate.Selected = 4;

			m_dvbc_ifec.Selected = 2;
			m_dvbc_ifec_rate.Selected = 4;
			m_dvbc_ofec.Selected = 3;
			m_dvbc_ofec_rate.Selected = 0;
			m_dvbc_modulation.Selected = 4;
			m_dvbc_frequency.Text = L"";
			m_dvbc_symbol_rate.TextAsInt = -1;

			m_dvbs_ifec.Selected = 2;
			m_dvbs_ifec_rate.Selected = 4;
			m_dvbs_ofec.Selected = 3;
			m_dvbs_ofec_rate.Selected = 0;
			m_dvbs_modulation.Selected = 4;
			m_dvbs_frequency.Text = L"";
			m_dvbs_symbol_rate.TextAsInt = -1;
			m_dvbs_polarisation.Selected = 2;
			m_dvbs_lnb_source.Selected = 0;

			m_analog_frequency.Text = L"";
		}

		return true;
	}

	bool MenuTuning::OnDeactivated(Control* c, int dir)
	{
		g_env->svc.StopTunerScan();

		m_scanning = false;

		m_thread.Join(false);

		if(dir < 0)
		{
			m_tuners.clear();
			m_packages.clear();
			m_presets.clear();
		}

		return true;
	}

	bool MenuTuning::OnPaintPresetListItem(Control* c, PaintItemParam* p)
	{
		Homesys::Preset& preset = m_presets[p->index];

		Vector4i r = p->rect;

		r.right = r.left + r.height();

		Texture* off = p->dc->GetTexture(p->selected ? L"CheckButtonFocused.png" : L"CheckButtonBackground.png");
		Texture* on = p->dc->GetTexture(L"CheckButtonChecked.png");

		if(off != NULL && on != NULL)
		{
			Vector4i r2 = r.deflate(3);

			p->dc->Draw(off, r2);

			if(preset.enabled) 
			{
				p->dc->Draw(on, r2);
			}
		}

		r.left = r.right + 5;
		r.right = p->rect.right;

		p->dc->Draw(p->text.c_str(), r);

		return true;
	}

	bool MenuTuning::OnScan(Control* c)
	{
		if(!m_scanning)
		{
			bool manual = !m_scan_auto.Visible;

			m_stat = Homesys::TunerStat();

			SwitchToManualScan(false);

			m_presets.clear();

			int i = m_tuner_select.Selected;

			if(i >= 0 && i < m_tuners.size())
			{
				const Homesys::TunerReg& t = m_tuners[i];

				std::list<Homesys::TuneRequest> trs;
				std::wstring provider; // TODO

				if(manual)
				{
					if(t.type == Homesys::TunerDevice::DVBT)
					{
						Homesys::TuneRequest tr;
						tr.freq = m_dvbt_frequency.TextAsInt;
						tr.ifec = m_fec[m_dvbt_ifec.Selected].first;
						tr.ifecRate = m_fec_rate[m_dvbt_ifec_rate.Selected].first;
						tr.ofec = m_fec[m_dvbt_ofec.Selected].first;
						tr.ofecRate = m_fec_rate[m_dvbt_ofec_rate.Selected].first;
						tr.modulation = m_modulation[m_dvbt_modulation.Selected].first;
						tr.symbolRate = m_dvbt_symbol_rate.TextAsInt;
						tr.bandwidth = m_dvbt_bandwidth.TextAsInt;
						tr.lpifec = m_fec[m_dvbt_ofec.Selected].first;
						tr.lpifecRate = m_fec_rate[m_dvbt_lpifec_rate.Selected].first;
						tr.halpha = m_halpha[m_dvbt_halpha.Selected].first;
						tr.guard = m_guard[m_dvbt_halpha.Selected].first;
						tr.transmissionMode = m_transmission[m_dvbt_transmission.Selected].first;
						trs.push_back(tr);
					}
					else if(t.type == Homesys::TunerDevice::DVBC)
					{
						Homesys::TuneRequest tr;
						tr.freq = m_dvbc_frequency.TextAsInt;
						tr.ifec = m_fec[m_dvbc_ifec.Selected].first;
						tr.ifecRate = m_fec_rate[m_dvbc_ifec_rate.Selected].first;
						tr.ofec = m_fec[m_dvbc_ofec.Selected].first;
						tr.ofecRate = m_fec_rate[m_dvbc_ofec_rate.Selected].first;
						tr.modulation = m_modulation[m_dvbc_modulation.Selected].first;
						tr.symbolRate = m_dvbc_symbol_rate.TextAsInt;
						trs.push_back(tr);
					}
					else if(t.type == Homesys::TunerDevice::DVBS)
					{
						Homesys::TuneRequest tr;
						tr.freq = m_dvbs_frequency.TextAsInt;
						tr.ifec = m_fec[m_dvbs_ifec.Selected].first;
						tr.ifecRate = m_fec_rate[m_dvbs_ifec_rate.Selected].first;
						tr.ofec = m_fec[m_dvbs_ofec.Selected].first;
						tr.ofecRate = m_fec_rate[m_dvbs_ofec_rate.Selected].first;
						tr.modulation = m_modulation[m_dvbs_modulation.Selected].first;
						tr.symbolRate = m_dvbs_symbol_rate.TextAsInt;
						tr.polarisation = m_polarisation[m_dvbs_polarisation.Selected].first;
						tr.lnbSource = m_lnb_source[m_dvbs_lnb_source.Selected].first;
						trs.push_back(tr);
					}
					else if(t.type == Homesys::TunerDevice::Analog)
					{
						Homesys::TuneRequest tr;
						tr.freq = m_analog_frequency.TextAsInt;
						tr.standard = m_standard[m_analog_standard.Selected].first;
						// TODO: tr.cable = m_cable[m_analog_cable.Selected].first;
						tr.connector.num = m_connector[m_analog_connector.Selected].first;
						trs.push_back(tr);
					}

					// TODO
				}
				else
				{
					if(t.type == Homesys::TunerDevice::DVBF)
					{
						std::wstring path = m_path_edit.Text;

						if(!path.empty())
						{
							Homesys::TuneRequest tr;
							tr.path = path;
							trs.push_back(tr);
						}
					}
					else
					{
						int j = m_package_select.Selected;

						if(j >= 0 && j < m_packages.size())
						{
							if(g_env->svc.StartTunerScanByPackage(t.id, m_packages[j].id))
							{
								m_scanning = true;
							}
						}
					}
				}

				if(!trs.empty())
				{
					if(g_env->svc.StartTunerScan(t.id, trs, provider.c_str()))
					{
						m_scanning = true;
					}
				}
			}
		}
		else
		{
			g_env->svc.StopTunerScan();

			m_scanning = false;
		}

		return true;
	}

	bool MenuTuning::OnNextClicked(Control* c)
	{
		if(m_presets.size())
		{
			std::list<Homesys::Preset> presets;

			for(auto i = m_presets.begin(); i != m_presets.end(); i++)
			{
				if(i->enabled)
				{
					presets.push_back(*i);
				}
			}

			if(g_env->svc.SaveTunerScanResult(m_tuners[m_tuner_select.Selected].id, presets, m_append.Checked))
			{
				DoneEvent(this);

				return true;
			}
		}

		// TODO: display an error message

		return false;
	}

	bool MenuTuning::OnTunerSelected(Control* c, int index)
	{
		m_packages.clear();
		m_presets.clear();
		m_connector.clear();

		if(index >= 0 && index < m_tuners.size())
		{
			const Homesys::TunerReg& t = m_tuners[index];

			std::list<Homesys::TuneRequestPackage> packages;

			if(g_env->svc.GetTuneRequestPackages(t.id, packages))
			{
				m_packages.insert(m_packages.end(), packages.begin(), packages.end());
			}

			for(auto i = t.connectors.begin(); i != t.connectors.end(); i++)
			{
				m_connector.push_back(std::make_pair(i->num, i->name));
			}
		}

		m_package_select.Selected = 0;

		SwitchToManualScan(false);

		return true;
	}

	bool MenuTuning::OnThreadTimer(Control* c, UINT64 n)
	{
		if(m_scanning)
		{
			std::list<Homesys::Preset> presets;

			if(g_env->svc.GetTunerScanResult(presets))
			{
				CAutoLock cAutoLock(&m_lock);

				m_presets.clear();

				if(!presets.empty())
				{
					m_presets.insert(m_presets.end(), presets.begin(), presets.end());
					m_preset_list.MakeVisible(m_presets.size() - 1);
				}
			}

			if(g_env->svc.IsTunerScanDone())
			{
				m_scanning = false;
			}
		}

		if(m_scanning)
		{
			g_env->svc.GetTunerStat(GUID_NULL, m_stat);
		}

		return true;
	}

	bool MenuTuning::OnYellow(Control* c)
	{
		SwitchToManualScan(m_scan_auto.Visible);

		return true;
	}

	void MenuTuning::SwitchToManualScan(bool manual)
	{
		if(m_scanning) return;

		if(manual)
		{
			int i = m_tuner_select.Selected; 

			if(i >= 0 && i < m_tuners.size())
			{
				const Homesys::TunerReg& t = m_tuners[i];

				m_scan_auto.Visible = false;
				m_scan_dvbt.Visible = t.type == Homesys::TunerDevice::DVBT;
				m_scan_dvbc.Visible = t.type == Homesys::TunerDevice::DVBC;
				m_scan_dvbs.Visible = t.type == Homesys::TunerDevice::DVBS;
				m_scan_analog.Visible = t.type == Homesys::TunerDevice::Analog;
			}
			else
			{
				ASSERT(0);
			}
		}
		else
		{
			m_scan_auto.Visible = true;
			m_scan_dvbt.Visible = false;
			m_scan_dvbc.Visible = false;
			m_scan_dvbs.Visible = false;
			m_scan_analog.Visible = false;
		}
	}
}