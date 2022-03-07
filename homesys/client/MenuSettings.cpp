#include "stdafx.h"
#include "MenuSettings.h"
#include "client.h"
#include "region.h"
#include "../../util/Config.h"

namespace DXUI
{
	MenuSettings::MenuSettings() 
	{
		m_progressbar.Position.Chain(&m_downloader.Progress);
		m_progressbar.Stop.Chain(&m_downloader.Progress);
		m_progressbar.Text.Chain(&m_downloader.ProgressText);

		ActivatedEvent.Add(&MenuSettings::OnActivated, this);
		DeactivatedEvent.Add(&MenuSettings::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_download_path.TextInputEvent.Add0(&MenuSettings::OnVerifyPath, this);
		m_recording_path.TextInputEvent.Add0(&MenuSettings::OnVerifyPath, this);
		m_recording_temp_path.TextInputEvent.Add0(&MenuSettings::OnVerifyPath, this);
		m_next.ClickedEvent.Add0(&MenuSettings::OnNext, this);
		m_update.ClickedEvent.Add0(&MenuSettings::OnUpdate, this);
		m_downloader.DoneEvent.Add(&MenuSettings::OnDownloaderDone, this);

		m_recording_temp_duration.m_list.ItemCount.Get([&] (int& count) {count = m_durations.size();});
		m_recording_temp_duration.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			p->text = Util::Format(L"%d ", m_durations[p->index]) + Control::GetString("MINUTES");

			return true;
		});

		m_exts.push_back(L".dsm");
		m_exts.push_back(L".ts");

		m_recording_format.m_list.ItemCount.Get([&] (int& count) {count = m_exts.size();});
		m_recording_format.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			p->text = m_exts[p->index];
		
			return true;
		});

		m_language.m_list.ItemCount.Get([&] (int& count) {count = m_languages.size();});
		m_language.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			std::wstring s;

			switch(m_languages[p->index])
			{
			case 'eng': s = L"English"; break;
			case 'deu': s = L"Deutsch"; break;
			case 'hun': s = L"Magyar"; break;
			}

			p->text = s;

			return true;
		});

		m_languages.push_back('eng');
		m_languages.push_back('deu');
		m_languages.push_back('hun');

		m_region.m_list.ItemCount.Get([&] (int& count) {count = g_region_count;});
		m_region.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = g_region[p->index].name; return true;});

		m_skin.m_list.ItemCount.Get([&] (int& count) {count = m_skins.size();});
		m_skin.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			std::string s = "SETTINGS_SKIN_" + Util::UTF16To8(m_skins[p->index].c_str());

			p->text = Control::GetString(s.c_str());

			return true;
		});

		m_skins.push_back(L"DARK");
		m_skins.push_back(L"ORANGE");
	}

	bool MenuSettings::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_recording_temp_path.Text = L"";
			m_recording_temp_duration.Selected = -1;

			m_durations.clear();

			Homesys::MachineReg m;

			g_env->svc.GetMachine(m);

			m_username.Text = m.username;
			m_password.Text = m.password;
			m_download_path.Text = m.downloadPath;
			m_recording_path.Text = m.recordingPath;
			m_recording_temp_path.Text = m.timeshiftPath;

			m_recording_format.Selected = -1;

			for(int i = 0; i < m_exts.size(); i++)
			{
				if(m_exts[i] == m.recordingFormat)
				{
					m_recording_format.Selected = i;
				
					break;
				}
			}

			int dur = (int)m.timeshiftDur.GetTotalMinutes();

			for(int i = 20, j = 0; i <= 300; i += 10, j++)
			{
				m_durations.push_back(i);

				if(i == dur) 
				{
					m_recording_temp_duration.Selected = j;
				}
			}

			if(m_recording_temp_duration.Selected == -1)
			{
				m_durations.push_back(dur);

				m_recording_temp_duration.Selected = m_durations.size() - 1;
			}

			std::wstring lang;

			if(g_env->svc.GetLanguage(lang))
			{
				if(lang == L"eng") m_language.Selected = 0;
				else if(lang == L"deu") m_language.Selected = 1;
				else if(lang == L"hun") m_language.Selected = 2;
			}

			m_region.Selected = -1;

			std::wstring region;

			if(g_env->svc.GetRegion(region))
			{
				if(region == L"unk")
				{
					// TODO: set geoid
				}
				else
				{
					for(int i = 0; i < g_region_count; i++)
					{
						if(region == g_region[i].id)
						{
							m_region.Selected = i;
						}
					}
				}
			}

			{
				std::wstring s = Util::Config(L"Homesys", L"Settings").GetString(L"Skin", DEFAULT_SKIN);

				int index = -1;

				for(int i = 0; i < m_skins.size(); i++)
				{
					if(m_skins[i] == s)
					{
						index = i;

						break;
					}
				}

				m_skin.Selected = index;
			}

			g_env->svc.GetMostRecentVersion(m_appversion);

			union {UINT64 ui64; struct {WORD ui8[4];};} version;

			g_env->svc.GetInstalledVersion(version.ui64);

#ifdef DEBUG
			version.ui64 = 0;
#endif

			m_version_info.Text = Util::Format(L"%d.%d.%d", version.ui8[3], version.ui8[2], version.ui8[0]);

			m_update.Enabled = m_appversion.version > version.ui64;
		}

		return true;
	}

	bool MenuSettings::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool MenuSettings::OnVerifyPath(Control* c)
	{
		// TODO

		return true;
	}

	bool MenuSettings::OnNext(Control* c)
	{
		Homesys::MachineReg m;

		if(!g_env->svc.GetMachine(m))
		{
			return false;
		}

		bool valid = true;

		if(!g_env->svc.SetTimeshiftPath(m_recording_temp_path.Text))
		{
			valid = false;
		}

		if(!g_env->svc.SetTimeshiftDuration(CTimeSpan(m_durations[m_recording_temp_duration.Selected] * 60)))
		{
			valid = false;
		}

		m_download_path.TextColor = Color::Text3;

		if(!g_env->svc.SetDownloadPath(m_download_path.Text))
		{
			m_download_path.TextColor = Color::Red;

			valid = false;
		}

		m_recording_path.TextColor = Color::Text3;

		if(!g_env->svc.SetRecordingPath(m_recording_path.Text))
		{
			m_recording_path.TextColor = Color::Red;

			valid = false;
		}

		{
			int i = m_recording_format.Selected;

			if(i >= 0 && i < m_exts.size())
			{
				if(!g_env->svc.SetRecordingFormat(m_exts[i]))
				{
					valid = false;
				}
			}
		}

		std::wstring username = m_username.Text;
		std::wstring password = m_password.Text;

		if(!username.empty() || !password.empty())
		{
			if(!g_env->svc.SetUsername(username) || !g_env->svc.SetPassword(password))
			{
				valid = false;
			}
		}

		if(m_language.Selected >= 0)
		{
			std::wstring s;

			switch(m_languages[m_language.Selected])
			{
			case 'eng': s = L"eng"; break;
			case 'deu': s = L"deu"; break;
			case 'hun': s = L"hun"; break;
			}

			g_env->svc.SetLanguage(s.c_str());

			// TODO: Control::SetLanguage(s.c_str());

			Util::Config(L"Homesys", L"Settings").SetString(L"UILang", s.c_str());
		}

		if(m_region.Selected >= 0)
		{
			g_env->svc.SetRegion(g_region[m_region.Selected].id);
		}
		else
		{
			valid = false;
		}

		if(m_skin.Selected >= 0)
		{
			Util::Config(L"Homesys", L"Settings").SetString(L"Skin", m_skins[m_skin.Selected].c_str());
		}

		if(valid) 
		{
			DoneEvent(this);

			return true;
		}

		return false;
	}

	bool MenuSettings::OnUpdate(Control* c)
	{
		if(!m_update.Captured)
		{
			if(m_appversion.url.find(L".msi") != std::wstring::npos
			|| m_appversion.url.find(L".zip") != std::wstring::npos)
			{
				m_update.Captured = true;
				m_update.Text = Control::GetString("CANCEL");
				m_progressbar.Visible = true;
				m_downloader.Url = m_appversion.url;
			}
			else
			{
				ShellExecute(NULL, L"open", m_appversion.url.c_str(), NULL, NULL, SW_SHOW);
			}
		}
		else
		{
			m_update.Captured = false;
			m_update.Text = Control::GetString("REFRESH");
			m_progressbar.Visible = false;
			m_downloader.Url = L"";
		}

		return true;
	}

	static void Run(const std::wstring& path)
	{
		WCHAR* buff = new WCHAR[path.size() + 1];
		
		wcscpy(buff, path.c_str());
		
		STARTUPINFO si;
		
		memset(&si, 0, sizeof(si));
		
		si.cb = sizeof(si);
		
		PROCESS_INFORMATION pi;

		memset(&pi, 0, sizeof(pi));
		
		if(!CreateProcess(NULL, buff, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			if(GetLastError() == 740) // elevation required
			{
				ShellExecute(NULL, L"open", buff, NULL, NULL, SW_SHOW);
			}
		}
		
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		delete [] buff;
	}

	bool MenuSettings::OnDownloaderDone(Control* c, int status)
	{
		if(status == Downloader::Success)
		{
			std::wstring url = m_downloader.Url;
			std::wstring src = m_downloader.Path;

			if(url.find(L".msi") != std::wstring::npos)
			{
				Run(Util::Format(L"msiexec.exe /i \"%s\"", src.c_str()).c_str());

				ExitEvent(this);
			}
			else if(url.find(L".zip") != std::wstring::npos)
			{
				std::wstring dst = Util::CombinePath(Util::RemoveFileSpec(src.c_str()).c_str(), L"setup");

				Homesys::ZipDecompressor zip;

				if(zip.Decompress(src.c_str(), dst.c_str()))
				{
					std::wstring dir = dst;

					dst = Util::CombinePath(dst.c_str(), L"setup.bat");

					if(FILE* fp = _wfopen(dst.c_str(), L"w"))
					{
						std::wstring str;

						CRegKey key;

						if(ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\Homesys", KEY_READ))
						{
							ULONG len = 128;
							WCHAR buff[128];

							if(ERROR_SUCCESS == key.QueryStringValue(L"ProductCode", buff, &len))
							{
								buff[len] = 0;
							}
							else
							{
								buff[0] = 0;
							}

							str = buff;
						}

						fwprintf(fp, 
							L"msiexec /x %s /passive\n"
							L"\"%s\\setup.exe\"\n", 
							str.c_str(), dir.c_str());

						fclose(fp);
					}
					
					Run(dst);

					ExitEvent(this);
				}
			}
		}

		return true;
	}
}