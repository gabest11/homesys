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

#include "BaseSplitter.h"

class CMPEGSplitterFile : public CBaseSplitterFile
{
	HRESULT Init();

public:
	CMPEGSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr);

	REFERENCE_TIME NextPTS(DWORD id);

	CCritSec m_csProps;

	enum {None, PS, ES, PVA} m_type;
	struct {REFERENCE_TIME start, end;} m_time;
	struct {__int64 start, end;} m_pos;
	int m_bitrate;

	class Stream
	{
	public:
		CMediaType mt;
		union {struct {BYTE pes, ps1; WORD extra;}; DWORD value;} id;
		Stream() {id.value = 0;}
		operator DWORD() const {return id.value;}
		bool operator == (const Stream& s) const {return id.value == s.id.value;}
	};

	enum {Video, Audio, Subpic, Unknown};

	static LPCWSTR ToString(int streamtype)
	{
		switch(streamtype)
		{
		case Video: return L"Video";
		case Audio: return L"Audio";
		case Subpic: return L"Subtitle";
		default: return L"Unknown";
		}
	}

	std::list<Stream> m_streams[Unknown];

	HRESULT SearchStreams(__int64 start, __int64 stop);

	DWORD AddStream(BYTE pesid, DWORD len);
	DWORD AddStream(BYTE pesid, DWORD len, int& type);

	void UpdatePrograms(const TSHeader& h);
};
