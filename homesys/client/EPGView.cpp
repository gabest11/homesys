#include "stdafx.h"
#include "EPGView.h"
#include "client.h"

namespace DXUI
{
	EPGView::EPGView()
	{
		Start.Chain(&m_header.Start);
		Duration.Chain(&m_header.Duration);

		ActivatedEvent.Add(&EPGView::OnActivated, this);
		DeactivatedEvent.Add(&EPGView::OnDeactivated, this);
		ProgramClicked.Add(&EPGView::OnProgramClicked, this);
		m_overlay.PaintForegroundEvent.Add(&EPGView::OnPaintOverlayForeground, this);

		PresetRows = 6;
		Compact = false;
	}

	bool EPGView::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		m_overlay.Create(ClientRect, &m_rowstack);
		m_overlay.AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Right, Anchor::Bottom);
		m_overlay.Enabled = false;

		Control* root = GetRoot();

		int w = m_dialog.Width;
		int h = m_dialog.Height;
		int x = (root->Width - w) / 2;
		int y = (root->Height - h) / 2;

		m_dialog.Parent = root;
		m_dialog.Move(Vector4i(x, y, x + w, y + h));

		return true;
	}

	void EPGView::MakeVisible(int presetId, CTime time)
	{
		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();

			if(CComQIPtr<ITunerPlayer> t = p)
			{
				if(m_tunerId != t->GetTunerId())
				{
					InitRows(t);
				}
			}
		}

		bool focus = false;

		auto i = std::find_if(m_presets.begin(), m_presets.end(), [&] (const Homesys::Preset& p) -> bool {return p.id == presetId;});

		if(i != m_presets.end())
		{
			for(auto j = m_rows.begin(); j != m_rows.end(); j++)
			{
				EPGRow* r = *j;

				if(i == m_presets.end()) 
				{
					i = m_presets.begin();
				}

				Homesys::Preset& p = *i++;

				if(r->CurrentPreset->id != p.id)
				{
					r->CurrentPreset = &p;

					focus = true;
				}
			}
		}

		if(time < Start || time >= (CTime)Start + Duration)
		{
			Start = time;
		}

		if(focus)
		{
			std::vector<EPGRowButton*>& cols = m_rows[0]->m_cols;

			for(auto i = cols.begin(); i != cols.end(); i++)
			{
				Homesys::Program* p = (*i)->CurrentProgram;

				if(p->start <= time && time < p->stop)
				{
					(*i)->Focused = true;

					break;
				}
			}
		}
	}

	void EPGView::InitRows(ITunerPlayer* t)
	{
		FreeRows();

		m_tunerId = t->GetTunerId();

		m_presets.clear();

		std::list<Homesys::Preset> presets;

		if(g_env->svc.GetPresets(m_tunerId, true, presets))
		{
			m_presets.insert(m_presets.begin(), presets.begin(), presets.end());
		}

		m_header.Visible = !Compact;

		Vector4i r = m_rowstack.Rect;
		r.top = m_header.Visible ? m_header.Bottom : 0;
		m_rowstack.Move(r);

		r = Vector4i(0, 0, m_rowstack.Width, 0);

		for(int i = 0, j = std::min<int>(m_presets.size(), PresetRows); i < j; i++)
		{
			m_presets[i].enabled = i + 1;

			// r.bottom = i < j - 1 ? r.top + m_rowstack.Height / PresetRows : m_rowstack.Height;
			r.bottom = r.top + m_rowstack.Height / PresetRows;

			if(abs(r.bottom - m_rowstack.Height) < 3) 
			{
				r.bottom = m_rowstack.Height;
			}

			EPGRow* row = new EPGRow();

			row->Create(r.deflate(Vector4i(0, 1, 0, 1)), &m_rowstack);
			row->AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Right, Anchor::Top);
			row->CurrentPreset = &m_presets[i];
			row->Start.Chain(&m_header.Start);
			row->Duration.Chain(&m_header.Duration);
			row->RowButtonFocused.Add(&EPGView::OnColFocused, this);
			row->RowButtonClicked.Add0(&EPGView::OnColClicked, this);
			row->NavigateLeftEvent.Add0(&EPGView::OnNavigateLeft, this);
			row->NavigateRightEvent.Add0(&EPGView::OnNavigateRight, this);
			row->NavigateUpEvent.Add0(&EPGView::OnNavigateUp, this);
			row->NavigateDownEvent.Add0(&EPGView::OnNavigateDown, this);
			row->ColsChanged.Add0(&EPGView::OnColsChanged, this);
			row->m_channelbox.PaintForegroundEvent.Add(&EPGView::OnPaintChannelForeground, this, true);
/*
			if(Compact)
			{
				row->m_channelbox.Visible = false;
				
				CRect r = row->m_programbox.Rect;
				r.left = 0;
				row->m_programbox.MoveRect(r);
			}
*/
			m_rows.push_back(row);

			r.top = r.bottom;
		}

		for(auto i = m_rows.begin(); i != m_rows.end(); i++)
		{
			(*i)->Activate(1);
		}
	}

	void EPGView::FreeRows()
	{
		for(auto i = m_rows.begin(); i != m_rows.end(); i++)
		{
			EPGRow* r = *i;
			
			r->Start.Chain(NULL);
			r->Duration.Chain(NULL);
			r->Deactivate(-1);
			r->Parent = NULL;

			delete r;
		}

		m_rows.clear();
		m_presets.clear();
		m_tunerId = GUID_NULL;
	}

	bool EPGView::OnActivated(Control* c, int dir)
	{
		FreeRows();

		MakeVisible(g_env->ps.tuner.preset.id, CTime::GetCurrentTime());

		return true;
	}

	bool EPGView::OnDeactivated(Control* c, int dir)
	{
		m_dialog.Hide();

		FreeRows();

		return true;
	}

	bool EPGView::OnColsChanged(Control* c)
	{
		m_overlay.ToFront();

		return true;
	}

	bool EPGView::OnPaintOverlayForeground(Control* c, DeviceContext* dc)
	{
		CTime now = CTime::GetCurrentTime();
		CTime start = now - CTimeSpan((g_env->ps.stop - g_env->ps.start) / 10000000);
		CTime current = now - CTimeSpan((g_env->ps.stop - g_env->ps.GetCurrent()) / 10000000);

		for(auto i = m_rows.begin(); i != m_rows.end(); i++)
		{
			EPGRow* r = *i;

			if(r->CurrentPreset->id == g_env->ps.tuner.preset.id)
			{
				Vector4i cr = MapClientRect(&r->m_programbox);
				Vector4i r = cr;

				if(0) // Compact)
				{
					r.top = cr.top - 10;
					r.bottom = cr.top - 2;
				}
				else
				{
					r.top = cr.bottom - 10;
					r.bottom = cr.bottom - 5;
				}

				r.left = cr.left + cr.width() * (start - Start).GetTimeSpan() / Duration->GetTimeSpan();
				r.right = cr.left + cr.width() * (now - Start).GetTimeSpan() / Duration->GetTimeSpan();

				r.left = max(cr.left, r.left);
				r.right = min(cr.right, r.right);

				if(r.left < r.right)
				{
					dc->Draw(C2W(r), g_env->ps.tuner.recording ? Color::Red : Color::Green);
				}

				break;
			}
		}

		if(!m_rows.empty()) // && !Compact)
		{
			Vector4i tl = MapClientRect(&m_rows.front()->m_programbox);
			Vector4i br = MapClientRect(&m_rows.back()->m_programbox);

			Vector4i cr = tl.upl64(br.zwxy());
			Vector4i r = cr;

			CTimeSpan t[2] = {now - Start, current - Start};

			{
				r.left = cr.left + cr.width() * t[0].GetTimeSpan() / Duration->GetTimeSpan();
				r.right = cr.left + cr.width() * t[1].GetTimeSpan() / Duration->GetTimeSpan();

				// dc->Draw(C2W(r.rintersect(cr)), 0x20ffffff);
			}

			Vertex v[12];

			int n = 0;

			for(int i = 1; i < 2; i++)
			{
				r.left = cr.left + cr.width() * t[i].GetTimeSpan() / Duration->GetTimeSpan();
				r.right = r.left;

				Vector4i p = C2W(r);

				Vector4 p0(p.xyzy());
				Vector4 p1(p.xwzw());

				if(r.left > cr.left + 1 && r.left < cr.right)
				{
					v[n].p = p0 - Vector4(1, 0);
					v[n].c.x = 0x40ffffff;
					n++;

					v[n].p = p1 - Vector4(1, 0);
					v[n].c.x = 0x40ffffff;
					n++;
				}

				if(r.left > cr.left && r.left < cr.right)
				{
					v[n].p = p0;
					v[n].c.x = 0x80ffffff;
					n++;

					v[n].p = p1;
					v[n].c.x = 0x80ffffff;
					n++;
				}

				if(r.left > cr.left && r.left < cr.right - 1)
				{
					v[n].p = p0 + Vector4(1, 0);
					v[n].c.x = 0x40ffffff;
					n++;

					v[n].p = p1 + Vector4(1, 0);
					v[n].c.x = 0x40ffffff;
					n++;
				}
			}

			if(n > 0)
			{
				for(int i = 0; i < n; i++)
				{
					v[i].p.z = 0.5f;
					v[i].p.w = 1.0f;
					v[i].c.y = 0;
				}

				dc->DrawLineList(v, n / 2);
			}
		}

		return true;
	}

	bool EPGView::OnPaintChannelForeground(Control* c, DeviceContext* dc)
	{
		Vector4i r = c->WindowRect;
		
		EPGRow* row = (EPGRow*)(Control*)c->Parent;

		Homesys::Preset* p = row->CurrentPreset;

		if(p != NULL)
		{
			if(Texture* t = m_ctf.GetTexture(p->channelId, dc))
			{
				Vector4i r2 = r;

				r2.right = r2.left + r2.height() * 4 / 3;

				r.left = r2.right;
			
				r2 = r2.deflate(3);

				dc->Draw(r2, Color::White);

				r2 = DeviceContext::FitRect(t->GetSize(), r2.deflate(1));

				dc->Draw(t, r2);
			}

			std::wstring s = p->name;

			if(p->enabled > 0)
			{
				s = Util::Format(L"%d. ", p->enabled) + s;
			}

			c->ScrollNoFocus = row->FocusedPath;

			r = r.deflate(c->TextMargin);

			c->DrawScrollText(dc, s.c_str(), r);
		}

		return true;
	}

	bool EPGView::OnColFocused(Control* c, bool focused)
	{
		if(EPGRowButton* b = dynamic_cast<EPGRowButton*>(GetFocus()))
		{
			m_program = *b->CurrentProgram;

			CurrentProgram = &m_program;
		}

		return true;
	}

	bool EPGView::OnColClicked(Control* c)
	{
		if(EPGRowButton* b = dynamic_cast<EPGRowButton*>(c))
		{
			ProgramClicked(b, b->CurrentProgram);
		}

		return true;
	}

	bool EPGView::OnNavigateLeft(Control* c)
	{
		Start = (CTime)Start - CTimeSpan(0, 0, 30, 0);

		return true;
	}

	bool EPGView::OnNavigateRight(Control* c)
	{
		Start = (CTime)Start + CTimeSpan(0, 0, 30, 0);

		return true;
	}

	bool EPGView::OnNavigateUp(Control* c)
	{
		EPGRow* row = dynamic_cast<EPGRow*>(c);

		CTime position = row->Position;

		if(row == m_rows.front())
		{
			int index = FindPreset(row->CurrentPreset->id);

			if(index >= 0)
			{
				if(--index < 0)
				{
					index = m_presets.size() - 1;
				}

				for(int i = 0, j = m_rows.size(); i < j; i++)
				{
					m_rows[i]->CurrentPreset = &m_presets[index];

					if(++index == m_presets.size())
					{
						index = 0;
					}
				}
			}

			ASSERT(row == m_rows.front());
		}
		else
		{
			for(int i = 0, j = m_rows.size(); i < j; i++)
			{
				if(row == m_rows[i])
				{
					row = m_rows[(i + j - 1) % j];

					break;
				}
			}
		}

		CTime now = CTime::GetCurrentTime();

		if((CTime)Start <= now && now <= (CTime)Start + Duration)
		{
			position = now;
		}

		row->Position = position;

		return true;
	}

	bool EPGView::OnNavigateDown(Control* c)
	{
		EPGRow* row = dynamic_cast<EPGRow*>(c);

		CTime position = row->Position;

		if(row == m_rows.back())
		{
			int index = FindPreset(row->CurrentPreset->id);

			if(index >= 0)
			{
				if(++index >= m_presets.size())
				{
					index = 0;
				}

				for(int i = 0, j = m_rows.size(); i < j; i++)
				{
					m_rows[j - i - 1]->CurrentPreset = &m_presets[index];

					if(--index < 0)
					{
						index = m_presets.size() - 1;
					}
				}

				ASSERT(row == m_rows.back());
			}
		}
		else
		{
			for(int i = 0, j = m_rows.size(); i < j; i++)
			{
				if(row == m_rows[i])
				{
					row = m_rows[(i + 1) % j];

					break;
				}
			}
		}

		CTime now = CTime::GetCurrentTime();

		if((CTime)Start <= now && now <= (CTime)Start + Duration)
		{
			position = now;
		}

		row->Position = position;

		return true;
	}

	int EPGView::FindPreset(int id)
	{
		for(int i = 0, j = m_presets.size(); i < j; i++)
		{
			if(m_presets[i].id == id)
			{
				return i;
			}
		}

		return -1;
	}

	bool EPGView::OnProgramClicked(Control* c, Homesys::Program* p)
	{
		Homesys::Preset* preset = ((EPGRow*)(Control*)c->Parent->Parent)->CurrentPreset;

		m_dialog.m_switch.UserData = (UINT_PTR)preset->id;
		m_dialog.m_details.UserData = (UINT_PTR)p->channelId;
		m_dialog.m_record.UserData = (UINT_PTR)p->id;

		m_dialog.Show(this);

		return true;
	}
}
