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

#include "StdAfx.h"
#include "DirectShow.h"
#include "AVIFile.h"
#include "AVISplitter.h"
#include "moreuuids.h"

// CAVISplitterFilter

CAVISplitterFilter::CAVISplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CAVISplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
	, m_timeformat(TIME_FORMAT_MEDIA_TIME)
{
}

CAVISplitterFilter::~CAVISplitterFilter()
{
	delete m_file;
}

STDMETHODIMP CAVISplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAVISplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_tFrame.clear();

	delete m_file;

	m_file = new CAVIFile(pAsyncReader, hr);

	if(FAILED(hr)) 
	{
		delete m_file; 
		
		m_file = NULL;
		
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_file->GetTotalTime();

	bool fHasIndex = m_file->HasIndex();

	for(int i = 0; i < m_file->m_strms.size(); i++)
	{
		CAVIFile::Stream* s = m_file->m_strms[i];

		if(fHasIndex && s->cs.size() == 0) 
		{
			continue;
		}

		CMediaType mt;

		std::vector<CMediaType> mts;
		
		std::wstring name, label;

		if(s->strh.fccType == FCC('vids'))
		{
			label = L"Video";

			// ASSERT(s->strf.GetCount() >= sizeof(BITMAPINFOHEADER));

			BITMAPINFOHEADER* pbmi = &((BITMAPINFO*)&s->strf[0])->bmiHeader;

			mt.majortype = MEDIATYPE_Video;
			mt.subtype = FOURCCMap(pbmi->biCompression);
			mt.formattype = FORMAT_VideoInfo;

			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + s->strf.size() - sizeof(BITMAPINFOHEADER));

			memset(mt.Format(), 0, mt.FormatLength());

			memcpy(&pvih->bmiHeader, &s->strf[0], s->strf.size());

			if(s->strh.dwRate > 0) 
			{
				pvih->AvgTimePerFrame = 10000000i64 * s->strh.dwScale / s->strh.dwRate;
			}

			switch(pbmi->biCompression)
			{
			case BI_RGB: 
			case BI_BITFIELDS: 
				switch(pbmi->biBitCount)
				{
				case 1: mt.subtype = MEDIASUBTYPE_RGB1; break;
				case 4: mt.subtype = MEDIASUBTYPE_RGB4; break;
				case 8: mt.subtype = MEDIASUBTYPE_RGB8; break;
				case 16: mt.subtype = MEDIASUBTYPE_RGB565; break;
				case 24: mt.subtype = MEDIASUBTYPE_RGB24; break;
				case 32: mt.subtype = MEDIASUBTYPE_ARGB32; break;
				default: mt.subtype = MEDIASUBTYPE_NULL; break;
				}
				break;
//			case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
//			case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
			}

			if(!s->cs.empty() && pvih->AvgTimePerFrame > 0)
			{
				__int64 size = 0;

				for(size_t i = 0; i < s->cs.size(); i++)
				{
					size += s->cs[i].orgsize;
				}

				pvih->dwBitRate = (DWORD)(size * 8 / s->cs.size() * 10000000i64 / pvih->AvgTimePerFrame);
			}

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 ? s->strh.dwSuggestedBufferSize * 3 / 2 : (pvih->bmiHeader.biWidth * pvih->bmiHeader.biHeight * 4));

			mts.push_back(mt);
		}
		else if(s->strh.fccType == FCC('auds'))
		{
			label = L"Audio";

			// ASSERT(s->strf.GetCount() >= sizeof(WAVEFORMATEX) || s->strf.GetCount() == sizeof(PCMWAVEFORMAT));

            WAVEFORMATEX* wfe = (WAVEFORMATEX*)&s->strf[0];

			if(wfe->nBlockAlign == 0) continue;

			mt.majortype = MEDIATYPE_Audio;
			mt.subtype = FOURCCMap(wfe->wFormatTag);
			mt.formattype = FORMAT_WaveFormatEx;

			mt.SetFormat((BYTE*)wfe, max(s->strf.size(), sizeof(WAVEFORMATEX)));

			wfe = (WAVEFORMATEX*)mt.Format();

			if(s->strf.size() == sizeof(PCMWAVEFORMAT)) 
			{
				wfe->cbSize = 0;
			}

			if(wfe->wFormatTag == WAVE_FORMAT_PCM) 
			{
				wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample >> 3;
			}

			if(wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) 
			{
				mt.subtype = FOURCCMap(WAVE_FORMAT_PCM); // audio renderer doesn't accept fffe in the subtype
			}

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 ? s->strh.dwSuggestedBufferSize * 3 / 2 : (wfe->nChannels * wfe->nSamplesPerSec * 32 >> 3));

			mts.push_back(mt);
		}
		else if(s->strh.fccType == FCC('mids'))
		{
			label = L"Midi";

			mt.majortype = MEDIATYPE_Midi;
			mt.subtype = MEDIASUBTYPE_NULL;
			mt.formattype = FORMAT_None;

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 ? s->strh.dwSuggestedBufferSize * 3 / 2 : 1024 * 1024);
			
			mts.push_back(mt);
		}
		else if(s->strh.fccType == FCC('txts'))
		{
			label = L"Text";

			mt.majortype = MEDIATYPE_Text;
			mt.subtype = MEDIASUBTYPE_NULL;
			mt.formattype = FORMAT_None;

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 ? s->strh.dwSuggestedBufferSize * 3 / 2 : 1024 * 1024);

			mts.push_back(mt);
		}
		else if(s->strh.fccType == FCC('iavs'))
		{
			label = L"Interleaved";

			// ASSERT(s->strh.fccHandler == FCC('dvsd'));

			mt.majortype = MEDIATYPE_Interleaved;
			mt.subtype = FOURCCMap(s->strh.fccHandler);
			mt.formattype = FORMAT_DvInfo;

			mt.SetFormat(&s->strf[0], max(s->strf.size(), sizeof(DVINFO)));

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 ? s->strh.dwSuggestedBufferSize * 3 / 2 : 1024 * 1024);

			mts.push_back(mt);
		}

		if(mts.empty())
		{
			// TRACE(_T("CAVISourceFilter: Unsupported stream (%d)\n"), i);

			continue;
		}

		name = Util::Format(L"%s %d", !s->strn.empty() ? s->strn.c_str() : label.c_str(), i);

		HRESULT hr;

		CBaseSplitterOutputPin* pin = new CAVISplitterOutputPin(mts, name.c_str(), this, &hr);

		AddOutputPin(i, pin);
	}

	for(auto i = m_file->m_info.begin(); i != m_file->m_info.end(); i++)
	{
		switch(i->first)
		{
		case FCC('INAM'): SetProperty(L"TITL", i->second.c_str()); break;
		case FCC('IART'): SetProperty(L"AUTH", i->second.c_str()); break;
		case FCC('ICOP'): SetProperty(L"CPYR", i->second.c_str()); break;
		case FCC('ISBJ'): SetProperty(L"DESC", i->second.c_str()); break;
		}
	}

	m_tFrame.resize(m_file->m_avih.dwStreams);

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool CAVISplitterFilter::DemuxInit()
{
	if(!m_file) return false;

	bool reindex = false;

	for(int i = 0; i < (int)m_file->m_avih.dwStreams && !reindex; i++)
	{
		if(m_file->m_strms[i]->cs.empty() && GetOutputPin(i) != NULL) 
		{
			reindex = true;
		}
	}

	if(reindex)
	{
		m_rtDuration = 0;

		m_nOpenProgress = 0;

		m_abort = false;

		m_file->EmptyIndex();

		m_file->Seek(0);

		std::vector<UINT64> buff;

		buff.resize(m_file->m_avih.dwStreams, 0);

		ReIndex(m_file->GetLength(), &buff[0]);

		if(m_abort) 
		{
			m_file->EmptyIndex();
			
			m_abort = false;
		}

		m_nOpenProgress = 100;
	}

	return true;
}

HRESULT CAVISplitterFilter::ReIndex(__int64 end, UINT64* pSize)
{
	HRESULT hr = S_OK;

	while(S_OK == hr && m_file->GetPos() < end && SUCCEEDED(hr) && !m_abort)
	{
		__int64 pos = m_file->GetPos();

		DWORD id = 0, size;

		if(S_OK != m_file->Read(id) || id == 0)
		{
			return E_FAIL;
		}

		if(id == FCC('RIFF') || id == FCC('LIST'))
		{
			if(S_OK != m_file->Read(size) || S_OK != m_file->Read(id))
			{
				return E_FAIL;
			}

			size += (size & 1) + 8;

			if(id == FCC('AVI ') || id == FCC('AVIX') || id == FCC('movi') || id == FCC('rec '))
			{
				hr = ReIndex(pos + size, pSize);
			}
		}
		else
		{
			if(S_OK != m_file->Read(size))
			{
				return E_FAIL;
			}

			DWORD TrackNumber = TRACKNUM(id);

			if(TrackNumber < m_file->m_strms.size())
			{
				CAVIFile::Stream* s = m_file->m_strms[TrackNumber];

				WORD type = TRACKTYPE(id);

				if(type == 'db' || type == 'dc' || /*type == 'pc' ||*/ type == 'wb'
				|| type == 'iv' || type == '__' || type == 'xx')
				{
					CAVIFile::Stream::chunk c;

					c.filepos = pos;
					c.size = pSize[TrackNumber];
					c.orgsize = size;
					c.fKeyFrame = size > 0; // TODO: find a better way...
					c.fChunkHdr = true;

					s->cs.push_back(c);

					pSize[TrackNumber] += s->GetChunkSize(size);

					REFERENCE_TIME rt = s->GetRefTime(s->cs.size() - 1, pSize[TrackNumber]);

					m_rtDuration = max(rt, m_rtDuration);
				}
			}

			size += (size & 1) + 8;
		}

		m_file->Seek(pos + size);

		m_nOpenProgress = m_file->GetPos() * 100 / m_file->GetLength();

		DWORD cmd;

		if(CheckRequest(&cmd))
		{
			if(cmd == CMD_EXIT) 
			{
				m_abort = true;
			}
			else 
			{
				Reply(S_OK);
			}
		}
	}

	return hr;
}

void CAVISplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	memset(&m_tFrame[0], 0, sizeof(DWORD) * m_file->m_avih.dwStreams);

	m_file->Seek(0);

	if(rt > 0)
	{
		UINT64 minfp = _I64_MAX;

		for(int j = 0; j < m_file->m_strms.size(); j++)
		{
			CAVIFile::Stream* s = m_file->m_strms[j];

			int f = s->GetKeyFrame(rt);

			UINT64 fp = f >= 0 ? s->cs[f].filepos : m_file->GetLength();

			if(!s->IsRawSubtitleStream())
			{

				minfp = min(minfp, fp);
			}
		}

		for(int j = 0; j < (int)m_file->m_strms.size(); j++)
		{
			CAVIFile::Stream* s = m_file->m_strms[j];

			for(size_t i = 0; i < s->cs.size(); i++)
			{
				const CAVIFile::Stream::chunk& c = s->cs[i];

				if(c.filepos >= minfp)
				{
					m_tFrame[j] = i;

					break;
				}
			}
		}
	}
}

bool CAVISplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	int tracks = (int)m_file->m_strms.size();

	std::vector<char> discontinuity(tracks, 0);

	while(SUCCEEDED(hr) && !CheckRequest(NULL))
	{
		int minTrack = tracks;

		UINT64 minFilePos = _I64_MAX;

		for(int i = 0; i < tracks; i++)
		{
			CAVIFile::Stream* s = m_file->m_strms[i];

			DWORD f = m_tFrame[i];

			if(f >= (DWORD)s->cs.size()) continue;

			bool fUrgent = s->IsRawSubtitleStream();

			if(fUrgent || s->cs[f].filepos < minFilePos)
			{
				minTrack = i;
				minFilePos = s->cs[f].filepos;
			}

			if(fUrgent) break;
		}

		if(minTrack == tracks)
		{
			break;
		}

		DWORD& f = m_tFrame[minTrack];

		do
		{
			CAVIFile::Stream* s = m_file->m_strms[minTrack];

			m_file->Seek(s->cs[f].filepos);

			DWORD size = 0;

			if(s->cs[f].fChunkHdr)
			{
				DWORD id = 0;

				if(S_OK != m_file->Read(id) || id == 0 || minTrack != TRACKNUM(id) || S_OK != m_file->Read(size))
				{
					discontinuity[minTrack] = 1;

					break;
				}

				UINT64 expectedsize = -1;

				expectedsize = f < (DWORD)s->cs.size() - 1 ? s->cs[f + 1].size - s->cs[f].size : s->totalsize - s->cs[f].size;

				if(expectedsize != s->GetChunkSize(size))
				{
					discontinuity[minTrack] = 1;
					// ASSERT(0);
					break;
				}
			}
			else
			{
				size = s->cs[f].orgsize;
			}

			CPacket* p = new CPacket(size);

			p->id = minTrack;

			p->start = s->GetRefTime(f, s->cs[f].size);
			p->stop = s->GetRefTime(f + 1, f + 1 < (DWORD)s->cs.size() ? s->cs[f + 1].size : s->totalsize);

			p->flags |= CPacket::TimeValid;

			if(discontinuity[minTrack])
			{
				p->flags |= CPacket::Discontinuity;
			}

			if(s->cs[f].fKeyFrame) 
			{
				p->flags |= CPacket::SyncPoint;
			}

			hr = m_file->ByteRead(&p->buff[0], size);

			if(S_OK != hr)
			{
				delete p;

				return true; // break;
			}
/*
			if(minTrack == 0 && (p->start - m_rtStart) / 10000 < 1000)
			printf("%d (%d): %I64d - %I64d, %I64d - %I64d (pos = %I64d, size = %d)\n", 
				minTrack, (p->flags & CPacket::SyncPoint) != 0 ? 1 : 0,
				(p->start) / 10000, (p->stop) / 10000, 
				(p->start - m_rtStart) / 10000, (p->stop - m_rtStart) / 10000,
				s->cs[f].filepos, size);
*/
			hr = DeliverPacket(p);

			discontinuity[minTrack] = 0;
		}
		while(0);

		f++;
	}

	return true;
}

// IMediaSeeking

STDMETHODIMP CAVISplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_file, VFW_E_NOT_CONNECTED);

	if(m_timeformat == TIME_FORMAT_FRAME)
	{
		for(int i = 0; i < (int)m_file->m_strms.size(); i++)
		{
			CAVIFile::Stream* s = m_file->m_strms[i];

			if(s->strh.fccType == FCC('vids'))
			{
				*pDuration = s->cs.size();

				return S_OK;
			}
		}

		return E_UNEXPECTED;
	}

	return __super::GetDuration(pDuration);
}

STDMETHODIMP CAVISplitterFilter::IsFormatSupported(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	HRESULT hr = __super::IsFormatSupported(pFormat);

	if(S_OK == hr) 
	{
		return hr;
	}

	return *pFormat == TIME_FORMAT_FRAME ? S_OK : S_FALSE;
}

STDMETHODIMP CAVISplitterFilter::GetTimeFormat(GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	*pFormat = m_timeformat;

	return S_OK;
}

STDMETHODIMP CAVISplitterFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	return *pFormat == m_timeformat ? S_OK : S_FALSE;
}

STDMETHODIMP CAVISplitterFilter::SetTimeFormat(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	if(S_OK != IsFormatSupported(pFormat)) 
	{
		return E_FAIL;
	}

	m_timeformat = *pFormat;

	return S_OK;
}

STDMETHODIMP CAVISplitterFilter::GetStopPosition(LONGLONG* pStop)
{
	CheckPointer(pStop, E_POINTER);

	if(FAILED(__super::GetStopPosition(pStop))) 
	{
		return E_FAIL;
	}

	if(m_timeformat == TIME_FORMAT_MEDIA_TIME) 
	{
		return S_OK;
	}

	LONGLONG rt = *pStop;

	if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, rt, &TIME_FORMAT_MEDIA_TIME))) 
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CAVISplitterFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	CheckPointer(pTarget, E_POINTER);

	const GUID& SourceFormat = pSourceFormat ? *pSourceFormat : m_timeformat;
	const GUID& TargetFormat = pTargetFormat ? *pTargetFormat : m_timeformat;
	
	if(TargetFormat == SourceFormat)
	{
		*pTarget = Source; 

		return S_OK;
	}
	else if(TargetFormat == TIME_FORMAT_FRAME && SourceFormat == TIME_FORMAT_MEDIA_TIME)
	{
		for(int i = 0; i < m_file->m_strms.size(); i++)
		{
			CAVIFile::Stream* s = m_file->m_strms[i];

			if(s->strh.fccType == FCC('vids'))
			{
				*pTarget = s->GetFrame(Source);

				return S_OK;
			}
		}
	}
	else if(TargetFormat == TIME_FORMAT_MEDIA_TIME && SourceFormat == TIME_FORMAT_FRAME)
	{
		for(int i = 0; i < m_file->m_strms.size(); i++)
		{
			CAVIFile::Stream* s = m_file->m_strms[i];

			if(s->strh.fccType == FCC('vids'))
			{
				if(Source < 0 || Source >= s->cs.size()) 
				{
					return E_FAIL;
				}

				CAVIFile::Stream::chunk& c = s->cs[(int)Source];

				*pTarget = s->GetRefTime((DWORD)Source, c.size);

				return S_OK;
			}
		}
	}
	
	return E_FAIL;
}

STDMETHODIMP CAVISplitterFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	HRESULT hr;

	hr = __super::GetPositions(pCurrent, pStop);

	if(FAILED(hr) || m_timeformat != TIME_FORMAT_FRAME)
	{
		return hr;
	}

	if(pCurrent)
	{
		if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_FRAME, *pCurrent, &TIME_FORMAT_MEDIA_TIME))) 
		{
			return E_FAIL;
		}
	}

	if(pStop)
	{
		if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, *pStop, &TIME_FORMAT_MEDIA_TIME))) 
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CAVISplitterFilter::SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if(m_timeformat != TIME_FORMAT_FRAME)
	{
		return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
	}

	if(pCurrent == NULL && pStop == NULL
	|| (dwCurrentFlags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
	{
		return S_OK;
	}

	REFERENCE_TIME rtCurrent = m_rtCurrent;
	REFERENCE_TIME rtStop = m_rtStop;

	if((dwCurrentFlags & AM_SEEKING_PositioningBitsMask)
	&& FAILED(ConvertTimeFormat(&rtCurrent, &TIME_FORMAT_FRAME, rtCurrent, &TIME_FORMAT_MEDIA_TIME))) 
	{
		return E_FAIL;
	}

	if((dwStopFlags & AM_SEEKING_PositioningBitsMask)
	&& FAILED(ConvertTimeFormat(&rtStop, &TIME_FORMAT_FRAME, rtStop, &TIME_FORMAT_MEDIA_TIME)))
	{
		return E_FAIL;
	}

	if(pCurrent != NULL)
	{
		switch(dwCurrentFlags & AM_SEEKING_PositioningBitsMask)
		{
		case AM_SEEKING_NoPositioning: break;
		case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
		case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
		case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
		}
	}

	if(pStop != NULL)
	{
		switch(dwStopFlags & AM_SEEKING_PositioningBitsMask)
		{
		case AM_SEEKING_NoPositioning: break;
		case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
		case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
		case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
		}
	}

	if((dwCurrentFlags & AM_SEEKING_PositioningBitsMask) && pCurrent)
	{
		if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_MEDIA_TIME, rtCurrent, &TIME_FORMAT_FRAME))) 
		{
			return E_FAIL;
		}
	}

	if((dwStopFlags&AM_SEEKING_PositioningBitsMask) && pStop)
	{
		if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_MEDIA_TIME, rtStop, &TIME_FORMAT_FRAME))) 
		{
			return E_FAIL;
		}
	}

	return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

// IKeyFrameInfo

STDMETHODIMP CAVISplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	if(!m_file) return E_UNEXPECTED;

	HRESULT hr = S_OK;

	nKFs = 0;

	for(int i = 0; i < m_file->m_strms.size(); i++)
	{
		CAVIFile::Stream* s = m_file->m_strms[i];

		if(s->strh.fccType != FCC('vids')) continue;

		for(size_t j = 0; j < s->cs.size(); j++)
		{
			if(s->cs[j].fKeyFrame) 
			{
				nKFs++;
			}
		}

		if(nKFs == s->cs.size())
		{
			hr = S_FALSE;
		}

		break;
	}

	return hr;
}

STDMETHODIMP CAVISplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if(!m_file) return E_UNEXPECTED;

	if(*pFormat != TIME_FORMAT_MEDIA_TIME && *pFormat != TIME_FORMAT_FRAME) 
	{
		return E_INVALIDARG;
	}

	UINT nKFsTmp = 0;

	for(size_t i = 0; i < m_file->m_strms.size(); i++)
	{
		CAVIFile::Stream* s = m_file->m_strms[i];

		if(s->strh.fccType != FCC('vids')) continue;

		bool fConvertToRefTime = !!(*pFormat == TIME_FORMAT_MEDIA_TIME);

		for(size_t j = 0; j < s->cs.size() && nKFsTmp < nKFs; j++)
		{
			if(s->cs[j].fKeyFrame)
			{
				pKFs[nKFsTmp++] = fConvertToRefTime ? s->GetRefTime(j, s->cs[j].size) : j;
			}
		}

		break;
	}

	nKFs = nKFsTmp;

	return S_OK;
}

// CAVISourceFilter

CAVISourceFilter::CAVISourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CAVISplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}

// CAVISplitterOutputPin

CAVISplitterOutputPin::CAVISplitterOutputPin(const std::vector<CMediaType>& mts, LPCWSTR pName, CBaseSplitterFilter* pFilter, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, phr)
{
}

CAVISplitterOutputPin::~CAVISplitterOutputPin()
{
	for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++)
	{
		delete *i;
	}
}

HRESULT CAVISplitterOutputPin::CheckConnect(IPin* pPin)
{
	CLSID clsid = DirectShow::GetCLSID(pPin);

	if(clsid == CLSID_VideoMixingRenderer || clsid == CLSID_OverlayMixer)
	{
		int i = 0;

		CMediaType mt;

		while(S_OK == GetMediaType(i++, &mt))
		{
			if(mt.majortype == MEDIATYPE_Video 
			&& (mt.subtype == FOURCCMap(FCC('IV32'))
			 || mt.subtype == FOURCCMap(FCC('IV31'))
			 || mt.subtype == FOURCCMap(FCC('IF09'))))
			{
				return E_FAIL;
			}

			mt.InitMediaType();
		}
	}

	return __super::CheckConnect(pPin);
}

// extern FILE* g_log;

HRESULT CAVISplitterOutputPin::DeliverPacket(CPacket* p)
{
	HRESULT hr = S_OK;
	
	if(m_mt.subtype == MEDIASUBTYPE_H264 || m_mt.subtype == MEDIASUBTYPE_AVC1)
	{
		BYTE slice_type = 0;

		CBaseSplitterFile f(p->buff.data(), p->buff.size());

		BYTE b;
		__int64 pos;

		while(f.NextNALU(b, pos))
		{
			BYTE forbidden_zero_bit = b >> 7;
			BYTE nal_ref_idc = (b >> 5) & 3;
			BYTE nal_unit_type = b & 0x1f;

			if(nal_unit_type == 1 || nal_unit_type == 5)
			{
				f.UExpGolombRead(); // first_mb_in_slice
				slice_type = f.UExpGolombRead();
				f.UExpGolombRead(); // pic_patameter_set_id
				f.BitRead(2); // frame_num

				break;
			}
		}
/*
		fwprintf(g_log, L"i %I64d %d %d\n", p->start / 10000, slice_type, (b >> 5) & 3);
		fflush(g_log);
*/
		int b_frame = slice_type == 1 || slice_type == 6 ? 1 : 0;
		int ref_frame = ((b >> 5) & 3) ? 1 : 0;

		p->flags = (p->flags & 0xff) | (b_frame << 8) | (ref_frame << 9);

		if(b_frame)
		{
			if(!m_h264queue.empty())
			{
				CPacket* p2 = m_h264queue.front();

				std::swap(p->start, p2->start);
				std::swap(p->stop, p2->stop);
				
				m_h264queue.push_back(p);
			}
			else
			{
				ASSERT(0);
			}
		}
		else
		{
			for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++)
			{
				CPacket* a = *i;

				if((a->flags & 0x300) == 0x300)
				{
					auto j = i;

					if(++j != m_h264queue.end())
					{
						CPacket* b = *j;

						if((b->flags & 0x100) != 0)
						{
							std::swap(a->start, b->start);
							std::swap(a->stop, b->stop);
						}
					}
				}
/*
				fwprintf(g_log, L"o %I64d %d\n", a->start / 10000, (a->flags >> 8) & 0xff);
				fflush(g_log);
*/
				a->flags &= 0xff;

				hr = __super::DeliverPacket(a);
			}

			m_h264queue.clear();

			m_h264queue.push_back(p);
		}
	}
	else
	{
		hr = __super::DeliverPacket(p);
	}

	return hr;
}

HRESULT CAVISplitterOutputPin::DeliverEndFlush()
{
	for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++)
	{
		delete *i;
	}

	m_h264queue.clear();

	return __super::DeliverEndFlush();
}
