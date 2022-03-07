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
#include "OggFile.h"

class OggPacket : public CPacket
{
public:
	OggPacket() {skip = false;}

	bool skip;
};

class COggSplitterOutputPin : public CBaseSplitterOutputPin
{
	class Comment
	{
	public: 
		std::wstring m_key, m_value; 

		Comment(LPCWSTR key, LPCWSTR value);
	};

	std::list<Comment*> m_comments;

protected:
	CCritSec m_csPackets;
	std::list<OggPacket*> m_packets;
	OggPacket* m_lastpacket;
	int m_lastseqnum;
	REFERENCE_TIME m_rtLast;
	bool m_skip;

	void ResetState(DWORD seqnum = -1);

public:
	COggSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~COggSplitterOutputPin();

	void AddComment(BYTE* p, int len);
	std::wstring GetComment(LPCWSTR key);

	HRESULT UnpackPage(OggPage& page);
	virtual HRESULT UnpackPacket(OggPacket* p, const BYTE* data, int len) = 0;
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position) = 0;
	OggPacket* GetPacket();
    
	HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};

class COggVorbisOutputPin : public COggSplitterOutputPin
{
	std::list<OggPacket*> m_initpackets;

	DWORD m_audio_sample_rate;
	DWORD m_blocksize[2];
	DWORD m_lastblocksize;
	std::vector<char> m_blockflags;

	HRESULT DeliverPacket(OggPacket* p);
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

public:
	COggVorbisOutputPin(OggVorbisIdHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT UnpackInitPage(OggPage& page);
	HRESULT UnpackPacket(OggPacket* p, const BYTE* data, int len);
	REFERENCE_TIME GetRefTime(__int64 granule_position);

	bool IsInitialized() {return m_initpackets.size() >= 3;}
};

class COggDirectShowOutputPin : public COggSplitterOutputPin
{
public:
	COggDirectShowOutputPin(AM_MEDIA_TYPE* pmt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT UnpackPacket(OggPacket* p, const BYTE* data, int len);
	REFERENCE_TIME GetRefTime(__int64 granule_position);
};

class COggStreamOutputPin : public COggSplitterOutputPin
{
	__int64 m_time_unit;
	__int64 m_samples_per_unit;
	DWORD m_default_len;

public:
	COggStreamOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT UnpackPacket(OggPacket* p, const BYTE* data, int len);
	REFERENCE_TIME GetRefTime(__int64 granule_position);
};

class COggVideoOutputPin : public COggStreamOutputPin
{
public:
	COggVideoOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggAudioOutputPin : public COggStreamOutputPin
{
public:
	COggAudioOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggTextOutputPin : public COggStreamOutputPin
{
public:
	COggTextOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

[uuid("9FF48807-E133-40AA-826F-9B2959E5232D")]
class COggSplitterFilter : public CBaseSplitterFilter
{
protected:
	COggFile* m_file;

	HRESULT CreateOutputPins(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	COggSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~COggSplitterFilter();
};

[uuid("6D3688CE-3E9D-42F4-92CA-8A11119D25CD")]
class COggSourceFilter : public COggSplitterFilter
{
public:
	COggSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
