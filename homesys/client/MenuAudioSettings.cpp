#include "stdafx.h"
#include "MenuAudioSettings.h"
#include "client.h"
#include "../../DirectShow/MPADecFilter.h"
#include "../../DirectShow/AudioSwitcher.h"

namespace DXUI
{
	static struct {LPCWSTR name; int ac3, dts;} s_speakers[] = 
	{
		{L"Mono", A52_MONO, DTS_MONO},
		{L"Stereo", A52_STEREO, DTS_STEREO},
		{L"3F", A52_3F, DTS_3F},
		{L"2F + 1R", A52_2F1R, DTS_2F1R},
		{L"3F + 1R", A52_3F1R, DTS_3F1R},
		{L"2F + 2R", A52_2F2R, DTS_2F2R},
		{L"3F + 2R", A52_3F2R, DTS_3F2R},
		{L"SPDIF", -1, -1}
	};

	MenuAudioSettings::MenuAudioSettings() 
	{
		ActivatedEvent.Add(&MenuAudioSettings::OnActivated, this);
		DeactivatedEvent.Add(&MenuAudioSettings::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_next.ClickedEvent.Add0(&MenuAudioSettings::OnNext, this);

		m_speakers_select.m_list.ItemCount.Get([&] (int& count) {count = sizeof(s_speakers) / sizeof(s_speakers[0]);});
		m_speakers_select.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = s_speakers[p->index].name; return true;});
	}

	bool MenuAudioSettings::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_speakers_select.Selected = -1;
			m_lfe.Checked = false;
			m_normalize.Checked = false;

			HRESULT hr;

			{
				CComPtr<IMPADecoderFilter> p = new CMPADecoderFilter(NULL, &hr);

				int config = p->GetSpeakerConfig(IMPADecoderFilter::AC3);

				if(config >= 0)
				{
					if(config & A52_LFE)
					{
						m_lfe.Checked = true;
					}

					config &= A52_CHANNEL_MASK;
				}

				for(int i = 0; i < sizeof(s_speakers) / sizeof(s_speakers[0]); i++)
				{
					if(s_speakers[i].ac3 == config)
					{
						m_speakers_select.Selected = i;

						break;
					}
				}

				if(m_speakers_select.Selected < 0)
				{
					m_speakers_select.Selected = 1;
				}
			}

			{
				CComPtr<IAudioSwitcherFilter> p = new CAudioSwitcherFilter(NULL, &hr);

				bool normalize, recover;
				float boost;

				p->GetNormalizeBoost(normalize, recover, boost);

				m_normalize.Checked = normalize;
			}
		}

		return true;
	}

	bool MenuAudioSettings::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool MenuAudioSettings::OnNext(Control* c)
	{
		HRESULT hr;

		{
			int i = m_speakers_select.Selected;
			
			int ac3 = s_speakers[i].ac3;
			int dts = s_speakers[i].dts;

			if(ac3 >= 0)
			{
				if(m_lfe.Checked)
				{
					ac3 |= A52_LFE;
					dts |= DTS_LFE;
				}
			}

			CComPtr<IMPADecoderFilter> p = new CMPADecoderFilter(NULL, &hr);

			p->SetSpeakerConfig(IMPADecoderFilter::AC3, ac3);
			p->SetSpeakerConfig(IMPADecoderFilter::DTS, dts);

			p->SaveConfig();
		}

		{
			CComPtr<IAudioSwitcherFilter> p = new CAudioSwitcherFilter(NULL, &hr);

			p->SetNormalizeBoost(m_normalize.Checked, true, 1.0f);

			p->SaveConfig();
		}

		DoneEvent(this);
		
		return true;
	}
}