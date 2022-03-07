#include "StdAfx.h"
#include "FontInstaller.h"
#include "DirectShow.h"

CFontInstaller::CFontInstaller()
{
}

CFontInstaller::~CFontInstaller()
{
	Uninstall();
}

bool CFontInstaller::Install(const std::vector<BYTE>& data)
{
	return Install(&data[0], data.size());
}

bool CFontInstaller::Install(const void* data, UINT len)
{
	return InstallFontFile(data, len) || InstallFontMemory(data, len);
}

void CFontInstaller::Uninstall()
{
	std::for_each(m_fonts.begin(), m_fonts.end(), [] (const HANDLE& h) 
	{
		RemoveFontMemResourceEx(h);
	});

	m_fonts.clear();

	std::for_each(m_files.begin(), m_files.end(), [] (const std::wstring& fn) 
	{
		RemoveFontResourceEx(fn.c_str(), FR_PRIVATE, 0); 

		if(!DeleteFile(fn.c_str()))
		{
			MoveFileEx(fn.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		}
	});

	m_files.clear();
}

bool CFontInstaller::InstallFontMemory(const void* data, UINT len)
{
	DWORD nFonts = 0;
	
	HANDLE hFont = AddFontMemResourceEx((PVOID)data, len, NULL, &nFonts);

	if(hFont && nFonts > 0) 
	{
		m_fonts.push_back(hFont);
	}

	return hFont && nFonts > 0;
}

bool CFontInstaller::InstallFontFile(const void* data, UINT len)
{
	std::wstring fn;

	if(!DirectShow::GetTempFileName(fn))
	{
		return false;
	}

	HANDLE file = CreateFile(fn.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

	if(file == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD written;

	WriteFile(file, data, len, &written, NULL);

	CloseHandle(file);

	if(!AddFontResourceEx(fn.c_str(), FR_PRIVATE, 0))
	{
		DeleteFile(fn.c_str());

		return false;
	}

	return true;
}
