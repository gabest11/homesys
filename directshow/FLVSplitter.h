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

[uuid("47E792CF-0BBE-4F7A-859C-194B0768650A")]
class CFLVSplitterFilter : public CBaseSplitterFilter
{
	UINT32 m_offset;

	std::list<CPacket*> m_h264queue;

	bool Sync(__int64& pos);

protected:
	CBaseSplitterFile* m_file;

	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CFLVSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CFLVSplitterFilter();
};

[uuid("C9ECE7B3-1D8E-41F5-9F24-B255DF16C087")]
class CFLVSourceFilter : public CFLVSplitterFilter
{
public:
	CFLVSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
/*
#include "BaseVideoFilter.h"
#include "VP62.h"

__if_exists(VP62) {

[uuid("7CEEEECF-3FEE-4548-B529-C254CAF4D182")]
class CFLVVideoDecoder : public CBaseVideoFilter
{
	VP62 m_dec;

protected:
	HRESULT Transform(IMediaSample* pIn);

public:
	CFLVVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CFLVVideoDecoder();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT CheckInputType(const CMediaType* mtIn);

	// TODO
	//bool m_fDropFrames;
	//HRESULT AlterQuality(Quality q);
};

}
*/