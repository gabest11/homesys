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
#include "MpaSplitterFile.h"

[uuid("0E9D4BF7-CBCB-46C7-BD80-4EF223A3DC2B")]
class CMPASplitterFilter : public CBaseSplitterFilter
{
	REFERENCE_TIME m_start;

protected:
	CMpaSplitterFile* m_file;

	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	STDMETHODIMP GetDuration(LONGLONG* pDuration);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CMPASplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMPASplitterFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};

[uuid("59A0DB73-0287-4C9A-9D3C-8CFF39F8E5DB")]
class CMPASourceFilter : public CMPASplitterFilter
{
public:
	CMPASourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
