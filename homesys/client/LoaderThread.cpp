#include "StdAfx.h"
#include "LoaderThread.h"
#include "client.h"

static bool TestFont(LPCWSTR name)
{
	LOGFONT lf;

	memset(&lf, 0, sizeof(lf));

	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = ANTIALIASED_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	lf.lfHeight = 12;
	lf.lfWeight = FW_NORMAL;
	lf.lfItalic = 0;
	lf.lfUnderline = 0;
	lf.lfStrikeOut = 0;
	wcscpy(lf.lfFaceName, name);

	HFONT hFont = CreateFontIndirect(&lf);

	if(hFont != NULL)
	{
		HDC hDC = CreateCompatibleDC(NULL);

		SelectObject(hDC, hFont);

		GetTextFace(hDC, sizeof(lf.lfFaceName) / sizeof(lf.lfFaceName[0]), lf.lfFaceName);

		DeleteDC(hDC);

		DeleteFont(hFont);

		return wcsicmp(name, lf.lfFaceName) == 0;
	}

	return false;
}
	
LoaderThread::LoaderThread()
{
	m_skin = Util::Config(L"Homesys", L"Settings").GetString(L"Skin", DEFAULT_SKIN);

	LPCWSTR xmls[] = {L"dxui.xml", L"lang.xml"};

	for(int i = 0; i < sizeof(xmls) / sizeof(xmls[0]); i++)
	{
		std::vector<BYTE> buff;

		if(GetResource(xmls[i], buff) && !buff.empty())
		{
			TiXmlDocument doc;

			doc.Parse((const char*)buff.data(), NULL, TIXML_ENCODING_UTF8);

			if(TiXmlElement* root = doc.FirstChildElement("DXUI"))
			{
				for(TiXmlElement* n = root->FirstChildElement(); n != NULL; n = n->NextSiblingElement())
				{
					if(n->ValueStr() == "Color")
					{
						struct {LPCSTR key; DWORD& value;} colors[] = 
						{
							{"Text1", DXUI::Color::Text1}, 
							{"Text2", DXUI::Color::Text2}, 
							{"Text3", DXUI::Color::Text3}, 
							{"MenuBackground1", DXUI::Color::MenuBackground1}, 
							{"MenuBackground2", DXUI::Color::MenuBackground2}, 
							{"MenuBackground3", DXUI::Color::MenuBackground3}, 
							{"SeparatorLine", DXUI::Color::SeparatorLine}, 
						};

						unsigned int c;

						for(int i = 0; i < sizeof(colors) / sizeof(colors[0]); i++)
						{
							if(n->QueryHexAttribute(colors[i].key, &c) == TIXML_SUCCESS) 
							{
								colors[i].value = (DWORD)c;
							}
						}
					}
					else if(n->ValueStr() == "FontType")
					{
						struct {LPCSTR key; std::wstring& value;} fonts[] = 
						{
							{"Normal", DXUI::FontType::Normal}, 
							{"Bold", DXUI::FontType::Bold},
						};

						std::string s;

						for(int i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++)
						{
							if(n->QueryValueAttribute(fonts[i].key, &s) == TIXML_SUCCESS) 
							{
								std::list<std::wstring> sl;

								Util::Explode(Util::UTF8To16(s.c_str()), sl, L",");

								for(auto j = sl.begin(); j != sl.end(); j++)
								{
									std::wstring name = *j; 
								
									if(TestFont(name.c_str())) 
									{
										fonts[i].value = name;

										break;
									}
								}
							}
						}
					}
					else if(n->ValueStr() == "Localization")
					{
						for(TiXmlElement* l = n->FirstChildElement("Language"); l != NULL; l = l->NextSiblingElement())
						{
							std::string id;

							if(l->QueryValueAttribute("id", &id) == TIXML_SUCCESS)
							{
								std::wstring lang = Util::UTF8To16(id.c_str()).c_str();

								for(TiXmlElement* t = l->FirstChildElement("T"); t != NULL; t = t->NextSiblingElement())
								{
									std::string a, b;

									if(t->QueryValueAttribute("a", &a) == TIXML_SUCCESS 
									&& t->QueryValueAttribute("b", &b) == TIXML_SUCCESS)
									{
										Util::Replace(b, "\\n", "\n");

										DXUI::Control::SetString(a.c_str(), Util::UTF8To16(b.c_str()).c_str(), lang.c_str());
									}
								}
							}
						}
					}
				}
			}
		}
	}

	for(int i = 0; i < 4; i++)
	{
		m_threads.push_back(new HelperThread(this));
	}
}

LoaderThread::~LoaderThread()
{
	for(auto i = m_threads.begin(); i != m_threads.end(); i++)
	{
		delete *i;
	}
}

void LoaderThread::QueueItem(const std::wstring hash, const WorkItem& item)
{
	if(m_items.find(hash) == m_items.end())
	{
		m_items[hash] = item;

		if(item.status != 1)
		{
			if(item.priority)
			{
				m_queue.push_front(hash);
			}
			else
			{
				m_queue.push_back(hash);
			}

			for(auto i = m_threads.begin(); i != m_threads.end(); i++)
			{
				(*i)->Create();
			}
		}
	}
}

DWORD LoaderThread::HelperThread::ThreadProc()
{
	Util::Socket::Startup();

	int idle = 0;

	while(m_hThread != NULL)
	{
		std::wstring hash;
		WorkItem item;

		{
			Sleep(1); // LAME

			CAutoLock cAutoLock(m_parent);

			if(m_parent->m_queue.empty())
			{
				if(++idle < 1000)
				{
					continue;
				}
					
				m_hThread = NULL;

				break;
			}

			idle = 0;

			hash = m_parent->m_queue.front();

			m_parent->m_queue.pop_front();

			item = m_parent->m_items[hash];

			// 
			wprintf(L"GET %p %d %d %s\n", m_hThread, m_parent->m_items.size(), m_parent->m_queue.size(), item.url.c_str());
		}

		item.status = -1;

		std::wstring tmp = item.offline + L".tmp";

		Util::Socket s;

		std::map<std::string, std::string> headers;

		if(s.HttpGet(item.url.c_str(), headers))
		{
			if(FILE* fp = _wfopen(tmp.c_str(), L"wb"))
			{
				int size = 0;

				BYTE* buff = new BYTE[4096];

				while(1)
				{
					int n = s.Read(buff, 4096);

					if(n > 0)
					{
						fwrite(buff, n, 1, fp);

						size += n;
					}
					else
					{
						break;
					}
				}

				delete [] buff;

				fclose(fp);

				CAutoLock cAutoLock(m_parent);

				_wunlink(item.offline.c_str());
				_wrename(tmp.c_str(), item.offline.c_str());

				item.status = 1;
			}
		}

		{
			CAutoLock cAutoLock(m_parent);

			m_parent->m_items[hash] = item;
		}
	}

	Util::Socket::Cleanup();

	return 0;
}

// ResourceLoader

bool LoaderThread::GetResource(LPCWSTR id, std::vector<BYTE>& buff, bool priority, bool refresh)
{
	if(!refresh && m_ignored.find(id) != m_ignored.end())
	{
		return false;
	}

	int res = LoadResource(id, buff, priority, refresh);

	if(res < 0)
	{
		IgnoreResource(id);

		return false;
	}

	return res == 1;
}

void LoaderThread::IgnoreResource(LPCWSTR id)
{
	// 
	wprintf(L"Ignoring %s\n", id);

	m_ignored.insert(id);
}

int LoaderThread::LoadResource(LPCWSTR id, std::vector<BYTE>& buff, bool priority, bool refresh)
{
	buff.clear();

	if(id == NULL || wcslen(id) == 0)
	{
		return -1;
	}

	std::wstring fn = id;

	if(fn.find(L"://") == std::wstring::npos)
	{
		Homesys::ZipDecompressor zd;

		std::wstring path = g_env->ResourcePath;

		std::wstring path1 = path + L"\\" + m_skin + L".zip";
		std::wstring path2 = path + L"\\res.zip";

		if(PathFileExists(path1.c_str()) && zd.Decompress(path1.c_str(), fn.c_str(), buff)
		|| PathFileExists(path2.c_str()) && zd.Decompress(path2.c_str(), fn.c_str(), buff))
		{
			return 1;
		}

		return -1;
	}

	LPCWSTR url = fn.c_str();

	std::wstring hash = Homesys::Hash::AsMD5(url);

	std::wstring dir = (std::wstring)g_env->CachePath + L"/" + hash.substr(0, 2);

	CreateDirectory(dir.c_str(), NULL);

	std::wstring offline = dir + L"/" + hash;

	CAutoLock cAutoLock(this);

	auto i = m_items.find(hash);

	if(i != m_items.end())
	{
		WorkItem& item = i->second;

		if(item.status == 1)
		{
			if(refresh)
			{
				if(std::find(m_queue.begin(), m_queue.end(), hash) == m_queue.end())
				{
					if(priority)
					{
						m_queue.push_front(hash);
					}
					else
					{
						m_queue.push_back(hash);
					}

					m_ignored.erase(id);

					for(auto i = m_threads.begin(); i != m_threads.end(); i++)
					{
						(*i)->Create();
					}
				}
			}
			
			FileGetContents(offline.c_str(), buff);
		}

		return item.status;
	}

	WorkItem item;

	item.url = url;
	item.offline = offline;
	item.status = 0;
	item.priority = priority;

	HANDLE file = CreateFile(offline.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(file != INVALID_HANDLE_VALUE)
	{
		FILETIME ftCreate, ftAccess, ftModify;

		GetFileTime(file, &ftCreate, &ftAccess, &ftModify);

		CloseHandle(file);

		if(!refresh && CTime::GetCurrentTime() - CTime(ftCreate) < CTimeSpan(1, 0, 0, 0))
		{
			item.status = 1;
		}

		QueueItem(hash, item);

		if(FileGetContents(offline.c_str(), buff))
		{
			return 1;
		}

		return -1;
	}

	QueueItem(hash, item);

	return 0;
}
