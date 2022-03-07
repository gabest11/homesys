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
#include "MatroskaReader.h"

class MatroskaPacket : public CPacket
{
public:
	MatroskaReader::BlockGroup* group;

	MatroskaPacket() 
		: group(NULL) 
	{
	}

	virtual ~MatroskaPacket() 
	{
		delete group;
	}

	int GetDataSize()
	{
		int size = 0;

		for(auto i = group->Block.BlockData.begin(); i != group->Block.BlockData.end(); i++)
		{
			size += (*i)->size();
		}

		return size;
	}
};

class CMatroskaSplitterOutputPin : public CBaseSplitterOutputPin
{
	HRESULT DeliverBlock(MatroskaPacket* mp);

	REFERENCE_TIME m_atpf;

	CCritSec m_csQueue;

	std::list<MatroskaPacket*> m_packets;

protected:
	HRESULT DeliverPacket(CPacket* p);

public:
	CMatroskaSplitterOutputPin(REFERENCE_TIME atpf, const std::vector<CMediaType>& mts, LPCWSTR pName, CBaseSplitterFilter* pFilter, HRESULT* phr);
	virtual ~CMatroskaSplitterOutputPin();

	HRESULT DeliverEndFlush();
	HRESULT DeliverEndOfStream();
};

[uuid("149D2E01-C32E-4939-80F7-C07B81015A7A")]
class CMatroskaSplitterFilter : public CBaseSplitterFilter
{
	void SetupChapters(LPCWSTR lng, MatroskaReader::ChapterAtom* parent, int level = 0);
	void InstallFonts();
	void SendVorbisHeaderSample();

	MatroskaReader::CMatroskaNode* m_segment;
	MatroskaReader::CMatroskaNode* m_cluster;
	MatroskaReader::CMatroskaNode* m_block;

protected:
	MatroskaReader::CMatroskaReaderFile* m_file;

	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	std::map<DWORD, MatroskaReader::TrackEntry*> m_tracks;

	MatroskaReader::TrackEntry* GetTrackEntryAt(UINT aTrackIdx);	

public:
	CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMatroskaSplitterFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IKeyFrameInfo

	STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

[uuid("0A68C3B5-9164-4a54-AFBF-995B2FF0E0D4")]
class CMatroskaSourceFilter : public CMatroskaSplitterFilter
{
public:
	CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
