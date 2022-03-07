#include "stdafx.h"
#include "Navigator.h"
#include "Control.h"
#include "../DirectShow/DirectShow.h"
#include "../util/String.h"

namespace DXUI
{
	Navigator::Navigator() 
	{
		Location.Set(&Navigator::SetLocation, this);

		ClickedEvent.Add0([&] (Control* c) -> bool 
		{
			Control::UserActionEvent(c);
		
			return true;
		});
	}

	void Navigator::SetLocation(std::wstring& value)
	{
		clear();

		if(!value.empty())
		{
			Util::Replace(value, L"/", L"\\");

			std::set<std::wstring> exts;

			if(!Extensions->empty())
			{
				std::list<std::wstring> sl;
				Util::Explode((std::wstring)Extensions, sl, '|');
				exts.insert(sl.begin(), sl.end());
			}

			std::wstring path = Util::CombinePath(value.c_str(), L"*.*");

			WIN32_FIND_DATA fd = {0};

			HANDLE hFind = FindFirstFile(path.c_str(), &fd);

			if(hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if(fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
					{
						continue;
					}

					if(fd.cFileName[0] == '.')
					{
						continue;
					}

					NavigatorItem item;

					item.name = fd.cFileName;
					item.label = item.name;

					((DWORD*)&item.size)[0] =  fd.nFileSizeLow;
					((DWORD*)&item.size)[1] =  fd.nFileSizeHigh;
						
					path = Util::ResolveShortcut(Util::CombinePath(value.c_str(), fd.cFileName).c_str(), &item.islnk);

					item.isdir = PathIsDirectory(path.c_str()) != FALSE;

					if(!item.isdir && !exts.empty())
					{
						if(LPCWSTR s = PathFindExtension(path.c_str()))
						{
							if(exts.find(Util::MakeLower(std::wstring(s))) == exts.end())
							{
								continue;
							}
						}
					}

					push_back(item);
				}
				while(FindNextFile(hFind, &fd));
				
				FindClose(hFind);
			}

			Sort();

			NavigatorItem item;

			item.isdir = true;
			item.name = L"..";
			item.label = item.name;

			push_front(item);
		}
		else
		{
			std::list<std::wstring> rootdrives;

			for(WCHAR drive[] = L"C:"; drive[0] <= 'Z'; drive[0]++)
			{
				if(GetDriveType(drive) != DRIVE_NO_ROOT_DIR)
				{
					NavigatorItem item;

					item.isdir = true;
					item.name = std::wstring(drive) + L"\\";
					item.label = item.name;

					std::wstring label = DirectShow::GetDriveLabel(drive[0]);
					
					if(!label.empty())
					{
						item.label = Util::Format(L"%s (%c:)", label.c_str(), drive[0]);
					}

					push_back(item);
				}
			}
		}
	}

	bool Navigator::Browse(const NavigatorItem& item)
	{
		if(item.isdir)
		{
			Location = GetPath(&item);
		}
		else
		{
			ClickedEvent(NULL, &item);
		}

		return true;
	}

	std::wstring Navigator::GetPath(const NavigatorItem* item)
	{
		std::wstring location = Location;

		std::wstring path;

		if(location.empty())
		{
			path = item->name;
		}
		else if(!(location.size() == 3 && item->name == L".."))
		{
			path = Util::CombinePath(location.c_str(), item->name.c_str());
		}

		path = Util::ResolveShortcut(path.c_str());

		return path;
	}

	void Navigator::Sort()
	{
		if(size() < 2) return;

		std::vector<const NavigatorItem*> items;

		items.reserve(size());

		for(auto i = begin(); i != end(); i++)
		{
			items.push_back(&*i);
		}
	
		std::sort(items.begin(), items.end(), [] (const NavigatorItem* a, const NavigatorItem* b) -> bool
		{
			if(a == b) return false;
			if(a->isdir && !b->isdir) return true;
			if(!a->isdir && b->isdir) return false;
			return wcsicmp(a->name.c_str(), b->name.c_str()) < 0;
		});

		list<NavigatorItem> tmp;

		for(auto i = items.begin(); i != items.end(); i++)
		{
			tmp.push_back(**i);
		}

		clear();

		insert(end(), tmp.begin(), tmp.end());
	}
}