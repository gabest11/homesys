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
#include "DSMSplitterFile.h"

[uuid("7691533B-082A-4264-BA2E-D2F654B679D0")]
class CDSMSplitterFilter : public CBaseSplitterFilter
{
protected:
	CDSMSplitterFile* m_file;
	
	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CDSMSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CDSMSplitterFilter();

	// IKeyFrameInfo

	STDMETHODIMP_(HRESULT) GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP_(HRESULT) GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

[uuid("EF767077-D96B-48C7-BEC5-0C77F2DABA48")]
class CDSMSourceFilter : public CDSMSplitterFilter
{
public:
	CDSMSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
