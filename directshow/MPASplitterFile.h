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

class CMpaSplitterFile : public CBaseSplitterFile
{
	enum {none, mpa, mp4a} m_mode;

	CMediaType m_mt;

	CBaseSplitterFile::MpegAudioHeader m_mpahdr;
	CBaseSplitterFile::AACHeader m_aachdr;

	REFERENCE_TIME m_dur;

	__int64 m_start;
	__int64 m_end;

	__int64 m_totalbps;
	std::map<__int64, int> m_pos2bps;

	std::map<DWORD, std::wstring> m_tags;

	HRESULT Init();

	void AdjustDuration(int nBytesPerSec);

public:
	CMpaSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr);

	const CMediaType& GetMediaType() {return m_mt;}

	REFERENCE_TIME GetDuration() {return m_dur;}

	__int64 GetStartPos() {return m_start;}
	__int64 GetEndPos() {return m_end;}

	bool Sync(int limit = 0x2000);
	bool Sync(int& FrameSize, REFERENCE_TIME& rtDuration, int limit = 0x2000);

	bool GetTag(DWORD tag, std::wstring& s);
};
