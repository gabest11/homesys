#include "stdafx.h"
#include "MenuSubtitleSettings.h"
#include "client.h"
#include "../../DirectShow/TextSubProvider.h"

namespace DXUI
{
	MenuSubtitleSettings::MenuSubtitleSettings() 
	{
		ActivatedEvent.Add(&MenuSubtitleSettings::OnActivated, this);
		DeactivatedEvent.Add(&MenuSubtitleSettings::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_next.ClickedEvent.Add0(&MenuSubtitleSettings::OnNext, this);

		m_face_select.Selected.ChangedEvent.Add(&MenuSubtitleSettings::OnFaceSelected, this);
		m_face_select.m_list.ItemCount.Get([&] (int& count) {count = m_face.size();});
		m_face_select.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_face[p->index]; return true;});
		m_face_select.m_list.PaintItemEvent.Add(&MenuSubtitleSettings::OnPaintFace, this, true);

		m_size_select.m_list.ItemCount.Get([&] (int& count) {count = m_size.size();});
		m_size_select.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = Util::Format(L"%d pt", m_size[p->index]); return true;});

		m_outline_select.m_list.ItemCount.Get([&] (int& count) {count = m_outline.size();});
		m_outline_select.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = Util::Format(L"%.1f pt", (float)m_outline[p->index] / 2); return true;});

		m_shadow_select.m_list.ItemCount.Get([&] (int& count) {count = m_shadow.size();});
		m_shadow_select.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = Util::Format(L"%.1f pt", (float)m_shadow[p->index] / 2); return true;});

		m_margin_bottom.m_list.ItemCount.Get([&] (int& count) {count = m_position.size();});
		m_margin_bottom.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = Util::Format(L"%d pt", m_position[p->index]); return true;});
	}

	static int CALLBACK EnumFontsProc(const LOGFONT* lf, const TEXTMETRIC* tm, DWORD type, LPARAM lParam)
	{
		std::vector<std::wstring>* l = (std::vector<std::wstring>*)lParam;

		if(type & TRUETYPE_FONTTYPE)
		{
			if(lf->lfFaceName[0] != '@')
			{
				l->push_back(lf->lfFaceName);
			}
		}

		return TRUE;
	}

	bool MenuSubtitleSettings::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			Util::Config cfg(L"Homesys", L"Subtitle");

			{
				m_face.clear();

				HDC hDC = GetDC(NULL);

				EnumFonts(hDC, NULL, EnumFontsProc, (LPARAM)&m_face);

				ReleaseDC(NULL, hDC);

				std::sort(m_face.begin(), m_face.end());

				std::wstring face = cfg.GetString(L"Face", L"Arial");

				auto i = std::find(m_face.begin(), m_face.end(), face);

				if(i == m_face.end()) 
				{
					i = std::find(m_face.begin(), m_face.end(), L"Arial");
				}

				if(i != m_face.end()) 
				{
					m_face_select.Selected = i - m_face.begin();
				}
			}

			{
				int size = std::min<int>(std::max<int>(cfg.GetInt(L"Size", 26), 18), 40);

				for(int i = 18; i <= 40; i++)
				{
					m_size.push_back(i);
				}

				m_size_select.Selected = size - 18;
			}

			m_bold.Checked = cfg.GetInt(L"Bold", 1) != 0;

			{
				int outline = std::min<int>(std::max<int>(cfg.GetInt(L"Outline", 3), 0), 8);

				for(int i = 0; i <= 8; i++)
				{
					m_outline.push_back(i);
				}

				m_outline_select.Selected = outline;
			}

			{
				int shadow = std::min<int>(std::max<int>(cfg.GetInt(L"Shadow", 3), 0), 8);

				for(int i = 0; i <= 8; i++)
				{
					m_shadow.push_back(i);
				}

				m_shadow_select.Selected = shadow;
			}

			{
				int position = std::min<int>(std::max<int>(cfg.GetInt(L"Position", 40), 0), 80);

				for(int i = 0; i <= 80; i++)
				{
					m_position.push_back(i);
				}

				m_margin_bottom.Selected = position;
			}

			m_bbox.Checked = cfg.GetInt(L"BBox", 0) != 0;
		}

		return true;
	}

	bool MenuSubtitleSettings::OnDeactivated(Control* c, int dir)
	{
		if(dir <= 0)
		{
			m_face.clear();
			m_size.clear();
			m_outline.clear();
			m_shadow.clear();
			m_position.clear();
		}

		return true;
	}

	bool MenuSubtitleSettings::OnNext(Control* c)
	{
		Util::Config cfg(L"Homesys", L"Subtitle");

		{
			int index = m_face_select.Selected;

			if(index >= 0 && index < m_face.size())
			{
				cfg.SetString(L"Face", m_face[index].c_str());
			}
		}

		cfg.SetInt(L"Size", m_size_select.Selected + 18);
		cfg.SetInt(L"Bold", m_bold.Checked ? 1 : 0);
		cfg.SetInt(L"Outline", (int)m_outline_select.Selected);
		cfg.SetInt(L"Shadow", (int)m_shadow_select.Selected);
		cfg.SetInt(L"Position", (int)m_margin_bottom.Selected);
		cfg.SetInt(L"BBox", m_bbox.Checked ? 1 : 0);

		DoneEvent(this);
		
		return true;
	}

	bool MenuSubtitleSettings::OnFaceSelected(Control* c, int index)
	{
		if(index >= 0 && index < m_face.size())
		{
			m_face_select.TextFace = m_face[index];
		}

		return true;
	}

	bool MenuSubtitleSettings::OnPaintFace(Control* c, PaintItemParam* p)
	{
		p->dc->TextFace = m_face[p->index];

		p->dc->Draw(p->text.c_str(), p->rect);

		return true;
	}
}