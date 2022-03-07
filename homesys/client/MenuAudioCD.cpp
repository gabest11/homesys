#include "stdafx.h"
#include "MenuAudioCD.h"
#include "client.h"

namespace DXUI
{
	MenuAudioCD::MenuAudioCD()
	{
		Location.ChangedEvent.Add(&MenuAudioCD::OnLocationChanged, this);
	}

	bool MenuAudioCD::OnLocationChanged(Control* c, TCHAR drive)
	{
		RemoveAll();

		std::list<std::wstring> files;

		if(DirectShow::GetDriveType(drive, files) == DirectShow::Drive_Audio)
		{
			for(auto i = files.begin(); i != files.end(); i++)
			{
				Add(i->c_str(), L"", L"");
			}
		}

		return true;
	}
}