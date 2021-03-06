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
#include "MP4SplitterFile.h"

[uuid("61F47056-E400-43d3-AF1E-AB7DFFD4C4AD")]
class CMP4SplitterFilter : public CBaseSplitterFilter
{
	struct TrackPosition 
	{
		DWORD /*AP4_Ordinal*/ index; 
		UINT64 /*AP4_TimeStamp*/ ts;
	};

	std::map<DWORD, TrackPosition> m_trackpos;
	SIZE m_framesize;

protected:
	CMP4SplitterFile* m_file;

	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CMP4SplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMP4SplitterFilter();

	// IKeyFrameInfo

	STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

[uuid("3CCC052E-BDEE-408a-BEA7-90914EF2964B")]
class CMP4SourceFilter : public CMP4SplitterFilter
{
public:
	CMP4SourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
