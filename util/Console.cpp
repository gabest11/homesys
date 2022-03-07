#include "StdAfx.h"
#include "Console.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

namespace Util
{
	Console::Console(LPCWSTR title, bool open)
		: m_console(NULL)
		, m_title(title)
	{
		if(open) Open();
	}

	Console::~Console()
	{
		Close();
	}

	void Console::Open()
	{
		if(m_console == NULL)
		{
			CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 

			AllocConsole();

			SetConsoleTitle(m_title.c_str());

			m_console = GetStdHandle(STD_OUTPUT_HANDLE);

			COORD size;

			size.X = 100;
			size.Y = 300;

			SetConsoleScreenBufferSize(m_console, size);

			GetConsoleScreenBufferInfo(m_console, &csbiInfo);

			SMALL_RECT rect;

			rect = csbiInfo.srWindow;
			rect.Right = rect.Left + 99;
			rect.Bottom = rect.Top + 64;

			SetConsoleWindowInfo(m_console, TRUE, &rect);

			*stdout = *_fdopen(_open_osfhandle((long)m_console, _O_TEXT), "w");

			setvbuf(stdout, NULL, _IONBF, 0);
		}
	}

	void Console::Close()
	{
		if(m_console != NULL)
		{
			FreeConsole(); 
		
			m_console = NULL;
		}
	}
}