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
#include "MPEGSplitterFile.h"

[uuid("DC257063-045F-4BE2-BD5B-E12279C464F1")]
class CMPEGSplitterFilter : public CBaseSplitterFilter
{
	REFERENCE_TIME m_start;

protected:
	CMPEGSplitterFile* m_file;

	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	bool DemuxNextPacket(REFERENCE_TIME rtStartOffset);

public:
	CMPEGSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMPEGSplitterFilter));

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};

[uuid("1365BE7A-C86A-473C-9A41-C0A6E82C9FA4")]
class CMPEGSourceFilter : public CMPEGSplitterFilter
{
public:
	CMPEGSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMPEGSourceFilter));
};

class CMPEGSplitterOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
	CPacket* m_packet;
	struct {REFERENCE_TIME current, offset;} m_time;

protected:
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT DeliverPacket(CPacket* p);
	HRESULT DeliverEndFlush();

public:
	CMPEGSplitterOutputPin(const std::vector<CMediaType>& mts, LPCWSTR pName, CBaseSplitterFilter* pFilter, HRESULT* phr);
	virtual ~CMPEGSplitterOutputPin();
};
