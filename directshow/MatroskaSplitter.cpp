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
#include "MatroskaSplitter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

using namespace MatroskaReader;

// CMatroskaSplitterFilter

CMatroskaSplitterFilter::CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMatroskaSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
	, m_segment(NULL)
	, m_cluster(NULL)
	, m_block(NULL)	
{
}

CMatroskaSplitterFilter::~CMatroskaSplitterFilter()
{
	delete m_block;
	delete m_cluster;
	delete m_segment;
	delete m_file;
}

STDMETHODIMP CMatroskaSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMatroskaSplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	delete m_file;

	m_tracks.clear();

	m_file = new CMatroskaReaderFile(pAsyncReader, hr);

	if(FAILED(hr))
	{
		delete m_file;

		m_file = NULL;
		
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	Info& info = m_file->m_segment.SegmentInfo;

	SetProperty(L"TITL", info.Title.c_str());

	int iVideo = 1, iAudio = 1, iSubtitle = 1;

	for(auto i = m_file->m_segment.Tracks.begin(); i != m_file->m_segment.Tracks.end(); i++)
	{
		Track* pT = *i;

		for(auto i = pT->TrackEntries.begin(); i != pT->TrackEntries.end(); i++)
		{
			TrackEntry* pTE = *i;

			if(!pTE->Expand(pTE->CodecPrivate, ContentEncoding::TracksPrivateData))
			{
				continue;
			}

			std::string CodecID = pTE->CodecID.ToString();

			std::wstring name = Util::Format(L"Output %I64d", (UINT64)pTE->TrackNumber);

			CMediaType mt;

			std::vector<CMediaType> mts;

			mt.SetSampleSize(1);

			if(pTE->TrackType == TrackEntry::TypeVideo)
			{
				name = Util::Format(L"Video %d", iVideo++);

				mt.majortype = MEDIATYPE_Video;

				if(CodecID == "V_MS/VFW/FOURCC")
				{
					mt.formattype = FORMAT_VideoInfo;

					VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.size() - sizeof(BITMAPINFOHEADER));

					memset(mt.Format(), 0, mt.FormatLength());

					memcpy(&vih->bmiHeader, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					mt.subtype = FOURCCMap(vih->bmiHeader.biCompression);

					switch(vih->bmiHeader.biCompression)
					{
					case BI_RGB: 
					case BI_BITFIELDS: 
						mt.subtype = 
							vih->bmiHeader.biBitCount == 1 ? MEDIASUBTYPE_RGB1 :
							vih->bmiHeader.biBitCount == 4 ? MEDIASUBTYPE_RGB4 :
							vih->bmiHeader.biBitCount == 8 ? MEDIASUBTYPE_RGB8 :
							vih->bmiHeader.biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
							vih->bmiHeader.biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
							vih->bmiHeader.biBitCount == 32 ? MEDIASUBTYPE_ARGB32 :
							MEDIASUBTYPE_NULL;
						break;
//					case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
//					case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
					}
					
					mts.push_back(mt);
				}
				else if(CodecID == "V_UNCOMPRESSED")
				{
				}
				else if(CodecID.find("V_MPEG4/ISO/AVC") == 0 && pTE->CodecPrivate.size() >= 6)
				{
					// TODO

					std::vector<BYTE> avcC = pTE->CodecPrivate;

					BYTE sps = avcC[5] & 0x1f;

					std::vector<BYTE> data;

					int i = 6;

					while(sps--)
					{
						if(i + 2 > avcC.size())
							goto avcfail;
						
						int spslen = ((int)avcC[i] << 8) | avcC[i + 1];
						
						if(i + 2 + spslen > avcC.size())
							goto avcfail;
						
						int cur = data.size();
						
						data.resize(cur + spslen + 2, 0);
						
						std::copy(avcC.begin() + i, avcC.begin() + i + 2 + spslen, data.begin() + cur);
						
						i += 2 + spslen;
					}

					if(i + 1 > avcC.size())
						continue;

					int pps = avcC[i++];
					
					while(pps--)
					{
						if(i + 2 > avcC.size())
							goto avcfail;
						
						int ppslen = ((int)avcC[i] << 8) | avcC[i + 1];
						
						if(i + 2 + ppslen > avcC.size())
							goto avcfail;
						
						int cur = data.size();
						
						data.resize(cur + ppslen + 2, 0);
						
						std::copy(avcC.begin() + i, avcC.begin() + i + 2 + ppslen, data.begin() + cur);
						
						i += 2 + ppslen;
					}
					
					goto avcsuccess;
avcfail:
					continue;
avcsuccess:

					mt.subtype = FOURCCMap('1CVA');
					mt.formattype = FORMAT_MPEG2Video;

					MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + data.size());

					memset(mt.Format(), 0, mt.FormatLength());

					vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
					vih->hdr.bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
					vih->hdr.bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
					vih->hdr.bmiHeader.biCompression = '1CVA';
					vih->hdr.bmiHeader.biPlanes = 1;
					vih->hdr.bmiHeader.biBitCount = 24;

					vih->dwProfile = pTE->CodecPrivate[1];
					vih->dwLevel = pTE->CodecPrivate[3];
					vih->dwFlags = (pTE->CodecPrivate[4] & 3) + 1;

					vih->cbSequenceHeader = data.size();

					memcpy(vih->dwSequenceHeader, &data[0], data.size());

					mts.push_back(mt);
				}
				else if(CodecID.find("V_MPEG4/") == 0)
				{
					mt.subtype = FOURCCMap('V4PM');
					mt.formattype = FORMAT_MPEG2Video;

					MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + pTE->CodecPrivate.size());

					memset(mt.Format(), 0, mt.FormatLength());

					vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
					vih->hdr.bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
					vih->hdr.bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
					vih->hdr.bmiHeader.biCompression = 'V4PM';
					vih->hdr.bmiHeader.biPlanes = 1;
					vih->hdr.bmiHeader.biBitCount = 24;

					vih->cbSequenceHeader = pTE->CodecPrivate.size();

					memcpy((BYTE*)vih->dwSequenceHeader, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					
					mts.push_back(mt);
				}
				else if(CodecID.find("V_REAL/RV") == 0)
				{
					mt.subtype = FOURCCMap('00VR' + ((CodecID[9] - 0x30) << 16));
					mt.formattype = FORMAT_VideoInfo;

					VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.size());
					
					memset(mt.Format(), 0, mt.FormatLength());
					memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					
					vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
					vih->bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
					vih->bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
					vih->bmiHeader.biCompression = mt.subtype.Data1;
					
					mts.push_back(mt);
				}
				else if(CodecID == "V_DIRAC")
				{
					mt.subtype = MEDIASUBTYPE_DiracVideo;
					mt.formattype = FORMAT_DiracVideoInfo;

					DIRACINFOHEADER* vih = (DIRACINFOHEADER*)mt.AllocFormatBuffer(FIELD_OFFSET(DIRACINFOHEADER, dwSequenceHeader) + pTE->CodecPrivate.size());

					memset(mt.Format(), 0, mt.FormatLength());

					vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
					vih->hdr.bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
					vih->hdr.bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
					vih->hdr.dwPictAspectRatioX = vih->hdr.bmiHeader.biWidth;
					vih->hdr.dwPictAspectRatioY = vih->hdr.bmiHeader.biHeight;

					vih->cbSequenceHeader = pTE->CodecPrivate.size();

					memcpy((BYTE*)vih->dwSequenceHeader, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					mts.push_back(mt);
				}
				else if(CodecID == "V_MPEG2")
				{
					int w = pTE->v.PixelWidth;
					int h = pTE->v.PixelHeight;

					if(DirectShow::Mpeg2InitData(mt, pTE->CodecPrivate.data(), pTE->CodecPrivate.size(), w, h))
					{
						mts.push_back(mt);
					}
				}
/*
				else if(CodecID == "V_DSHOW/MPEG1VIDEO") // V_MPEG1
				{
					mt.majortype = MEDIATYPE_Video;
					mt.subtype = MEDIASUBTYPE_MPEG1Payload;
					mt.formattype = FORMAT_MPEGVideo;

					MPEG1VIDEOINFO* vih = (MPEG1VIDEOINFO*)mt.AllocFormatBuffer(pTE->CodecPrivate.size());
					
					memcpy(vih, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					
					mt.SetSampleSize(vih->hdr.bmiHeader.biWidth * vih->hdr.bmiHeader.biHeight * 4);
					
					mts.push_back(mt);
				}
*/
				REFERENCE_TIME AvgTimePerFrame = 0;

                if(pTE->v.FramePerSec > 0)
				{
					AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 / pTE->v.FramePerSec);
				}
				else if(pTE->DefaultDuration > 0)
				{
					AvgTimePerFrame = (REFERENCE_TIME)pTE->DefaultDuration / 100;
				}

				if(AvgTimePerFrame)
				{
					for(auto i = mts.begin(); i != mts.end(); i++)
					{
						CMediaType& mt = *i;

						if(mt.formattype == FORMAT_VideoInfo
						|| mt.formattype == FORMAT_VideoInfo2
						|| mt.formattype == FORMAT_MPEG2Video)
						{
							((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame = AvgTimePerFrame;
						}
					}
				}

				if(pTE->v.DisplayWidth != 0 && pTE->v.DisplayHeight != 0)
				{
					for(auto i = mts.begin(); i != mts.end(); i++)
					{
						if(i->formattype == FORMAT_VideoInfo)
						{
							DWORD vih1 = FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
							DWORD vih2 = FIELD_OFFSET(VIDEOINFOHEADER2, bmiHeader);
							DWORD bmi = i->FormatLength() - FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
							mt.formattype = FORMAT_VideoInfo2;
							mt.AllocFormatBuffer(vih2 + bmi);
							memcpy(mt.Format(), i->Format(), vih1);
							memset(mt.Format() + vih1, 0, vih2 - vih1);
							memcpy(mt.Format() + vih2, i->Format() + vih1, bmi);
							((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioX = (DWORD)pTE->v.DisplayWidth;
							((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioY = (DWORD)pTE->v.DisplayHeight;
							i = mts.insert(i, mt);
							i++;
						}
						else if(i->formattype == FORMAT_MPEG2Video)
						{
							((MPEG2VIDEOINFO*)i->Format())->hdr.dwPictAspectRatioX = (DWORD)pTE->v.DisplayWidth;
							((MPEG2VIDEOINFO*)i->Format())->hdr.dwPictAspectRatioY = (DWORD)pTE->v.DisplayHeight;
						}
					}
				}
			}
			else if(pTE->TrackType == TrackEntry::TypeAudio)
			{
				name = Util::Format(L"Audio %d", iAudio++);

				mt.majortype = MEDIATYPE_Audio;
				mt.formattype = FORMAT_WaveFormatEx;

				WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));

				memset(wfe, 0, mt.FormatLength());

				wfe->nChannels = (WORD)pTE->a.Channels;
				wfe->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
				wfe->wBitsPerSample = (WORD)pTE->a.BitDepth;
				wfe->nBlockAlign = (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);
				wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;
				
				mt.SetSampleSize(256000);

				static std::map<std::string, int> id2fmt;

				if(id2fmt.empty())
				{
					id2fmt["A_MPEG/L3"] = WAVE_FORMAT_MP3;
					id2fmt["A_MPEG/L2"] = WAVE_FORMAT_MPEG;
					id2fmt["A_AC3"] = WAVE_FORMAT_DOLBY_AC3;
					id2fmt["A_DTS"] = WAVE_FORMAT_DVD_DTS;
					id2fmt["A_PCM/INT/LIT"] = WAVE_FORMAT_PCM;
					id2fmt["A_PCM/FLOAT/IEEE"] = WAVE_FORMAT_IEEE_FLOAT;
					id2fmt["A_AAC"] = -WAVE_FORMAT_AAC;
					id2fmt["A_FLAC"] = -WAVE_FORMAT_FLAC;
					id2fmt["A_WAVPACK4"] = -WAVE_FORMAT_WAVPACK4;
					id2fmt["A_TTA1"] = WAVE_FORMAT_TTA1;
				}

				auto i = id2fmt.find(CodecID);

				if(i != id2fmt.end())
				{
					int wFormatTag = i->second;

					if(wFormatTag < 0)
					{
						wFormatTag = -wFormatTag;
						wfe->cbSize = pTE->CodecPrivate.size();
						wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
						memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					}

					wfe->wFormatTag = wFormatTag;

					mt.subtype = FOURCCMap(wFormatTag);

					mts.push_back(mt);

					if(wFormatTag == WAVE_FORMAT_FLAC)
					{
						mt.subtype = MEDIASUBTYPE_FLAC_FRAMED;

						mts.insert(mts.begin(), mt);
					}
				}
				else if(CodecID == "A_VORBIS")
				{
					BYTE* src = pTE->CodecPrivate.data();

					std::vector<int> sizes;

					int total = 0;

					for(BYTE n = *src++; n > 0; n--)
					{
						int size = 0;

						do {size += *src;} while(*src++ == 0xff);

						sizes.push_back(size);

						total += size;
					}

					sizes.push_back(pTE->CodecPrivate.size() - (src - pTE->CodecPrivate.data()) - total);

					total += sizes.back();

					if(sizes.size() == 3)
					{
						mt.subtype = MEDIASUBTYPE_Vorbis2;
						mt.formattype = FORMAT_VorbisFormat2;

						VORBISFORMAT2* wfe = (VORBISFORMAT2*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT2) + total);
						
						memset(wfe, 0, mt.FormatLength());

						wfe->Channels = (WORD)pTE->a.Channels;
						wfe->SamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
						wfe->BitsPerSample = (DWORD)pTE->a.BitDepth;

						BYTE* dst = mt.Format() + sizeof(VORBISFORMAT2);

						for(int i = 0; i < sizes.size(); i++)
						{
							int size = sizes[i];
							
							wfe->HeaderSize[i] = size;
							
							memcpy(dst, src, size);
							
							src += size;
							dst += size;
						}

						mts.push_back(mt);
					}

					mt.subtype = MEDIASUBTYPE_Vorbis;
					mt.formattype = FORMAT_VorbisFormat;

					VORBISFORMAT* wfe = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));

					memset(wfe, 0, mt.FormatLength());

					wfe->nChannels = (WORD)pTE->a.Channels;
					wfe->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
					wfe->nMinBitsPerSec = wfe->nMaxBitsPerSec = wfe->nAvgBitsPerSec = -1;

					mts.push_back(mt);
				}
				else if(CodecID == "A_MS/ACM")
				{
					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(pTE->CodecPrivate.size());

					memcpy(wfe, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					
					mt.subtype = FOURCCMap(wfe->wFormatTag);
					
					mts.push_back(mt);
				}
				else if(CodecID.find("A_AAC/") == 0)
				{
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_AAC);

					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 5);

					wfe->cbSize = 2;

					int profile;

					if(CodecID.find("/MAIN") > 0) profile = 0;
					else if(CodecID.find("/SBR") > 0) profile = -1;
					else if(CodecID.find("/LC") > 0) profile = 1;
					else if(CodecID.find("/SSR") > 0) profile = 2;
					else if(CodecID.find("/LTP") > 0) profile = 3;
					else continue;

					WORD cbSize = DirectShow::AACInitData((BYTE*)(wfe + 1), profile, wfe->nSamplesPerSec, pTE->a.Channels);

					mts.push_back(mt);

					if(profile < 0)
					{
						wfe->cbSize = cbSize;
						wfe->nSamplesPerSec *= 2;
						wfe->nAvgBytesPerSec *= 2;

						mts.insert(mts.begin(), mt);
					}
				}
				else if(CodecID.find("A_REAL/") == 0 && CodecID.size() >= 11)
				{
					DWORD fcc = (DWORD)CodecID[7] | ((DWORD)CodecID[8] << 8) | ((DWORD)CodecID[9] << 16) | ((DWORD)CodecID[10] << 24);

					mt.subtype = FOURCCMap(fcc);
					mt.bTemporalCompression = TRUE;

					wfe->cbSize = pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());

					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					wfe->cbSize = 0; // IMPORTANT: this is screwed, but cbSize has to be 0 and the extra data from codec priv must be after WAVEFORMATEX

					mts.push_back(mt);
				}
			}
			else if(pTE->TrackType == TrackEntry::TypeSubtitle)
			{
				if(iSubtitle == 1) InstallFonts();

				name = Util::Format(L"Subtitle %d", iSubtitle++);

				mt.SetSampleSize(1);

				if(CodecID == "S_TEXT/ASCII")
				{
					mt.majortype = MEDIATYPE_Text;
					mt.subtype = MEDIASUBTYPE_NULL;
					mt.formattype = FORMAT_None;

					mts.push_back(mt);
				}
				else
				{
					mt.majortype = MEDIATYPE_Subtitle;
					mt.formattype = FORMAT_SubtitleInfo;

					SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + pTE->CodecPrivate.size());
					
					memset(psi, 0, mt.FormatLength());
					
					strncpy(psi->IsoLang, pTE->Language.c_str(), countof(psi->IsoLang) - 1);
					wcsncpy(psi->TrackName, pTE->Name.c_str(), countof(psi->TrackName) - 1);
					
					memcpy(mt.pbFormat + (psi->dwOffset = sizeof(SUBTITLEINFO)), pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					mt.subtype = 
						CodecID == "S_TEXT/UTF8" ? MEDIASUBTYPE_UTF8 :
						CodecID == "S_TEXT/SSA" || CodecID == "S_SSA" ? MEDIASUBTYPE_SSA :
						CodecID == "S_TEXT/ASS" || CodecID == "S_ASS" ? MEDIASUBTYPE_ASS :
						CodecID == "S_TEXT/SSF" || CodecID == "S_SSF" ? MEDIASUBTYPE_SSF :
						CodecID == "S_TEXT/USF" || CodecID == "S_USF" ? MEDIASUBTYPE_USF :
						CodecID == "S_VOBSUB" ? MEDIASUBTYPE_VOBSUB :
						MEDIASUBTYPE_NULL;

					if(mt.subtype != MEDIASUBTYPE_NULL)
					{
						mts.push_back(mt);
					}
				}
			}

			if(mts.empty())
			{
				continue;
			}

			std::wstring lang(pTE->Language.begin(), pTE->Language.end());

			name = (!lang.empty() ? Util::ISO6392ToLanguage(lang.c_str()) : L"English")
				+ (!pTE->Name.empty() ? L", " + pTE->Name : L"")
				+ (L" (" + name + L")");

			HRESULT hr;

			CBaseSplitterOutputPin* pin = new CMatroskaSplitterOutputPin(pTE->DefaultDuration / 100, mts, name.c_str(), this, &hr);
			
			if(!pTE->Name.empty())
			{
				pin->SetProperty(L"NAME", pTE->Name.c_str());

				if(pTE->TrackType == TrackEntry::TypeVideo)
				{
					if(!info.Title.empty())
					{
						SetProperty(L"TITL", pTE->Name.c_str());
					}
				}

			}

			if(lang.size() == 3)
			{
				pin->SetProperty(L"LANG", lang.c_str());
			}

			AddOutputPin((DWORD)pTE->TrackNumber, pin);

			m_tracks[(DWORD)pTE->TrackNumber] = pTE;
		}
	}

	m_rtDuration = (REFERENCE_TIME)(info.Duration * info.TimeCodeScale / 100);

	m_rtNewStop = m_rtStop = m_rtDuration;

	// resources

	for(auto i = m_file->m_segment.Attachments.begin(); i != m_file->m_segment.Attachments.end(); i++)
	{
		Attachment* pA = *i;

		for(auto i = pA->AttachedFiles.begin(); i != pA->AttachedFiles.end(); i++)
		{
			AttachedFile* pF = *i;

			std::vector<BYTE> data;

			data.resize(pF->FileDataLen);

			m_file->Seek(pF->FileDataPos);

			if(SUCCEEDED(m_file->ByteRead(&data[0], data.size())))
			{
				std::wstring mime(pF->FileMimeType.begin(), pF->FileMimeType.end());

				ResAppend(pF->FileName.c_str(), pF->FileDescription.c_str(), mime.c_str(), &data[0], data.size());
			}
		}
	}

	// chapters

	if(ChapterAtom* caroot = m_file->m_segment.FindChapterAtom(0))
	{
		wchar_t buff[3] = {0};

		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, buff, countof(buff));

		std::wstring ChapLanguage = Util::ISO6391To6392(buff);

		if(ChapLanguage.size() < 3) ChapLanguage = L"eng";

		SetupChapters(ChapLanguage.c_str(), caroot);
	}

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

void CMatroskaSplitterFilter::SetupChapters(LPCWSTR lng, ChapterAtom* parent, int level)
{
	std::wstring tabs(level, '+');

	if(!tabs.empty()) 
	{
		tabs += ' ';
	}

	for(auto i = parent->ChapterAtoms.begin(); i != parent->ChapterAtoms.end(); i++)
	{
		if(ChapterAtom* ca = m_file->m_segment.FindChapterAtom((*i)->ChapterUID))
		{
			std::wstring name, first;

			for(auto i = ca->ChapterDisplays.begin(); i != ca->ChapterDisplays.end(); i++)
			{
				ChapterDisplay* cd = *i;

				if(first.empty()) 
				{
					first = cd->ChapString;
				}

				std::wstring cl(cd->ChapLanguage.begin(), cd->ChapLanguage.end());

				if(cl == lng) 
				{
					name = cd->ChapString;
				}
			}

			name = tabs + (!name.empty() ? name : first);

			ChapAppend(ca->ChapterTimeStart / 100 - m_file->m_start, name.c_str());

			if(!ca->ChapterAtoms.empty())
			{
				SetupChapters(lng, ca, level + 1);
			}
		}			
	}
}

void CMatroskaSplitterFilter::InstallFonts()
{
	for(auto i = m_file->m_segment.Attachments.begin(); i != m_file->m_segment.Attachments.end(); i++)
	{
		Attachment* pA = *i;

		for(auto i = pA->AttachedFiles.begin(); i != pA->AttachedFiles.end(); i++)
		{
			AttachedFile* pF = *i;

			if(pF->FileMimeType == "application/x-truetype-font")
			{
				// assume this is a font resource

				if(BYTE* data = new BYTE[(UINT)pF->FileDataLen])
				{
					m_file->Seek(pF->FileDataPos);

					if(SUCCEEDED(m_file->ByteRead(data, pF->FileDataLen)))
					{
						m_font.Install(data, (UINT)pF->FileDataLen);
					}

					delete [] data;
				}
			}
		}
	}
}

void CMatroskaSplitterFilter::SendVorbisHeaderSample()
{
	HRESULT hr;

	for(auto i = m_tracks.begin(); i != m_tracks.end(); i++)
	{
		TrackEntry* pTE = i->second;

		CBaseSplitterOutputPin* pin = GetOutputPin(i->first);

		if(pTE == NULL || pin == NULL || !pin->IsConnected())
		{
			continue;
		}

		const CMediaType& mt = pin->CurrentMediaType();

		if(pTE->CodecID.ToString() == "A_VORBIS" && mt.subtype == MEDIASUBTYPE_Vorbis && !pTE->CodecPrivate.empty())
		{
			BYTE* ptr = pTE->CodecPrivate.data();

			std::list<int> sizes;

			int total = 0;

			for(BYTE n = *ptr++; n > 0; n--)
			{
				int size = 0;
				
				do {size += *ptr;} while(*ptr++ == 0xff);
				
				sizes.push_back(size);

				total += size;
			}

			sizes.push_back(pTE->CodecPrivate.size() - (ptr - pTE->CodecPrivate.data()) - total);

			hr = S_OK;

			for(auto i = sizes.begin(); i != sizes.end(); i++)
			{
				long len = *i;

				CPacket* p = new CPacket();

				p->id = (DWORD)pTE->TrackNumber;

				p->start = 0; 
				p->stop = 1;
				
				p->flags |= CPacket::TimeValid;
				
				p->SetData(ptr, len);

				ptr += len;
				
				hr = DeliverPacket(p);
			}
		}
	}
}

bool CMatroskaSplitterFilter::DemuxInit()
{
	CMatroskaNode root(m_file);

	m_segment = root.Child(0x18538067);

	if(m_segment == NULL)
	{
		return false;
	}

	m_cluster = m_segment->Child(0x1F43B675);

	if(m_cluster == NULL)
	{
		return false;
	}

	// reindex if needed

	if(m_file->m_segment.Cues.empty())
	{
		m_nOpenProgress = 0;

		m_file->m_segment.SegmentInfo.Duration.Set(0);

		UINT64 id = m_file->m_segment.GetMasterTrack();

		Cue* cue = new Cue();

		do
		{
			Cluster c;

			c.ParseTimeCode(m_cluster);

			m_file->m_segment.SegmentInfo.Duration.Set((float)c.TimeCode - m_file->m_start / 10000);

			CueTrackPosition* ctp = new CueTrackPosition();

			ctp->CueTrack.Set(id);
			ctp->CueClusterPosition.Set(m_cluster->m_filepos - m_segment->m_start);

			CuePoint* cp = new CuePoint();

			cp->CueTime.Set(c.TimeCode);
			cp->CueTrackPositions.push_back(ctp);

			cue->CuePoints.push_back(cp);

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
		while(!m_abort && m_cluster->Next(true));

		m_nOpenProgress = 100;

		if(!m_abort)
		{
			m_file->m_segment.Cues.push_back(cue);
		}
		else
		{
			delete cue;
		}

		m_abort = false;
	}

	delete m_cluster;
	delete m_block;

	m_cluster = NULL;
	m_block = NULL;

	return true;
}

void CMatroskaSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	delete m_cluster;

	m_cluster = m_segment->Child(0x1F43B675);
	
	delete m_block;

	m_block = NULL;

	if(rt <= 0) return;

	rt += m_file->m_start;

	UINT64 lastCueClusterPosition = -1;

	Segment& s = m_file->m_segment;

	UINT64 id = s.GetMasterTrack();

	for(auto i = s.Cues.begin(); i != s.Cues.end(); i++)
	{
		Cue* cue = *i;

		for(auto i = cue->CuePoints.rbegin(); i != cue->CuePoints.rend(); i++)
		{
			CuePoint* cp = *i;

			if((UINT64)cp->CueTime >= 0xffffffffffui64 || rt < s.GetRefTime(cp->CueTime))
			{
				continue;
			}

			for(auto i = cp->CueTrackPositions.begin(); i != cp->CueTrackPositions.end(); i++)
			{
				CueTrackPosition* ctp = *i;

				if(id != ctp->CueTrack || lastCueClusterPosition == ctp->CueClusterPosition)
				{
					continue;
				}

				lastCueClusterPosition = ctp->CueClusterPosition;

				m_cluster->SeekTo(m_segment->m_start + ctp->CueClusterPosition);

				m_cluster->Parse();

				bool fFoundKeyFrame = false;

				Cluster c;

				c.ParseTimeCode(m_cluster);

				CMatroskaNode* block = m_cluster->GetFirstBlock();

				if(block != NULL)
				{
					bool fPassedCueTime = false;

					do
					{
						CBlockGroupNode groups;

						if(block->m_id == 0xA0)
						{
							groups.Parse(block, true);
						}
						else if(block->m_id == 0xA3)
						{
							BlockGroup* group = new BlockGroup();

							group->Block.Parse(block, true);

							if((group->Block.Lacing & 0x80) == 0) 
							{
								group->ReferenceBlock.Set(0); // not a kf
							}

							groups.push_back(group);
						}

						for(auto i = groups.begin(); i != groups.end() && !fPassedCueTime; i++)
						{
							BlockGroup* group = *i;

							if(group->Block.TrackNumber == ctp->CueTrack && rt < s.GetRefTime(c.TimeCode + group->Block.TimeCode)
							|| rt + 5000000i64 < s.GetRefTime(c.TimeCode + group->Block.TimeCode)) // allow 500ms difference between tracks, just in case intreleaving wasn't that much precise
							{
								fPassedCueTime = true;
							}
							else if(group->Block.TrackNumber == ctp->CueTrack && !group->ReferenceBlock.IsValid())
							{
								fFoundKeyFrame = true;

								delete m_block;

								m_block = block->Copy();
							}
						}
					}
					while(!fPassedCueTime && block->NextBlock());

					delete block;
				}

				if(fFoundKeyFrame)
				{
					return;
				}
			}
		}
	}

	DemuxSeek(0);
}

bool CMatroskaSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;
	
	SendVorbisHeaderSample(); // HACK: init vorbis decoder with the headers

	do
	{
		Cluster c;

		c.ParseTimeCode(m_cluster);

		if(m_block == NULL)
		{
			m_block = m_cluster->GetFirstBlock();

			if(m_block == NULL)
			{
				continue;
			}
		}

		do
		{
			CBlockGroupNode groups;

			if(m_block->m_id == 0xA0)
			{
				groups.Parse(m_block, true);
			}
			else if(m_block->m_id == 0xA3)
			{
				BlockGroup* group = new BlockGroup();

				group->Block.Parse(m_block, true);

				if((group->Block.Lacing & 0x80) == NULL)
				{
					group->ReferenceBlock.Set(0); // not a kf
				}

				groups.push_back(group);
			}

			while(!groups.empty() && SUCCEEDED(hr))
			{
				MatroskaPacket* p = new MatroskaPacket();

				p->group = groups.front();

				groups.pop_front();

				p->id = (DWORD)p->group->Block.TrackNumber;

				auto i = m_tracks.find(p->id);

				if(i == m_tracks.end())
				{
					delete p;

					continue;
				}

				TrackEntry* pTE = i->second;

				p->start = m_file->m_segment.GetRefTime((REFERENCE_TIME)c.TimeCode + p->group->Block.TimeCode);
				p->stop = p->start + (p->group->BlockDuration.IsValid() ? m_file->m_segment.GetRefTime(p->group->BlockDuration) : 1);

				p->start -= m_file->m_start;
				p->stop -= m_file->m_start;

				p->flags |= CPacket::TimeValid;

				if(!p->group->ReferenceBlock.IsValid())
				{
					p->flags |= CPacket::SyncPoint;
				}

				if(pTE->TrackType == TrackEntry::TypeSubtitle && !p->group->BlockDuration.IsValid())
				{
					// fix subtitle with duration = 0

					p->group->BlockDuration.Set(1); // just setting it to be valid
					p->stop = p->start;
				}

				for(auto i = p->group->Block.BlockData.begin(); i != p->group->Block.BlockData.end(); i++)
				{
					pTE->Expand(*(*i), ContentEncoding::AllFrameContents);
				}

				hr = DeliverPacket(p);
			}
		}
		while(m_block->NextBlock() && SUCCEEDED(hr) && !CheckRequest(NULL));

		delete m_block;

		m_block = NULL;
	}
	while(m_file->GetPos() < m_file->m_segment.pos + m_file->m_segment.len && m_cluster->Next(true) && SUCCEEDED(hr) && !CheckRequest(NULL));

	delete m_cluster;

	m_cluster = NULL;

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CMatroskaSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_file, E_UNEXPECTED);

	HRESULT hr = S_OK;

	nKFs = 0;

	for(auto i = m_file->m_segment.Cues.begin(); i != m_file->m_segment.Cues.end(); i++)
	{
		nKFs += (*i)->CuePoints.size();
	}

	return hr;
}

STDMETHODIMP CMatroskaSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_file, E_UNEXPECTED);

	if(*pFormat != TIME_FORMAT_MEDIA_TIME) 
	{
		return E_INVALIDARG;
	}

	UINT nKFsTmp = 0;

	for(auto i = m_file->m_segment.Cues.begin(); i != m_file->m_segment.Cues.end() && nKFsTmp < nKFs; i++)
	{
		Cue* cue = *i;

		for(auto i = cue->CuePoints.begin(); i != cue->CuePoints.end() && nKFsTmp < nKFs; i++)
		{
			pKFs[nKFsTmp++] = m_file->m_segment.GetRefTime((*i)->CueTime);
		}
	}

	nKFs = nKFsTmp;

	return S_OK;
}

// CMatroskaSourceFilter

CMatroskaSourceFilter::CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMatroskaSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}

// CMatroskaSplitterOutputPin

CMatroskaSplitterOutputPin::CMatroskaSplitterOutputPin(REFERENCE_TIME atpf, const std::vector<CMediaType>& mts, LPCWSTR pName, CBaseSplitterFilter* pFilter, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, phr)
	, m_atpf(atpf)
{
}

CMatroskaSplitterOutputPin::~CMatroskaSplitterOutputPin()
{
	for(auto i = m_packets.begin(); i != m_packets.end(); i++)
	{
		delete *i;
	}
}

HRESULT CMatroskaSplitterOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(&m_csQueue);

		for(auto i = m_packets.begin(); i != m_packets.end(); i++)
		{
			delete *i;
		}

		m_packets.clear();
	}

	return __super::DeliverEndFlush();
}

HRESULT CMatroskaSplitterOutputPin::DeliverEndOfStream()
{
	CAutoLock cAutoLock(&m_csQueue);

	// send out the last remaining packets from the queue

	while(!m_packets.empty())
	{
		HRESULT hr = DeliverBlock(m_packets.front());

		m_packets.pop_front();

		if(hr != S_OK) 
		{
			return hr;
		}
	}

	return __super::DeliverEndOfStream();
}

HRESULT CMatroskaSplitterOutputPin::DeliverPacket(CPacket* p)
{
	MatroskaPacket* mp = dynamic_cast<MatroskaPacket*>(p);

	if(mp == NULL)
	{
		return __super::DeliverPacket(p);
	}

	CAutoLock cAutoLock(&m_csQueue);

	m_packets.push_back(mp);

	if(m_packets.size() == 2)
	{
		MatroskaPacket* mp1 = m_packets.front();
		MatroskaPacket* mp2 = m_packets.back();

		if(!mp1->group->BlockDuration.IsValid())
		{
			mp1->group->BlockDuration.Set(1); // just to set it valid

			if(mp1->start < mp2->start)
			{
				mp1->stop = mp2->start;
			}
		}
	}

	while(!m_packets.empty())
	{
		mp = m_packets.front();

		if(!mp->group->BlockDuration.IsValid()) break;

		m_packets.pop_front();

		HRESULT hr = DeliverBlock(mp);

		if(hr != S_OK) 
		{
			return hr;
		}
	}

	return S_OK;
}

HRESULT CMatroskaSplitterOutputPin::DeliverBlock(MatroskaPacket* mp)
{
	HRESULT hr = S_FALSE;

	REFERENCE_TIME delta = (mp->stop - mp->start) / mp->group->Block.BlockData.size();
	REFERENCE_TIME start = mp->start;
	REFERENCE_TIME stop = mp->start + delta;

	for(auto i = mp->group->Block.BlockData.begin(); i != mp->group->Block.BlockData.end(); i++)
	{
		CPacket* p = new CPacket();

		p->id = mp->id;
		p->start = start;
		p->stop = stop;
		p->flags = mp->flags;
		p->buff.swap(*(*i));

		hr = DeliverPacket(p);

		if(S_OK != hr) 
		{
			break;
		}

		start += delta;
		stop += delta;

		mp->flags &= ~(CPacket::SyncPoint | CPacket::Discontinuity);
	}

	if(m_mt.subtype == FOURCCMap(WAVE_FORMAT_WAVPACK4))
	{
		for(auto i = mp->group->ba.bm.begin(); i != mp->group->ba.bm.end(); i++)
		{
			CPacket* p = new CPacket();

			p->id = mp->id;
			p->start = mp->start;
			p->stop = mp->stop;
			p->flags = CPacket::TimeValid;
			p->buff = (*i)->BlockAdditional;

			hr = DeliverPacket(p);

			if(S_OK != hr) 
			{
				break;
			}
		}
	}

	delete mp;

	return hr;
}

