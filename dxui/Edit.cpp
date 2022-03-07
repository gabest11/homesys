#include "stdafx.h"
#include "Edit.h"
#include "Localizer.h"
#include "../util/String.h"

namespace DXUI
{
	Event<> Edit::EditTextEvent;

	Edit::Edit()
		: m_anim(true)
		, m_start(0)
	{
		CanFocus.Get([&] (int& value) {value = !ReadOnly ? 1 : 0;});
		DisplayedText.Get(&Edit::GetDisplayedText, this);
		TextAsInt.Get([&] (int& value) {value = _wtoi(Text->c_str());});
		TextAsInt.Set([&] (int value) {Text = Util::Format(L"%d", value);});

		// Text.ChangedEvent.Add([&] (Control* c, const std::wstring& s) -> bool {if(CursorPos > s.size()) CursorPos = s.size(); return true;});
		CursorPos.Get([&] (int& value) {if(value > Text->size()) value = Text->size();});
		CursorPos.ChangedEvent.Add(&Edit::OnCursorPosChanged, this);
		PaintForegroundEvent.Add(&Edit::OnPaintForeground, this, true);
		KeyEvent.Add(&Edit::OnKey, this);
		MouseEvent.Add(&Edit::OnMouse, this);
		FocusSetEvent.Add0(&Edit::OnFocusSet, this);
		RedEvent.Add0([&] (Control* c) -> bool {EditTextEvent(this); return true;});

		BackgroundImage = L"EditBackground.png";
		FocusedImage = L"EditFocused.png";
		BackgroundMargin = Vector4i(4);
		TextMargin = Vector4i(6, 4, 6, 4);
		TextColor = Color::Text3;
		TextHeight.Get([&] (int& value) {value = Height - 10;});
		TextAlign = Align::Left | Align::Middle;
		RedInfo = Control::GetString("ON_SCREEN_KEYBOARD");
		HideCursor = true;

		m_anim.Set(0, 1, 1200);
	}

	void Edit::GetDisplayedText(std::wstring& value)
	{
		value = Text;

		if(Password && !value.empty())
		{
			for(int i = 0, j = value.size(); i < j; i++)
			{
				value[i] = '*';
			}
		}
		else
		{
			if(value.empty() && !Focused)
			{
				value = BalloonText;
			}
		}
	}

	void Edit::Paste()
	{
		std::wstring str;

		if(IsClipboardFormatAvailable(CF_UNICODETEXT))
		{
			if(OpenClipboard(NULL))
			{
				if(HANDLE hData = GetClipboardData(CF_UNICODETEXT)) 
				{ 
					if(LPCWSTR s = (LPCWSTR)GlobalLock(hData))
					{
						str = s;

						GlobalUnlock(hData); 
					} 
				} 

				CloseClipboard(); 
			}
		}

		if(!str.empty())
		{
			Util::Replace(str, L"\n", L"");
			Util::Replace(str, L"\r", L"");
			Util::Replace(str, L"\t", L" ");

			std::wstring text = Text;

			int pos = CursorPos;
			Text = text.substr(0, pos) + str + text.substr(pos);
			CursorPos = pos + str.size();
			TextInputEvent(this);
		}
	}

	bool Edit::OnCursorPosChanged(Control* c, int pos)
	{
		DeviceContext* dc = DC;

		ASSERT(dc);

		CAutoLock cAutoLock(dc);

		SetTextProperties(dc);

		Vector4i r = ClientRect->deflate(TextMargin);

		std::wstring text = Text;

		if(pos > 0 && pos == m_start && pos == text.size())
		{
			m_start = text.size();

			while(1)
			{
				TextEntry* te = dc->Draw(DisplayedText->substr(m_start, pos - m_start).c_str(), r, true);

				if(te != NULL && te->bbox.right < Width && m_start > 0)
				{
					m_start--;
				}
				else
				{
					break;
				}
			}
		}
		
		if(pos < m_start)
		{
			m_start = pos;
		}
		else
		{
			while(1)
			{
				TextEntry* te = dc->Draw(DisplayedText->substr(m_start, pos - m_start).c_str(), r, true);

				if(te != NULL && te->bbox.right > Width && m_start < pos)
				{
					m_start++;
				}
				else
				{
					break;
				}
			}
		}

		m_anim.SetSegment(0);

		return true;
	}

	bool Edit::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i r = WindowRect->deflate(TextMargin);

		dc->Draw(DisplayedText->substr(m_start).c_str(), r);

		if(Text->empty() && !DisplayedText->empty())
		{
			return true;
		}

		if((!HideCursor || Focused) && m_anim < 0.5f)
		{
			Vector4i bbox = Vector4i::zero();

			TextEntry* te = dc->Draw(DisplayedText->substr(m_start, CursorPos - m_start).c_str(), r, true);

			if(te != NULL)
			{
				bbox = te->bbox;
			}
			else
			{
				bbox.bottom = Height;
			}

			bbox += r.xyxy();
			
			if(r.left <= bbox.right && bbox.right < r.right)
			{
				r.left = bbox.right;
				r.right = bbox.right + 1;

				dc->Draw(r, TextColor);
			}
		}

		return true;
	}

	bool Edit::OnKey(Control* c, const KeyEventParam& p)
	{
		if(ReadOnly)
		{
			return false;
		}

		std::wstring text = Text;

		if(p.cmd.chr >= 0x20)
		{
			int pos = CursorPos;
			Text = text.substr(0, pos) + (wchar_t)p.cmd.chr + text.substr(pos);
			CursorPos = pos + 1;
			TextInputEvent(this);

			return true;
		}

		if(p.cmd.key == 'C' && p.mods == KeyEventParam::Ctrl)
		{
			std::wstring str = Text; 

			if(OpenClipboard(NULL))
			{
				EmptyClipboard();
				
				HGLOBAL hData = GlobalAlloc(GMEM_DDESHARE, (str.size() + 1) * sizeof(WCHAR));
				wcscpy((LPWSTR)GlobalLock(hData), str.c_str());
				GlobalUnlock(hData);
				
				SetClipboardData(CF_UNICODETEXT, hData);

				CloseClipboard();
			}

			return true;
		}

		if(p.cmd.key == 'V' && p.mods == KeyEventParam::Ctrl
		|| p.cmd.key == VK_INSERT && p.mods == KeyEventParam::Shift)
		{
			Paste();

			return true;
		}

		if(p.mods == 0)
		{
			if(p.cmd.key == VK_BACK)
			{
				if(!text.empty() && CursorPos > 0)
				{
					int pos = CursorPos;
					Text = text.substr(0, pos - 1) + text.substr(pos);
					CursorPos = pos - 1;
					TextInputEvent(this);
				}

				return true;
			}

			if(p.cmd.key == VK_DELETE)
			{
				if(!text.empty() && CursorPos < text.size())
				{
					int pos = CursorPos;
					Text = text.substr(0, pos) + text.substr(pos + 1);
					CursorPos = pos;
					TextInputEvent(this);
				}

				return true;
			}

			if(p.cmd.key == VK_LEFT)
			{
				if(CursorPos > 0)
				{
					CursorPos = CursorPos - 1;
				}
				else
				{
					return false;
				}
				
				return true;
			}

			if(p.cmd.key == VK_RIGHT)
			{
				if(CursorPos < text.size())
				{
					CursorPos = CursorPos + 1;
				}
				else
				{
					return false;
				}
				
				return true;
			}

			if(p.cmd.key == VK_HOME)
			{
				CursorPos = 0;
				
				return true;
			}

			if(p.cmd.key == VK_END)
			{
				CursorPos = text.size();
				
				return true;
			}

			if(p.cmd.key == VK_RETURN && WantReturn)
			{
				ReturnKeyEvent(this);

				return true;
			}
		}

		return false;
	}

	bool Edit::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Down && Hovering)
		{
			DeviceContext* dc = DC;

			CAutoLock cAutoLock(dc);

			std::wstring text = Text;

			Vector4i r = ClientRect->deflate(TextMargin);

			int i = m_start;

			for(; i <= text.size(); i++)
			{
				SetTextProperties(dc);

				TextEntry* te = dc->Draw(DisplayedText->substr(m_start, i - m_start).c_str(), r, true);

				if(te != NULL && p.point.x < te->bbox.right) 
				{
					break;
				}
			}
			
			CursorPos = i - 1;

			return true;
		}
		
		if(p.cmd == MouseEventParam::RDown && Hovering)
		{
			Paste();

			return true;
		}

		if(p.cmd == MouseEventParam::Dbl && Hovering)
		{
			EditTextEvent(this);

			return true;
		}

		return false;
	}

	bool Edit::OnFocusSet(Control* c)
	{
		CursorPos = Text->size();

		return true;
	}
}