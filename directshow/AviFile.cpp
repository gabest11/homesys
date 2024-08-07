﻿#include "StdAfx.h"
#include "AVIFile.h"

// CAVIFile

CAVIFile::CAVIFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr)
	, m_idx1(NULL)
{
	if(FAILED(hr)) 
	{
		return;
	}

	hr = Init();
}

CAVIFile::~CAVIFile()
{
	delete [] m_idx1;

	for(auto i = m_strms.begin(); i != m_strms.end(); i++)
	{
		delete *i;
	}
}

HRESULT CAVIFile::Init()
{
	Seek(0);

	DWORD dw[3];

	if(S_OK != Read(dw) || dw[0] != FCC('RIFF') || (dw[2] != FCC('AVI ') && dw[2] != FCC('AVIX')))
	{
		return E_FAIL;
	}

	Seek(0);

	HRESULT hr = Parse(0, GetLength());

	if(m_movis.size() == 0) // FAILED(hr) is allowed as long as there was a movi chunk
	{
		return E_FAIL;
	}

	if(m_avih.dwStreams == 0 && m_strms.size() > 0)
	{
		m_avih.dwStreams = m_strms.size();
	}

	if(m_avih.dwStreams != m_strms.size())
	{
		return E_FAIL;
	}

	for(int i = 0; i < (int)m_avih.dwStreams; i++)
	{
		Stream* s = m_strms[i];

		if(s->strh.fccType == FCC('auds')) 
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)&s->strf[0];

			if(wfe->wFormatTag == 0x55 && wfe->nBlockAlign == 1152 && s->strh.dwScale == 1 && s->strh.dwRate != wfe->nSamplesPerSec)
			{
				// correcting encoder bugs...

				s->strh.dwScale = 1152;
				s->strh.dwRate = wfe->nSamplesPerSec;
			}
		}
	}

	if(FAILED(BuildIndex()))
	{
		EmptyIndex();
	}

	return S_OK;
}

HRESULT CAVIFile::Parse(DWORD parentid, __int64 end)
{
	HRESULT hr = S_OK;

	Stream* strm = NULL;

	while(S_OK == hr && GetPos() < end)
	{
		UINT64 pos = GetPos();

		DWORD id = 0, size;

		if(S_OK != Read(id) || id == 0)
		{
			delete strm;

			return E_FAIL;
		}

		if(id == FCC('RIFF') || id == FCC('LIST'))
		{
			if(S_OK != Read(size) || S_OK != Read(id))
			{
				delete strm;

				return E_FAIL;
			}

			size += (size & 1) + 8;

			if(id == FCC('movi'))
			{
				m_movis.push_back(pos);
			}
			else
			{
				hr = Parse(id, pos + size);
			}
		}
		else
		{
			if(S_OK != Read(size))
			{
				delete strm;

				return E_FAIL;
			}

			if(parentid == FCC('INFO') && size > 0)
			{
				std::wstring s;

				switch(id)
				{
				case FCC('IARL'): // Archival Location. Indicates where the subject of the file is archived.
				case FCC('IART'): // Artist. Lists the artist of the original subject of the file; for example, “Michaelangelo.”
				case FCC('ICMS'): // Commissioned. Lists the name of the person or organization that commissioned the subject of the file; for example, “Pope Julian II.”
				case FCC('ICMT'): // Comments. Provides general comments about the file or the subject of the file. If the comment is several sentences long, end each sentence with a period. Do not include new-line characters.
				case FCC('ICOP'): // Copyright. Records the copyright information for the file; for example, “Copyright Encyclopedia International 1991.” If there are multiple copyrights, separate them by a semicolon followed by a space.
				case FCC('ICRD'): // Creation date. Specifies the date the subject of the file was created. List dates in year-month-day format, padding one-digit months and days with a zero on the left; for example, “1553-05-03” for May 3, 1553.
				case FCC('ICRP'): // Cropped. Describes whether an image has been cropped and, if so, how it was cropped; for example, “lower-right corner.”
				case FCC('IDIM'): // Dimensions. Specifies the size of the original subject of the file; for example, “8.5 in h, 11 in w.”
				case FCC('IDPI'): // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as “300.”
				case FCC('IENG'): // Engineer. Stores the name of the engineer who worked on the file. If there are multiple engineers, separate the names by a semicolon and a blank; for example, “Smith, John; Adams, Joe.”
				case FCC('IGNR'): // Genre. Describes the original work, such as “landscape,” “portrait,” “still life,” etc.
				case FCC('IKEY'): // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon and a blank; for example, “Seattle; aerial view; scenery.”
				case FCC('ILGT'): // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
				case FCC('IMED'): // Medium. Describes the original subject of the file, such as “computer image,” “drawing,” “lithograph,” and so on.
				case FCC('INAM'): // Name. Stores the title of the subject of the file, such as “Seattle From Above.”
				case FCC('IPLT'): // Palette Setting. Specifies the number of colors requested when digitizing an image, such as “256.”
				case FCC('IPRD'): // Product. Specifies the name of the title the file was originally intended for, such as “Encyclopedia of Pacific Northwest Geography.”
				case FCC('ISBJ'): // Subject. Describes the contents of the file, such as “Aerial view of Seattle.”
				case FCC('ISFT'): // Software. Identifies the name of the software package used to create the file, such as “Microsoft WaveEdit.”
				case FCC('ISHP'): // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
				case FCC('ISRC'): // Source. Identifies the name of the person or organization who supplied the original subject of the file; for example, “Trey Research.”
				case FCC('ISRF'): // Source Form. Identifies the original form of the material that was digitized, such as “slide,” “paper,” “map,” and so on. This is not necessarily the same as IMED.
				case FCC('ITCH'): // Technician. Identifies the technician who digitized the subject file; for example, “Smith, John.”
					ReadAString(s, size);
					m_info[id] = s;
					break;
				}
			}

			switch(id)
			{
			case FCC('avih'):
				m_avih.fcc = FCC('avih');
				m_avih.cb = size;
				if(S_OK != Read(m_avih, 8)) {delete strm; return E_FAIL;}
				break;
			case FCC('strh'):
				if(strm == NULL) strm = new Stream();
				strm->strh.fcc = FCC('strh');
				strm->strh.cb = size;
				if(S_OK != Read(strm->strh, 8)) {delete strm; return E_FAIL;}
				break;
			case FCC('strn'):
				if(S_OK != ReadAString(strm->strn, size)) {delete strm; return E_FAIL;}
				break;
			case FCC('strf'):
				if(strm == NULL) strm = new Stream();
				strm->strf.resize(size);
				if(S_OK != ByteRead(strm->strf.data(), size)) {delete strm; return E_FAIL;}
				break;
			case FCC('indx'):
				if(strm == NULL) strm = new Stream();
				ASSERT(strm->indx == NULL);
				strm->indx = (AVISUPERINDEX*)new BYTE[size + 8];
				strm->indx->fcc = FCC('indx');
				strm->indx->cb = size;
				if(S_OK != ByteRead((BYTE*)strm->indx + 8, size)) {delete strm; return E_FAIL;}
				ASSERT(strm->indx->wLongsPerEntry == 4 && strm->indx->bIndexType == AVI_INDEX_OF_INDEXES);
				break;
			case FCC('dmlh'):
				if(S_OK != Read(m_dmlh)) return E_FAIL;
				break;
			case FCC('vprp'):
//				if(S_OK != Read(m_vprp)) return E_FAIL;
				break;
			case FCC('idx1'):
				ASSERT(m_idx1 == NULL);
				m_idx1 = (AVIOLDINDEX*)new BYTE[size + 8];
				m_idx1->fcc = FCC('idx1');
				m_idx1->cb = size;
				if(S_OK != ByteRead((BYTE*)m_idx1 + 8, size)) {delete strm; return E_FAIL;}
				break;
			}

			size += (size & 1) + 8;
		}

		Seek(pos + size);
	}

	if(strm != NULL)
	{
		m_strms.push_back(strm);
	}

	return hr;
}

REFERENCE_TIME CAVIFile::GetTotalTime()
{
	REFERENCE_TIME t = 0; /*10i64 * m_avih.dwMicroSecPerFrame * m_avih.dwTotalFrames*/

	for(int i = 0; i < (int)m_avih.dwStreams; i++)
	{
		Stream* s = m_strms[i];

		REFERENCE_TIME t2 = s->GetRefTime(s->cs.size(), s->totalsize);

		t = max(t, t2);
	}

	if(t == 0) 
	{
		t = 10i64 * m_avih.dwMicroSecPerFrame * m_avih.dwTotalFrames;
	}

	return t;
}

HRESULT CAVIFile::BuildIndex()
{
	EmptyIndex();

	int nSuperIndexes = 0;

	for(int i = 0; i < (int)m_avih.dwStreams; i++)
	{
		Stream* s = m_strms[i];

		if(s->indx && s->indx->nEntriesInUse > 0) 
		{
			nSuperIndexes++;
		}
	}

	if(nSuperIndexes == m_avih.dwStreams)
	{
		for(int i = 0; i < (int)m_avih.dwStreams; i++)
		{
			Stream* s = m_strms[i];

			AVISUPERINDEX* idx = (AVISUPERINDEX*)s->indx;

			DWORD nEntriesInUse = 0;

			for(int j = 0; j < (int)idx->nEntriesInUse; j++)
			{
				Seek(idx->aIndex[j].qwOffset);

				AVISTDINDEX stdidx;

				if(S_OK != ByteRead((BYTE*)&stdidx, FIELD_OFFSET(AVISTDINDEX, aIndex)))
				{
					EmptyIndex();

					return E_FAIL;
				}

				if(stdidx.nEntriesInUse > stdidx.cb / sizeof(stdidx.aIndex[0])) // - 3
				{
					idx->nEntriesInUse = j;

					break;
				}

				nEntriesInUse += stdidx.nEntriesInUse;
			} 

			s->cs.resize(nEntriesInUse);

			DWORD frame = 0;
			UINT64 size = 0;

			for(int j = 0; j < (int)idx->nEntriesInUse; j++)
			{
				Seek(idx->aIndex[j].qwOffset);

				AVISTDINDEX* p = (AVISTDINDEX*)new BYTE[idx->aIndex[j].dwSize];

				if(S_OK != ByteRead((BYTE*)p, idx->aIndex[j].dwSize)) 
				{
					EmptyIndex();

					delete [] p;

					return E_FAIL;
				}

				for(int k = 0, l = 0; k < (int)p->nEntriesInUse; k++)
				{
					s->cs[frame].size = size;
					s->cs[frame].filepos = p->qwBaseOffset + p->aIndex[k].dwOffset;
					s->cs[frame].fKeyFrame = !(p->aIndex[k].dwSize & AVISTDINDEX_DELTAFRAME) || s->strh.fccType == FCC('auds');
					s->cs[frame].fChunkHdr = false;
					s->cs[frame].orgsize = p->aIndex[k].dwSize & AVISTDINDEX_SIZEMASK;

					if(m_idx1 != NULL)
					{
						s->cs[frame].filepos -= 8;
						s->cs[frame].fChunkHdr = true;
					}

					frame++;
					size += s->GetChunkSize(p->aIndex[k].dwSize & AVISTDINDEX_SIZEMASK);
				}

				delete [] p;
			}

			s->totalsize = size;
		}
	}
	else if(m_idx1 != NULL)
	{
		int len = m_idx1->cb / sizeof(m_idx1->aIndex[0]);

		UINT64 offset = m_movis.front() + 8;

		for(int i = 0; i < (int)m_avih.dwStreams; i++)
		{
			Stream* s = m_strms[i];

			int nFrames = 0;

			for(int j = 0; j < len; j++)
			{
				if(TRACKNUM(m_idx1->aIndex[j].dwChunkId) == i)
				{
					nFrames++;
				}
			}

			s->cs.resize(nFrames);

			DWORD frame = 0;
			UINT64 size = 0;

			for(int j = 0, k = 0; j < len; j++)
			{
				DWORD TrackNumber = TRACKNUM(m_idx1->aIndex[j].dwChunkId);

				if(TrackNumber == i)
				{
					if(j == 0 && m_idx1->aIndex[j].dwOffset > offset)
					{
						DWORD id;

						Seek(offset + m_idx1->aIndex[j].dwOffset);
						
						Read(id);

						if(id != m_idx1->aIndex[j].dwChunkId)
						{
							// TRACE(_T("WARNING: CAVIFile::Init() detected absolute chunk addressing in \'idx1\'"));

							offset = 0;
						}
					}

					s->cs[frame].size = size;
					s->cs[frame].filepos = offset + m_idx1->aIndex[j].dwOffset;
					s->cs[frame].fKeyFrame = !!(m_idx1->aIndex[j].dwFlags & AVIIF_KEYFRAME) 
						|| s->strh.fccType == FCC('auds') // FIXME: some audio index is without any kf flag
						|| frame == 0; // grrr
					s->cs[frame].fChunkHdr = j == len - 1 || m_idx1->aIndex[j].dwOffset != m_idx1->aIndex[j + 1].dwOffset;
					s->cs[frame].orgsize = m_idx1->aIndex[j].dwSize;

					frame++;
					size += s->GetChunkSize(m_idx1->aIndex[j].dwSize);
				}
			}

			s->totalsize = size;
		}
	}

	delete [] m_idx1;

	m_idx1 = NULL;

	for(int i = 0; i < (int)m_avih.dwStreams; i++)
	{
		delete [] m_strms[i]->indx;

		m_strms[i]->indx = NULL;
	}

	return S_OK;
}

void CAVIFile::EmptyIndex()
{
	for(int i = 0; i < (int)m_avih.dwStreams; i++)
	{
		m_strms[i]->cs.clear();
		m_strms[i]->totalsize = 0;
	}
}

bool CAVIFile::HasIndex()
{
	for(auto i = m_strms.begin(); i != m_strms.end(); i++)
	{
		if((*i)->cs.size() > 0) 
		{
			return true;
		}
	}

	return false;
}

bool CAVIFile::IsInterleaved(bool keepinfo)
{
	if(m_strms.size() < 2)
	{
		return true;
	}
/*
	if(m_avih.dwFlags&AVIF_ISINTERLEAVED) // not reliable, nandub can write f*cked up files and still sets it
	{
		return true;
	}
*/
	for(int i = 0; i < (int)m_avih.dwStreams; i++)
	{
		m_strms[i]->cs2.resize(m_strms[i]->cs.size());
	}

	DWORD* curchunks = new DWORD[m_avih.dwStreams];
	UINT64* cursizes = new UINT64[m_avih.dwStreams];

	memset(curchunks, 0, sizeof(DWORD) * m_avih.dwStreams);
	memset(cursizes, 0, sizeof(UINT64) * m_avih.dwStreams);

	int end = 0;

	while(1)
	{
		UINT64 fpmin = _I64_MAX;

		DWORD n = -1;

		for(int i = 0; i < (int)m_avih.dwStreams; i++)
		{
			int curchunk = curchunks[i];
			
			std::vector<Stream::chunk>& cs = m_strms[i]->cs;

			if(curchunk >= cs.size()) continue;

            UINT64 fp = cs[curchunk].filepos;

			if(fp < fpmin) 
			{
				fpmin = fp; 
				n = i;
			}
		}

		if(n == -1) break;

		Stream* s = m_strms[n];

		DWORD& curchunk = curchunks[n];
		UINT64& cursize = cursizes[n];

		if(!s->IsRawSubtitleStream())
		{
			Stream::chunk2& cs2 = s->cs2[curchunk];

			cs2.t = (DWORD)(s->GetRefTime(curchunk, cursize) >> 13); // for comparing later it is just as good as /10000 to get a near [ms] accuracy
//			cs2.t = (DWORD)(s->GetRefTime(curchunk, cursize) / 10000);
			cs2.n = end++;
		}

		cursize = s->cs[curchunk].size;
		curchunk++;
	}

	memset(curchunks, 0, sizeof(DWORD) * m_avih.dwStreams);

	Stream::chunk2 cs2last = {-1, 0};

	bool interleaved = true;

	while(interleaved)
	{
		Stream::chunk2 cs2min = {LONG_MAX, LONG_MAX};

		int n = -1;

		for(int i = 0; i < (int)m_avih.dwStreams; i++)
		{
			int curchunk = curchunks[i];

			if(curchunk >= m_strms[i]->cs2.size()) continue;

			Stream::chunk2& cs2 = m_strms[i]->cs2[curchunk];

			if(cs2.t < cs2min.t) 
			{
				cs2min = cs2; 
				n = i;
			}
		}

		if(n == -1) break;

		curchunks[n]++;

		if(cs2last.t >= 0 && abs((int)cs2min.n - (int)cs2last.n) >= 1000)
		{
			interleaved = false;
		}

		cs2last = cs2min;
	}

	delete [] curchunks;
	delete [] cursizes;

	if(interleaved && !keepinfo)
	{
		// this is not needed anymore, let's save a little memory then

		for(int i = 0; i < (int)m_avih.dwStreams; i++)
		{
			m_strms[i]->cs2.clear();
		}
	}

	return interleaved;
}

REFERENCE_TIME CAVIFile::Stream::GetRefTime(DWORD frame, UINT64 size)
{
	float f = (float)frame;

	if(strh.fccType == FCC('auds'))
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)&strf[0];

		f = wfe->nBlockAlign ? 1.0f * size / wfe->nBlockAlign : 0;
	}

	float scale_per_rate = strh.dwRate ? 1.0f * strh.dwScale / strh.dwRate : 0;

	return (REFERENCE_TIME)(scale_per_rate * f * 10000000 + 0.5f);
}

int CAVIFile::Stream::GetFrame(REFERENCE_TIME rt)
{
	int i = -1;

	float rate_per_scale = strh.dwScale ? 1.0f * strh.dwRate / strh.dwScale : 0;

	if(strh.fccType == FCC('auds'))
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)&strf[0];

		__int64 size = (__int64)(rate_per_scale * wfe->nBlockAlign * rt / 10000000 + 0.5f);

		for(i = 0; i < cs.size(); i++)
		{
			if(cs[i].size > size)
			{
				i--;

				break;
			}
		}
	}
	else
	{
		i = (int)(rate_per_scale * rt / 10000000 + 0.5f);
	}

	if(i >= cs.size())
	{
		i = cs.size() - 1;
	}

	return i;
}

int CAVIFile::Stream::GetKeyFrame(REFERENCE_TIME rt)
{
	int i = GetFrame(rt);

	for(; i > 0; i--) 
	{
		if(cs[i].fKeyFrame) 
		{
			break;
		}
	}
	
	return i;
}

DWORD CAVIFile::Stream::GetChunkSize(DWORD size)
{
	if(strh.fccType == FCC('auds'))
	{
		WORD nBlockAlign = ((WAVEFORMATEX*)&strf[0])->nBlockAlign;

		size = nBlockAlign ? (size + (nBlockAlign - 1)) / nBlockAlign * nBlockAlign : 0; // round up for nando's vbr hack
	}

	return size;
}

bool CAVIFile::Stream::IsRawSubtitleStream()
{
	return strh.fccType == FCC('txts') && cs.size() == 1;
}
