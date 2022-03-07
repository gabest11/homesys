/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Stream.h"
#include "../util/String.h"

namespace SSF
{
	Stream::Stream()
		: m_line(0)
		, m_col(-1)
		, m_encoding(None)
		, m_codepage(CP_ACP)
	{
	}

	Stream::~Stream()
	{
	}

	void Stream::ThrowError(LPCWSTR fmt, ...)
	{
		va_list args;

		va_start(args, fmt);

		std::wstring str;

		WCHAR* buff = NULL;

		int len = _vscwprintf(fmt, args) + 1;

		if(len > 0)
		{
			buff = new WCHAR[len];

			vswprintf_s(buff, len, fmt, args);

			str = std::wstring(buff, len - 1);
		}

		va_end(args);

		delete [] buff;

		throw Exception(L"Error (Ln %d Col %d): %s", m_line + 1, m_col + 1, str.c_str());
	}

	bool Stream::IsUnicode() const 
	{
		return m_encoding >= UTF8 && m_encoding <= WChar;
	}

	void Stream::SetCharSet(DWORD charset)
	{
		m_codepage = Util::CharSetToCodePage(charset);
	}

	bool Stream::IsWhiteSpace(int c, LPCWSTR morechars)
	{
		return c != 0xa0 && iswspace((WCHAR)c) || morechars && wcschr(morechars, (WCHAR)c);
	}

	//

	InputStream::InputStream(bool comments)
		: m_comments(comments)
	{
		m_items = 0;	
	}

	InputStream::~InputStream()
	{
	}

	int InputStream::Next()
	{
		int cur = NextByte();

		if(m_encoding == None)
		{
			m_encoding = AChar;

			switch(cur)
			{
			case 0xef: 
				if(NextByte() == 0xbb && NextByte() == 0xbf) m_encoding = UTF8;
				cur = NextByte();
				break;
			case 0xff: 
				if(NextByte() == 0xfe) m_encoding = UTF16LE;
				cur = NextByte();
				break;
			case 0xfe:
				if(NextByte() == 0xff) m_encoding = UTF16BE;
				cur = NextByte();
				break;
			}
		}

		if(cur != EOS)
		{
			int i, c;

			switch(m_encoding)
			{
			case UTF8: 
				for(i = 7; i >= 0 && (cur & (1 << i)); i--);
				cur &= (1 << i) - 1;
				while(++i < 7) {c = NextByte(); if(c == EOS) {cur = EOS; break;} cur = (cur << 6) | (c & 0x3f);}
				break;
			case UTF16LE: 
				c = NextByte();
				if(c == EOS) {cur = EOS; break;}
				cur = (c << 8) | cur;
				break;
			case UTF16BE: 
				c = NextByte();
				if(c == EOS) {cur = EOS; break;}
				cur = cur | (c << 8);
				break;
			case WChar:
				break;
			case AChar:
				char cc[2] = {cur, 0};
				if(IsDBCSLeadByteEx(m_codepage, cur)) {cc[1] = NextByte(); if(cc[1] == EOS) break;}
				MultiByteToWideChar(m_codepage, 0, cc, cc[1] != 0 ? 2 : 1, (WCHAR*)&cur, 1);
				break;
			}
		}

		return cur;
	}

	int InputStream::Push()
	{
		int c = Next();

		if(m_items >= 2) 
		{
			ThrowError(L"input stream queue is full");
		}

		m_queue[m_items++] = c;

		return c;
	}

	int InputStream::Pop()
	{
		if(m_items == 0)
		{
			ThrowError(L"stream error, cannot continue");
		}

		int c = m_queue[0];

		m_queue[0] = m_queue[1];

		m_items--;

		if(c != EOS)
		{
			if(c == '\n') 
			{
				m_line++; 
				m_col = -1;
			}

			m_col++;
		}

		return c;
	}

	int InputStream::Peek()
	{
		if(m_comments)
		{
			while(m_items < 2)
			{
				Push();
			}

			if(m_queue[0] == '/')
			{
				if(m_queue[1] == '/')
				{
					while(m_items > 0) Pop();
					int c;
					do {Push(); c = Pop();} while(!(c == '\n' || c == EOS));
					return Peek();
				}
				else if(m_queue[1] == '*')
				{
					while(m_items > 0) Pop();
					int c1, c2;
					Push();
					do {c2 = Push(); c1 = Pop();} while(!((c1 == '*' && c2 == '/') || c1 == EOS));
					Pop();
					return Peek();
				}
			}
		}
		else
		{
			if(m_items == 0)
			{
				Push();
			}
		}

		return m_queue[0];
	}

	int InputStream::Get()
	{
		if(m_items == 0) 
		{
			Peek();
		}

		return Pop();
	}

	int InputStream::SkipWhiteSpace(LPCWSTR morechars)
	{
		int c = Peek();

		for(; IsWhiteSpace(c, morechars); c = Peek()) 
		{
			Get();
		}

		return c;
	}

	int InputStream::ReadString(std::wstring& s, int eol)
	{
		s.clear();

		int c;

		while((c = Peek()) != EOS)
		{
			c = Get();

			if(c == '\r') continue;
			if(c == '\n' || c == eol) break;
			
			s += (wchar_t)c;
		}

		if(c == EOS && eol == '\n' && !s.empty()) // missing \n in last line?
		{
			return '\n'; // fake it!
		}

		return c != EOS ? c : 0;
	}

	// FileInputStream

	FileInputStream::FileInputStream(LPCWSTR fn, bool comments) 
		: InputStream(comments)
		, m_file(NULL)
	{
		m_file = _wfopen(fn, L"r");

		if(m_file == NULL) 
		{
			ThrowError(L"cannot open file '%s'", fn);
		}
	}

	FileInputStream::~FileInputStream()
	{
		if(m_file != NULL)
		{
			fclose(m_file); 
			
			m_file = NULL;
		}
	}

	int FileInputStream::NextByte()
	{
		return fgetc(m_file);
	}

	// MemoryInputStream

	MemoryInputStream::MemoryInputStream(BYTE* buff, int len, bool copy, bool free, bool comments)
		: InputStream(comments)
		, m_buff(NULL)
		, m_pos(0)
		, m_len(len)
	{
		if(copy)
		{
			m_buff = new BYTE[len];
			memcpy(m_buff, buff, len);
			m_free = true;
		}
		else
		{
			m_buff = buff;
			m_free = free;
		}

		if(m_buff == NULL)
		{
			ThrowError(L"memory stream pointer is NULL");
		}
	}

	MemoryInputStream::~MemoryInputStream()
	{
		if(m_free) 
		{
			delete [] m_buff;
			
			m_buff = NULL;
		}
	}

	int MemoryInputStream::NextByte()
	{
		if(m_pos >= m_len) 
		{
			return Stream::EOS;
		}

		return (int)m_buff[m_pos++];
	}

	// WCharInputStream
	
	WCharInputStream::WCharInputStream(LPCWSTR str, bool comments)
		: InputStream(comments)
		, m_str(str)
		, m_pos(0)
	{
		m_encoding = Stream::WChar; // HACK: it should return real bytes from NextByte (two per wchar_t), but this way it's a lot more simple...
	}

	int WCharInputStream::NextByte()
	{
		if(m_pos >= m_str.size()) 
		{
			return Stream::EOS;
		}

		return m_str[m_pos++];
	}

	// OutputStream

	OutputStream::OutputStream(Encoding e)
	{
		m_encoding = e;
		m_bof = true;
	}

	OutputStream::~OutputStream()
	{
	}

	void OutputStream::PutChar(WCHAR c)
	{
		if(m_bof)
		{
			m_bof = false;

			switch(m_encoding)
			{
			case UTF8:
			case UTF16LE: 
			case UTF16BE:
				PutChar(0xfeff);
				break;
			}
		}

		switch(m_encoding)
		{
		case UTF8: 
			if(0 <= c && c < 0x80) // 0xxxxxxx
			{
				NextByte(c);
			}
			else if(0x80 <= c && c < 0x800) // 110xxxxx 10xxxxxx
			{
				NextByte(0xc0 | ((c<<2)&0x1f));
				NextByte(0x80 | ((c<<0)&0x3f));
			}
			else if(0x800 <= c && c < 0xFFFF) // 1110xxxx 10xxxxxx 10xxxxxx
			{
				NextByte(0xe0 | ((c<<4)&0x0f));
				NextByte(0x80 | ((c<<2)&0x3f));
				NextByte(0x80 | ((c<<0)&0x3f));
			}
			else
			{
				NextByte('?');
			}
			break;
		case UTF16LE:
			NextByte(c & 0xff);
			NextByte((c >> 8) & 0xff);
			break;
		case UTF16BE: 
			NextByte((c >> 8) & 0xff);
			NextByte(c & 0xff);
			break;
		case WChar:
			NextByte(c);
			break;
		}
	}

	void OutputStream::PutString(LPCWSTR fmt, ...)
	{
		va_list args;

		va_start(args, fmt);

		WCHAR* buff = NULL;

		int len = _vscwprintf(fmt, args) + 1;

		if(len > 0)
		{
			buff = new WCHAR[len];

			vswprintf_s(buff, len, fmt, args);
		}

		va_end(args);

		if(buff != NULL)
		{
			for(int i = 0; buff[i] != 0; i++)
			{
				PutChar(buff[i]);
			}
		}

		delete [] buff;
	}

	// WCharOutputStream

	WCharOutputStream::WCharOutputStream()
		: OutputStream(WChar)
	{
	}

	void WCharOutputStream::NextByte(int b)
	{
		m_str += (WCHAR)b;
	}

	// DebugOutputStream

	DebugOutputStream::DebugOutputStream()
		: OutputStream(WChar)
	{
	}

	DebugOutputStream::~DebugOutputStream()
	{
		wprintf(L"%s\n", m_str.c_str());
	}

	void DebugOutputStream::NextByte(int b)
	{
		if(b == '\n') 
		{
			wprintf(L"%s\n", m_str.c_str()); 

			m_str.clear();
		}
		else if(b != '\r') 
		{
			m_str += (WCHAR)b;
		}
	}
}