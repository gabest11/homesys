#pragma once

class CFontInstaller
{
	std::list<HANDLE> m_fonts;
	std::list<std::wstring> m_files;

	bool InstallFontMemory(const void* data, UINT len);
	bool InstallFontFile(const void* data, UINT len);

public:
	CFontInstaller();
	virtual ~CFontInstaller();

	bool Install(const std::vector<BYTE>& data);
	bool Install(const void* data, UINT len);	
	void Uninstall();
};
