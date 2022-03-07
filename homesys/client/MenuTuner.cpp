#include "stdafx.h"
#include "MenuTuner.h"
#include "client.h"

namespace DXUI
{
	MenuTuner::MenuTuner()
	{
		ActivatedEvent.Add(&MenuTuner::OnActivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_next.ClickedEvent.Add0(&MenuTuner::OnNextClicked, this);
	}

	MenuTuner::~MenuTuner()
	{
		Clear();
	}

	void MenuTuner::Clear()
	{
		for(auto i = m_tuners.begin(); i != m_tuners.end(); i++)
		{
			delete *i;
		}

		m_tuners.clear();
		m_devs.clear();
	}

	bool MenuTuner::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			Clear();

			if(m_name.Text->empty())
			{
				DWORD len = 256;

				wchar_t* buff = new wchar_t[len];

				buff[0] = 0;

				if(GetComputerName(buff, &len))
				{
					buff[len] = 0;

					m_name.Text = buff;
				}

				delete [] buff;
			}

			Control* c = m_tuners_label.Parent;

			Vector4i r;

			r.left = 0;
			r.right = c->Width;
			r.top = c->MapClientRect(&m_tuners_label).bottom + 10;
			r.bottom = r.top + 30;

			std::list<Homesys::TunerReg> tuners;

			g_env->svc.GetTuners(tuners);

			g_env->svc.GetAllTuners(m_devs);

			for(auto i = m_devs.begin(); i != m_devs.end(); i++)
			{
				const Homesys::TunerDevice& td = *i;

				CheckButton* b = new CheckButton();

				b->Create(r, c);
				b->Text = td.name;
				b->UserData = (UINT_PTR)&td;
				b->AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Right, Anchor::Top);

				auto j = std::find_if(tuners.begin(), tuners.end(), [&] (const Homesys::TunerReg& tr) -> bool 
				{
					return tr.dispname == td.dispname;
				});

				if(j != tuners.end() || tuners.empty() && i->type != Homesys::TunerDevice::DVBF)
				{
					b->Checked = true;
				}

				m_tuners.push_back(b);
				
				r += Vector4i(0, 40).xyxy();
			}

			m_prev.ToFront();
			m_next.ToFront();
		}

		return true;
	}

	bool MenuTuner::OnNextClicked(Control* c)
	{
		if(m_name.Text->empty())
		{
			m_name.Focused = true;
			
			return false;
		}

		std::list<Homesys::TunerDevice> devs;

		for(auto i = m_tuners.begin(); i != m_tuners.end(); i++)
		{
			CheckButton* b = *i;

			if(b->Checked)
			{
				devs.push_back(*(Homesys::TunerDevice*)(UINT_PTR)b->UserData);
			}
		}

		if(g_env->svc.RegisterMachine(m_name.Text->c_str(), devs))
		{
			DoneEvent(this);

			return true;
		}

		// TODO: display an error message

		return false;
	}
}