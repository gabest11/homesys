#include "stdafx.h"
#include "MenuFileBrowser.h"
#include "ImageGrabber.h"
#include "client.h"

namespace DXUI
{
	MenuFileBrowser::MenuFileBrowser()
	{
		ActivatedEvent.Add(&MenuFileBrowser::OnActivated, this);
		DeactivatedEvent.Add(&MenuFileBrowser::OnDeactivated, this);
		PaintForegroundEvent.Add(&MenuFileBrowser::OnPaintForeground, this);
		BlueEvent.Add0([&] (Control* c) -> bool {TogglePlaylist(-1, false); return true;});

		m_location.ReturnKeyEvent.Add0(&MenuFileBrowser::OnReturnKey, this);
		m_filebrowser.m_navigator.Location.ChangedEvent.Add(&MenuFileBrowser::OnDirectory, this);
		m_filebrowser.m_navigator.ClickedEvent.Add(&MenuFileBrowser::OnFile, this);
		m_yesno.m_yes.ClickedEvent.Add0(&MenuFileBrowser::OnEditFile, this);
		m_yesno.m_no.ClickedEvent.Add0(&MenuFileBrowser::OnPlayFile, this);

		m_playlist_add.ClickedEvent.Add0(&MenuFileBrowser::OnSavePlaylist, this);
		m_playlist_list.KeyEvent.Add(&MenuFileBrowser::OnPlaylistKey, this);
		m_playlist_list.ItemCount.Get([&] (int& count) {count = m_playlist_items.size();});
		m_playlist_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			const PlaylistItem& item = m_playlist_items[p->index];

			p->text = item.name;

			if(item.dur > 0)
			{
				p->text += L" (" + FormatTime(item.dur) + L")";
			}

			return true;
		});

		m_thread.MessageEvent.Add(&MenuFileBrowser::OnThreadMessage, this);

		m_anim.Set(0, 0, 1000);
		m_animplaylist = false;
	}

	void MenuFileBrowser::TogglePlaylist(int i, bool prompt)
	{
		float curdst = m_anim[1];
		float dst = i == -1 ? 1.0f - curdst : i ? 1 : 0;

		CAutoLock csAutoLock(&m_thread);

		m_playlist_items.clear();
		m_total = 0;
		m_playlist_name.Text = L"";

		m_anim.Set(1.0f - dst, dst, 500);

		if(prompt)
		{
			m_anim.SetSegment(1);
		}

		m_animplaylist = true;
	}

	bool MenuFileBrowser::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			TogglePlaylist(0, true);

			CAutoLock csAutoLock(&m_thread);

			m_playlist_items.clear();
			m_total = 0;
		}

		m_thread.Create();

		return true;
	}

	bool MenuFileBrowser::OnDeactivated(Control* c, int dir)
	{
		m_thread.Post(WM_APP + 1);
		m_thread.Join(false);

		if(dir <= 0)
		{
			CAutoLock csAutoLock(&m_thread);

			m_playlist_items.clear();
			m_total = 0;
			m_playlist_total.Text = L"";
		}

		return true;
	}

	bool MenuFileBrowser::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(m_animplaylist)
		{
			float f = 1.0f - m_anim;

			Vector4i r1 = m_container.Rect;
			Vector4i r2 = m_filebrowser.Rect;
			Vector4i r3 = m_playlist.Rect;

			r2.right = r2.left + (int)((1.0f + f) / 2 * r1.width());
			r3.left = r2.right + 10;
			r3.right = r3.left + r1.width() / 2 - 10;

			m_filebrowser.Move(r2, false);
			m_playlist.Move(r3, false);

			m_filebrowser.AnchorRect = Vector4i(Anchor::Left, Anchor::Top, f == 0 ? Anchor::Percent : Anchor::Right, Anchor::Bottom);
			m_playlist.AnchorRect = Vector4i(Anchor::Percent, Anchor::Top, Anchor::Right, Anchor::Bottom);

			m_playlist.Visible = f < 1;

			if(m_anim.GetSegment() == 1)
			{
				m_animplaylist = false;
			}
		}

		return true;
	}

	bool MenuFileBrowser::OnReturnKey(Control* c)
	{
		std::wstring str = Util::TrimRight(c->Text, L"\\/");

		if(PathIsDirectory(str.c_str()) || str.size() == 2 && str[1] == ':')
		{
			m_filebrowser.m_navigator.Location = c->Text;
		}

		return true;
	}

	bool MenuFileBrowser::OnDirectory(Control* c, std::wstring path)
	{
		m_location.Text = path;

		return true;
	}

	bool MenuFileBrowser::OnFile(Control* c, const NavigatorItem* item)
	{
		std::wstring path = m_filebrowser.m_navigator.GetPath(item);

		std::wstring ext;

		if(LPWSTR s = PathFindExtension(path.c_str()))
		{
			ext = Util::MakeLower(s);
		}

		if(m_playlist.Visible)
		{
			CAutoLock csAutoLock(&m_thread);

			if(ext == L".pls")
			{
				// TODO

				if(FILE* f = _wfopen(path.c_str(), L"r"))
				{
					m_playlist_items.clear();
					m_total = 0;

					std::wstring title = path.substr(0, path.size() - ext.size());

					std::wstring::size_type i = title.find_last_of('\\');

					if(i++ != std::wstring::npos)
					{
						title = path.substr(i, title.size() - i);
					}

					m_playlist_name.Text = title;

					wchar_t* buff = new wchar_t[1024 + 1];

					while(!feof(f) && fgetws(buff, 1024, f))
					{
						std::list<std::wstring> sl;

						Util::Explode(std::wstring(buff), sl, '=', 2);

						if(sl.front().find(L"File") == 0)
						{
							std::wstring s = sl.back();

							std::wstring::size_type i = s.find_last_of('\\');

							if(i++ != std::wstring::npos)
							{
								s = s.substr(i, s.size() - i);
							}

							PlaylistItem pi;

							pi.path = sl.back();
							pi.name = s;
							pi.dur = -1;

							m_playlist_items.push_back(pi);
						}
					}

					delete [] buff;

					fclose(f);

					int last = m_playlist_items.size() - 1;

					m_playlist_list.Selected = last;
					m_playlist_list.MakeVisible(last);

					m_thread.Post(WM_APP);
				}

			}
			else
			{
				std::wstring s = path;

				std::wstring::size_type i = s.find_last_of('\\');

				if(i++ != std::wstring::npos)
				{
					s = s.substr(i, s.size() - i);
				}

				PlaylistItem pi;

				pi.path = path;
				pi.name = s;
				pi.dur = -1;

				m_playlist_items.push_back(pi);

				int last = m_playlist_items.size() - 1;

				m_playlist_list.Selected = last;
				m_playlist_list.MakeVisible(last);
				
				m_thread.Post(WM_APP);
			}
		}
		else
		{
			if(ext == L".dvr-ms")
			{
				m_lastpath = path;

				m_yesno.m_message.Text = Control::GetString("EDIT_OR_PLAY");
				m_yesno.m_no.Focused = true;
				m_yesno.Show();
			}
			else
			{
				g_env->PlayFileEvent(c, path);
			}
		}

		return true;
	}

	bool MenuFileBrowser::OnEditFile(Control* c)
	{
		g_env->EditFileEvent(c, m_lastpath);

		return true;
	}

	bool MenuFileBrowser::OnPlayFile(Control* c)
	{
		g_env->PlayFileEvent(c, m_lastpath);

		return true;
	}

	bool MenuFileBrowser::OnPlaylistKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			int i = m_playlist_list.Selected;

			if(i >= 0)
			{
				if(p.cmd.key == VK_DELETE)
				{
					CAutoLock csAutoLock(&m_thread);

					if(m_playlist_items[i].dur > 0)
					{
						m_total -= m_playlist_items[i].dur;
					}

					m_playlist_items.erase(m_playlist_items.begin() + i);

					if(m_playlist_items.empty())
					{
						m_playlist_name.Text = L"";
					}

					if(i < m_playlist_items.size() - 1)
					{
						m_playlist_list.Selected = i;
					}
					else
					{
						m_playlist_list.Selected = m_playlist_items.size() - 1;
					}

					return true;
				}
				else if(p.cmd.app == APPCOMMAND_MEDIA_CHANNEL_UP)
				{
					CAutoLock csAutoLock(&m_thread);

					if(i > 0)
					{
						PlaylistItem tmp = m_playlist_items[i - 1];
						m_playlist_items[i - 1] = m_playlist_items[i];
						m_playlist_items[i] = tmp;

						m_playlist_list.Selected = i - 1;

						return true;
					}
				}
				else if(p.cmd.app == APPCOMMAND_MEDIA_CHANNEL_DOWN)
				{
					CAutoLock csAutoLock(&m_thread);

					if(i < m_playlist_items.size() - 1)
					{
						PlaylistItem tmp = m_playlist_items[i + 1];
						m_playlist_items[i + 1] = m_playlist_items[i];
						m_playlist_items[i] = tmp;

						m_playlist_list.Selected = i + 1;

						return true;
					}

					return true;
				}
			}
		}

		return false;
	}

	bool MenuFileBrowser::OnSavePlaylist(Control* c)
	{
		CAutoLock csAutoLock(&m_thread);

		std::wstring name = m_playlist_name.Text;
		
		if(!name.empty())
		{
			std::wstring path = m_filebrowser.m_navigator.Location;

			path += L"\\" + name + L".pls";

			if(FILE* fp = _wfopen(path.c_str(), L"w"))
			{
				for(size_t i = 0; i < m_playlist_items.size(); i++)
				{
					const PlaylistItem& pi = m_playlist_items[i];

					fwprintf(fp, L"File%d=%s\n", i + 1, pi.path.c_str());
				}

				fclose(fp);

				m_filebrowser.m_navigator.Location = m_filebrowser.m_navigator.Location;
			}
		}

		return true;
	}

	bool MenuFileBrowser::OnThreadMessage(Control* c, const MSG& msg)
	{
		if(msg.message == WM_APP)
		{
			bool again = false;

			for(int i = 0; !m_thread.Peek(); i++)
			{
				{
					CAutoLock csAutoLock(&m_thread);

					if(i >= m_playlist_items.size())
					{
						if(again) {i = 0; continue;}
						else break;
					}

					if(m_playlist_items[i].dur < 0)
					{
						again = true;

						ImageGrabber ig;
						
						REFERENCE_TIME dur = 0;

						if(ig.GetInfo(m_playlist_items[i].path.c_str(), 0, NULL, NULL, &dur))
						{
							if(dur < 0) dur = 0;

							m_playlist_items[i].dur = dur;
							m_total += dur;
						}
						else
						{
							m_playlist_items[i].dur = 0;
						}
					}
				}

				{
					CAutoLock cAutoLock(&m_lock);

					m_playlist_total.Text = FormatTime(m_total);
				}
			}

			// TODO

			return true;
		}

		return false;
	}
}