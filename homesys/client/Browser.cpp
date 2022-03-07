#include "stdafx.h"
#include "Browser.h"
#include "ImageGrabber.h"
#include "client.h"

namespace DXUI
{
	Browser::Browser() 
		: m_width(256)
		, m_height(256)
		, m_stretch(true)
		, m_dc(NULL)
	{
		Control::DC.ChangedEvent.Add(&Browser::OnDeviceContextChanged, this);
		ActivatedEvent.Add(&Browser::OnActivated, this);
		DeactivatedEvent.Add(&Browser::OnDeactivated, this);
		KeyEvent.Add(&Browser::OnKey, this);
		RedEvent.Add0(&Browser::OnRed, this);
		GreenEvent.Add0(&Browser::OnGreen, this);
		YellowEvent.Add0(&Browser::OnYellow, this);
		m_thread.MessageEvent.Add(&Browser::OnThreadMessage, this);
		m_navigator.Location.ChangedEvent.Add(&Browser::OnLocationChanged, this);

		RedInfo = Control::GetString("DIR_RECORDINGS");
		GreenInfo = Control::GetString("DIR_DOWNLOADS");
		YellowInfo = Control::GetString("DIR_DOCUMENTS");
	}

	void Browser::Refresh()
	{
		m_navigator.Location = m_navigator.Location;
	}

	void Browser::Lock()
	{
		m_thread.Lock();

		m_visible.clear();
	}
	
	void Browser::Unlock()
	{
		int index = 0;

		for(auto i = m_items.begin(); i != m_items.end(); i++)
		{
			if(m_visible.find(index++) == m_visible.end())
			{
				delete i->tex;
				
				i->tex = NULL;
			}
		}

		m_thread.Post(WM_APP);

		m_thread.Unlock();
	}

	void Browser::PaintItem(int index, const Vector4i& r, DeviceContext* dc)
	{
		const Item& item = m_items[index];

		Texture* t = NULL;

		// if(!item.ni.isdir)
		{
			if(!item.bogus && item.tex)
			{
				t = item.tex;
			}
			else if(item.loading && !item.ni.isdir)
			{
				t = dc->GetTexture(L"jolly.png"); // TODO
			}
		}

		if(t == NULL)
		{
			if(t == NULL)
			{
				t = dc->GetTexture(L"archive.png"); // TODO
			}
		}

		if(t != NULL)
		{
			dc->Draw(t, r);
		}

		if(item.ni.islnk)
		{
			t = dc->GetTexture(L"link.png");

			if(t != NULL)
			{
				Vector4i r2 = r;
				
				int y = (r.top + r.bottom) / 2;
				
				r2.left = r.right - 20;
				r2.right = r.right;
				r2.top = y - 10;
				r2.bottom = y + 10;

				dc->Draw(t, r2, 0.8f);
			}
		}

		m_visible.insert(index);
	}

	bool Browser::OnLocationChanged(Control* c, std::wstring path)
	{
		CAutoLock cAutoLock(&m_thread);

		for(auto i = m_items.begin(); i != m_items.end(); i++)
		{
			delete i->tex;
		}

		m_items.clear();
		m_visible.clear();

		m_items.reserve(m_navigator.size());

		for(auto i = m_navigator.begin(); i != m_navigator.end(); i++)
		{
			Item item;
			item.ni = *i;
			m_items.push_back(item);
		}

		return true;
	}

	bool Browser::OnDeviceContextChanged(Control* c, DeviceContext* dc)
	{
		CAutoLock cAutoLock(&m_thread);

		m_dc = dc;

		for(auto i = m_items.begin(); i != m_items.end(); i++)
		{
			delete i->tex;

			i->tex = NULL;
		}

		return true;
	}

	bool Browser::OnActivated(Control* c, int dir)
	{
		m_thread.Create();

		if(dir >= 0)
		{
			m_navigator.Location = m_navigator.Location;
		}

		return true;
	}

	bool Browser::OnDeactivated(Control* c, int dir)
	{
		m_thread.Join();

		if(dir <= 0)
		{
			for(auto i = m_items.begin(); i != m_items.end(); i++)
			{
				delete i->tex;
			}

			m_items.clear();
			m_visible.clear();
			m_navigator.clear();
		}

		return true;
	}

	bool Browser::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_F5)
			{
				Refresh();

				return true;
			}
		}

		return false;
	}

	bool Browser::OnRed(Control* c)
	{
		std::wstring path;

		if(g_env->svc.GetRecordingPath(path))
		{
			m_navigator.Location = path;
		}

		return false;
	}

	bool Browser::OnGreen(Control* c)
	{
		std::wstring path;

		if(g_env->svc.GetDownloadPath(path))
		{
			m_navigator.Location = path;
		}

		return true;
	}

	bool Browser::OnYellow(Control* c)
	{
		TCHAR path[MAX_PATH];
		
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, path))) 
		{
			m_navigator.Location = path;
		}

		return true;
	}

	bool Browser::OnThreadMessage(Control* c, const MSG& msg)
	{
		if(msg.message != WM_APP) return false;

		std::set<int> visible;

		{
			CAutoLock cAutoLock(&m_thread);

			visible = m_visible;
		}

		for(auto i = visible.begin(); i != visible.end(); i++)
		{
			int index = *i;

			std::wstring path;

			{
				CAutoLock cAutoLock(&m_thread);

				if(!m_thread) return false;

				if(index >= m_items.size()) 
				{
					return false;
				}

				Item& item = m_items[index];

				if(/*!item.ni.isdir && */ !item.tex && !item.bogus)
				{
					item.loading = true;

					path = m_navigator.GetPath(&item.ni);
				}
			}

			if(path.empty()) 
			{
				continue;
			}

			std::wstring thumb;
			std::wstring mediainfo;

			{
				BY_HANDLE_FILE_INFORMATION info;

				memset(&info, 0, sizeof(info));

				HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

				if(hFile != INVALID_HANDLE_VALUE)
				{
					GetFileInformationByHandle(hFile, &info);

					CloseHandle(hFile);
				}

				std::wstring tohash = path + Util::Format(
					L":%08x:%08x:%08x:%08x", 
					info.ftCreationTime.dwHighDateTime, 
					info.ftCreationTime.dwLowDateTime, 
					info.nFileSizeHigh, 
					info.nFileSizeLow);

				std::wstring hash = Homesys::Hash::AsMD5(tohash.c_str());

				std::wstring dir = (std::wstring)g_env->CachePath + L"\\" + hash.substr(0, 2);

				CreateDirectory(dir.c_str(), NULL);

				thumb = dir + L"\\" + hash;
				mediainfo = thumb + _T(":mediainfo");
			}

			ImageGrabber::Bitmap bm;
			std::map<std::wstring, std::wstring> tags;
			REFERENCE_TIME dur = 0;
			bool hasimg = false;

			Util::ImageDecompressor id;

			if(id.Open(thumb.c_str()))
			{
				bm.bits = new DWORD[id.m_width * id.m_height];
				bm.width = id.m_width;
				bm.height = id.m_height;

				BYTE* src = (BYTE*)id.m_pixels;
				BYTE* dst = (BYTE*)bm.bits;

				for(int y = 0; y < bm.height; y++, src += id.m_pitch, dst += bm.width * 4)
				{
					memcpy(dst, src, bm.width * 4);
				}

				if(FILE* fp = _wfopen(mediainfo.c_str(), L"rt, ccs=UNICODE"))
				{
					wchar_t* buff = new TCHAR[65536];

					while(!feof(fp))
					{
						buff[0] = 0;

						_fgetts(buff, 65536 - 1, fp);

						std::wstring str(buff);
						std::list<std::wstring> sl;

						Util::Explode(str, sl, '=', 2);

						if(sl.size() != 2) break;

						tags[sl.front()] = sl.back();

						if(sl.front() == L"__DURATION")
						{
							dur = _wtoi64(sl.back().c_str());
						}
					}

					delete [] buff;

					fclose(fp);
				}

				hasimg = true;
			}
			else
			{
				ImageGrabber ig;

				hasimg = ig.GetInfo(path.c_str(), 0.5f, &bm, &tags, &dur);

				if(hasimg && dur >= 0)
				{
					Util::ImageCompressor ic;

					if(ic.SaveJPEG(thumb.c_str(), bm.width, bm.height, bm.width * 4, bm.bits))
					{
						if(FILE* fp = _wfopen(mediainfo.c_str(), _T("wt, ccs=UNICODE")))
						{
							for(auto i = tags.begin(); i != tags.end(); i++)
							{
								fwprintf(fp, L"%s=%s\n", i->first.c_str(), i->second.c_str());
							}

							fwprintf(fp, L"__DURATION=%I64d\n", dur);

							fclose(fp);
						}
					}
				}
			}

			if(hasimg)
			{
				int width = m_width;
				int height = m_height;

				if(!m_stretch)
				{
					Vector4i r = DeviceContext::FitRect(Vector2i(bm.width, bm.height), Vector2i(m_width, m_height));

					width = r.width();
					height = r.height();
				}

				bm.Resize(width, height);
			}

			{
				CAutoLock cAutoLock(&m_thread);

				if(!m_thread) return false;

				if(index >= m_items.size()) 
				{
					return false;
				}

				Item& item = m_items[index];

				if(!item.loading)
				{
					return false;
				}

				item.bogus = true;

				bool hastitle = false;

				std::list<std::wstring> sl;

				auto i = tags.find(L"TITL");

				if(i != tags.end())
				{
					std::wstring title = i->second;

					i = tags.find(L"HTNM");

					if(i != tags.end())
					{
						int track = _wtoi(i->second.c_str());

						if(track > 0)
						{
							title = Util::Format(L"%02d. %s", track, title.c_str());
						}
					}

					sl.push_back(title);

					hastitle = true;
				}

				if(i != tags.end())
				{
					std::wstring author = i->second;

					i = tags.find(L"CPYR");

					if(i != tags.end())
					{
						if(_wtoi(i->second.c_str()) > 0)
						{
							author += L" (" + i->second + L")";
						}
					}

					sl.push_back(author);
				}

				if(hastitle)
				{
					item.ni.label = Util::Implode(sl, L"\n");
				}

				if(hasimg && m_dc != NULL)
				{
					Texture* t = m_dc->CreateTexture((BYTE*)bm.bits, bm.width, bm.height, bm.width * 4, false);

					if(t != NULL)
					{
						item.tex = t;
						item.bogus = false;
					}
				}

				item.dur = dur;
				item.loading = false;
			}
		}

		return true;
	}
}