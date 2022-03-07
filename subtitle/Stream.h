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

#pragma once

#include "Exception.h"

namespace SSF
{
	class Stream
	{
	public:
		enum {EOS = -1};
		enum Encoding {None, Unknown, UTF8, UTF16LE, UTF16BE, WChar, AChar};

	protected:
		int m_line;
		int m_col;
		Encoding m_encoding;
		DWORD m_codepage;

	public:
		Stream();
		virtual ~Stream();

		int GetLine() const {return m_line;}
		int GetCol() const {return m_col;}

		bool IsUnicode() const;
		void SetCharSet(DWORD charset);

		static bool IsWhiteSpace(int c, LPCWSTR morechars = NULL);

		void ThrowError(LPCWSTR fmt, ...);
	};

	class InputStream : public Stream
	{
		int m_queue[2];
		int m_items;
		bool m_comments;

		int Push();
		int Pop();
		int Next();

	protected:
		virtual int NextByte() = 0;

	public:
		InputStream(bool comments = true);
		virtual ~InputStream();

		int Peek();
		int Get();
		int SkipWhiteSpace(LPCWSTR morechars = NULL);
		int ReadString(std::wstring& s, int eol = '\n'); // returns 'eol' OR zero if EOS is reached
	};

	class FileInputStream : public InputStream
	{
		FILE* m_file;

	protected:
		int NextByte();

	public:
		FileInputStream(LPCWSTR fn, bool comments = true);
		virtual ~FileInputStream();
	};

	class MemoryInputStream : public InputStream
	{
		BYTE* m_buff;
		int m_pos;
		int m_len;
		bool m_free;

	protected:
		int NextByte();

	public:
		MemoryInputStream(BYTE* buff, int len, bool copy, bool free, bool comments = true);
		virtual ~MemoryInputStream();
	};

	class WCharInputStream : public InputStream
	{
		std::wstring m_str;
		int m_pos;

	protected:
		int NextByte();

	public:
		WCharInputStream(LPCWSTR str, bool comments = true);
	};

	class OutputStream : public Stream
	{
		bool m_bof;

	protected:
		virtual void NextByte(int b) = 0;

	public:
		OutputStream(Encoding e);
		virtual ~OutputStream();

		void PutChar(WCHAR c);
		void PutString(LPCWSTR fmt, ...);
	};

	class WCharOutputStream : public OutputStream
	{
		std::wstring m_str;

	protected:
		void NextByte(int b);

	public:
		WCharOutputStream();

		const std::wstring& ToString() {return m_str;}
	};

	class DebugOutputStream : public OutputStream
	{
		std::wstring m_str;

	protected:
		void NextByte(int b);

	public:
		DebugOutputStream();
		virtual ~DebugOutputStream();
	};
}