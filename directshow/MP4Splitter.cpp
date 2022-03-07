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
#include "MP4Splitter.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include "HttpReader.h"

#include "../3rdparty/bento4/Ap4.h"
#include "../3rdparty/bento4/Ap4File.h"
#include "../3rdparty/bento4/Ap4StssAtom.h"
#include "../3rdparty/bento4/Ap4StsdAtom.h"
#include "../3rdparty/bento4/Ap4IsmaCryp.h"
#include "../3rdparty/bento4/Ap4AvcCAtom.h"
#include "../3rdparty/bento4/Ap4ChplAtom.h"
#include "../3rdparty/bento4/Ap4FtabAtom.h"
#include "../3rdparty/bento4/Ap4DataAtom.h"
#include "../3rdparty/bento4/Ap4DrefAtom.h"

#include <initguid.h>
#include "moreuuids.h"

// CMP4SplitterFilter

CMP4SplitterFilter::CMP4SplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMP4SplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
{
}

CMP4SplitterFilter::~CMP4SplitterFilter()
{
	delete m_file;
}

HRESULT CMP4SplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_trackpos.clear();

	delete m_file;

	m_file = new CMP4SplitterFile(pAsyncReader, hr);

	if(FAILED(hr)) 
	{
		delete m_file;
		
		m_file = NULL;
		
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	std::map<AP4_Track::Type, REFERENCE_TIME> dur;
	
	dur[AP4_Track::TYPE_VIDEO] = -1;
	dur[AP4_Track::TYPE_AUDIO] = -1;
	dur[AP4_Track::TYPE_TEXT] = -1;
	dur[AP4_Track::TYPE_SUBP] = -1;

	m_framesize.cx = 640;
	m_framesize.cy = 480;

	AP4_Movie* movie = (AP4_Movie*)m_file->GetMovie();

	if(movie == NULL)
	{
		return E_FAIL;
	}

	AP4_List<AP4_Track>& tracks = movie->GetTracks();

	if(tracks.ItemCount() == 0)
	{
		AP4_RdrfAtom* rdrf = dynamic_cast<AP4_RdrfAtom*>(movie->GetMoovAtom()->FindChild("rmra/rmda/rdrf"));

		if(rdrf == NULL)
		{
			return E_FAIL;
		}

		std::wstring url(rdrf->m_url.begin(), rdrf->m_url.end());

		CComQIPtr<IFileSourceFilter> pFSF = (IBaseFilter*)this;

		if(pFSF == NULL)
		{
			pFSF = DirectShow::GetUpStreamFilter(this, m_pInput);
		}

		if(pFSF != NULL)
		{
			if(url.find(L"://") == std::wstring::npos && url.find(L"\\\\") == std::wstring::npos)
			{
				LPOLESTR str = NULL;

				if(SUCCEEDED(pFSF->GetCurFile(&str, NULL)))
				{
					std::wstring s = str;

					CoTaskMemFree(str);

					std::wstring::size_type i = s.find('?');

					if(i != std::wstring::npos && i > 0)
					{
						s = s.substr(0, i);
					}

					i = s.find_last_of('/');

					if(i != std::wstring::npos && i > 0)
					{
						s = s.substr(0, i + 1);
					}

					i = s.find_last_of('\\');

					if(i != std::wstring::npos && i > 0)
					{
						s = s.substr(0, i + 1);
					}

					url = s + url;
				}
			}

			if(CComQIPtr<IHttpReader> pHttpReader = pFSF)
			{
				pHttpReader->SetUserAgent("QuickTime"); // movies.apple.com requires this
			}

			if(SUCCEEDED(pFSF->Load(url.c_str(), NULL)))
			{
				return m_pInput ? CreateOutputPins(pAsyncReader) : S_OK;
			}
		}

		return E_FAIL;
	}
	
	for(AP4_List<AP4_Track>::Item* item = tracks.FirstItem(); item; item = item->GetNext())
	{
		AP4_Track* track = item->GetData();

		if(track->GetType() != AP4_Track::TYPE_VIDEO 
		&& track->GetType() != AP4_Track::TYPE_AUDIO
		&& track->GetType() != AP4_Track::TYPE_TEXT
		&& track->GetType() != AP4_Track::TYPE_SUBP)
		{
			continue;
		}

		AP4_Sample sample;

		if(AP4_FAILED(track->GetSample(0, sample)) || sample.GetDescriptionIndex() == 0xFFFFFFFF)
		{
			continue;
		}

		DWORD id = track->GetId();

		std::wstring name = Util::UTF8To16(track->GetTrackName().c_str());
		std::wstring lang = Util::UTF8To16(track->GetTrackLanguage().c_str());

		if(name.empty())
		{
			name = Util::Format(L"Output %d", id);
		}

		if(!lang.empty() && lang != L"und")
		{
			name += L" (" + lang + L")";
		}

		std::vector<CMediaType> mts;

		CMediaType mt;

		mt.SetSampleSize(1);

		VIDEOINFOHEADER* vih = NULL;
		WAVEFORMATEX* wfe = NULL;

		AP4_DataBuffer empty;

		if(AP4_SampleDescription* desc = track->GetSampleDescription(sample.GetDescriptionIndex()))
		{
			AP4_MpegSampleDescription* mpeg_desc = NULL;

			if(desc->GetType() == AP4_SampleDescription::TYPE_MPEG)
			{
				mpeg_desc = dynamic_cast<AP4_MpegSampleDescription*>(desc);
			}
			else if(desc->GetType() == AP4_SampleDescription::TYPE_ISMACRYP)
			{
				AP4_IsmaCrypSampleDescription* isma_desc = dynamic_cast<AP4_IsmaCrypSampleDescription*>(desc);

				mpeg_desc = isma_desc->GetOriginalSampleDescription();
			}

			if(AP4_MpegVideoSampleDescription* video_desc = dynamic_cast<AP4_MpegVideoSampleDescription*>(mpeg_desc))
			{
				const AP4_DataBuffer* di = video_desc->GetDecoderInfo();

				if(di == NULL) di = &empty;

				mt.majortype = MEDIATYPE_Video;
				mt.formattype = FORMAT_VideoInfo;

				vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + di->GetDataSize());

				memset(vih, 0, mt.FormatLength());

				vih->dwBitRate = video_desc->GetAvgBitrate()/8;
				vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
				vih->bmiHeader.biWidth = (LONG)video_desc->GetWidth();
				vih->bmiHeader.biHeight = (LONG)video_desc->GetHeight();

				memcpy(vih + 1, di->GetData(), di->GetDataSize());

				switch(video_desc->GetObjectTypeId())
				{
				case AP4_MPEG4_VISUAL_OTI:
					mt.subtype = FOURCCMap('v4pm');
					mt.formattype = FORMAT_MPEG2Video;
					{
					MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + di->GetDataSize());
					memset(vih, 0, mt.FormatLength());
					vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
					vih->hdr.bmiHeader.biWidth = (LONG)video_desc->GetWidth();
					vih->hdr.bmiHeader.biHeight = (LONG)video_desc->GetHeight();
					vih->hdr.bmiHeader.biCompression = 'v4pm';
					vih->hdr.bmiHeader.biPlanes = 1;
					vih->hdr.bmiHeader.biBitCount = 24;
					vih->hdr.dwPictAspectRatioX = vih->hdr.bmiHeader.biWidth;
					vih->hdr.dwPictAspectRatioY = vih->hdr.bmiHeader.biHeight;
					vih->cbSequenceHeader = di->GetDataSize();
					memcpy(vih->dwSequenceHeader, di->GetData(), di->GetDataSize());
					mts.push_back(mt);
					mt.subtype = FOURCCMap(vih->hdr.bmiHeader.biCompression = 'V4PM');
					mts.push_back(mt);
					}
					break;
				case AP4_MPEG2_VISUAL_SIMPLE_OTI:
				case AP4_MPEG2_VISUAL_MAIN_OTI:
				case AP4_MPEG2_VISUAL_SNR_OTI:
				case AP4_MPEG2_VISUAL_SPATIAL_OTI:
				case AP4_MPEG2_VISUAL_HIGH_OTI:
				case AP4_MPEG2_VISUAL_422_OTI:
					mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
					{
					m_file->Seek(sample.GetOffset());
					CBaseSplitterFile::MpegSequenceHeader h;
					CMediaType mt2;
					if(m_file->Read(h, sample.GetSize(), &mt2)) mt = mt2;
					}
					mts.push_back(mt);
					break;
				case AP4_MPEG1_VISUAL_OTI: // ???
					mt.subtype = MEDIASUBTYPE_MPEG1Payload;
					{
					m_file->Seek(sample.GetOffset());
					CBaseSplitterFile::MpegSequenceHeader h;
					CMediaType mt2;
					if(m_file->Read(h, sample.GetSize(), &mt2)) mt = mt2;
					}
					mts.push_back(mt);
					break;
				}

				if(mt.subtype == GUID_NULL)
				{
					wprintf(L"Unknown video OBI: %02x\n", video_desc->GetObjectTypeId());
				}
			}
			else if(AP4_MpegAudioSampleDescription* audio_desc = dynamic_cast<AP4_MpegAudioSampleDescription*>(mpeg_desc))
			{
				const AP4_DataBuffer* di = audio_desc->GetDecoderInfo();

				if(di == NULL) di = &empty;

				mt.majortype = MEDIATYPE_Audio;
				mt.formattype = FORMAT_WaveFormatEx;

				wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + di->GetDataSize());

				memset(wfe, 0, mt.FormatLength());

				wfe->nSamplesPerSec = audio_desc->GetSampleRate();
				wfe->nAvgBytesPerSec = audio_desc->GetAvgBitrate()/8;
				wfe->nChannels = audio_desc->GetChannelCount();
				wfe->wBitsPerSample = audio_desc->GetSampleSize();
				wfe->cbSize = (WORD)di->GetDataSize();

				memcpy(wfe + 1, di->GetData(), di->GetDataSize());

				switch(audio_desc->GetObjectTypeId())
				{
				case AP4_MPEG4_AUDIO_OTI:
				case AP4_MPEG2_AAC_AUDIO_MAIN_OTI: // ???
				case AP4_MPEG2_AAC_AUDIO_LC_OTI: // ???
				case AP4_MPEG2_AAC_AUDIO_SSRP_OTI: // ???
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_AAC);
					if(wfe->cbSize >= 2) wfe->nChannels = (((BYTE*)(wfe + 1))[1] >> 3) & 0xf;
					mts.push_back(mt);
					break;
				case AP4_MPEG2_PART3_AUDIO_OTI: // ???
				case AP4_MPEG1_AUDIO_OTI:
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MP3);
					{
					m_file->Seek(sample.GetOffset());
					CBaseSplitterFile::MpegAudioHeader h;
					CMediaType mt2;
					if(m_file->Read(h, sample.GetSize(), false, &mt2)) mt = mt2;
					}
					mts.push_back(mt);
					break;
				}

				if(mt.subtype == GUID_NULL)
				{
					wprintf(L"Unknown audio OBI: %02x\n", audio_desc->GetObjectTypeId());
				}
			}
			else if(AP4_MpegSystemSampleDescription* system_desc = dynamic_cast<AP4_MpegSystemSampleDescription*>(desc))
			{
				const AP4_DataBuffer* di = system_desc->GetDecoderInfo();
				
				if(di == NULL) di = &empty;

				switch(system_desc->GetObjectTypeId())
				{
				case AP4_NERO_VOBSUB:
					if(di->GetDataSize() >= 16*4)
					{
						int width = 720;
						int height = 576;

						if(AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD)))
						{
							width = tkhd->GetWidth() >> 16;
							height = tkhd->GetHeight() >> 16;
						}

						const AP4_Byte* pal = di->GetData();

						std::list<std::wstring> sl;

						for(int i = 0; i < 16 * 4; i += 4)
						{
							BYTE y = (pal[i + 1] - 16) * 255 / 219;
							BYTE u = pal[i + 2];
							BYTE v = pal[i + 3];

							BYTE r = (BYTE)min(max(1.0 * y + 1.4022 * (v - 128), 0), 255);
							BYTE g = (BYTE)min(max(1.0 * y - 0.3456 * (u - 128) - 0.7145 * (v - 128), 0), 255);
							BYTE b = (BYTE)min(max(1.0 * y + 1.7710 * (u - 128), 0) , 255);

							sl.push_back(Util::Format(L"%02x%02x%02x", r, g, b));
						}

						std::wstring hdr = Util::Format(
							L"# VobSub index file, v7 (do not modify this line!)\n"
							L"size: %dx%d\n"
							L"palette: %s\n",
							width, height,
							Util::Implode(sl, ','));
						
						mt.majortype = MEDIATYPE_Subtitle;
						mt.subtype = MEDIASUBTYPE_VOBSUB;
						mt.formattype = FORMAT_SubtitleInfo;

						SUBTITLEINFO* si = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + hdr.size());

						memset(si, 0, mt.FormatLength());

						si->dwOffset = sizeof(SUBTITLEINFO);
						
						strcpy_s(si->IsoLang, countof(si->IsoLang), std::string(lang.begin(), lang.end()).c_str());
						wcscpy_s(si->TrackName, countof(si->TrackName), name.c_str());

						std::string s(hdr.begin(), hdr.end());

						memcpy(si + 1, s.c_str(), s.size());
						
						mts.push_back(mt);
					}

					break;
				}

				if(mt.subtype == GUID_NULL)
				{
					wprintf(L"Unknown audio OBI: %02x\n", system_desc->GetObjectTypeId());
				}
			}
			else if(AP4_UnknownSampleDescription* unknown_desc = dynamic_cast<AP4_UnknownSampleDescription*>(desc)) // TEMP
			{
				AP4_SampleEntry* sample_entry = unknown_desc->GetSampleEntry();

				if(dynamic_cast<AP4_TextSampleEntry*>(sample_entry)
				|| dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry))
				{
					mt.majortype = MEDIATYPE_Subtitle;
					mt.subtype = MEDIASUBTYPE_ASS2;
					mt.formattype = FORMAT_SubtitleInfo;

					std::wstring hdr = Util::Format(
						L"[Script Info]\n"
						L"ScriptType: v4.00++\n"
						L"ScaledBorderAndShadow: yes\n"
						L"PlayResX: %d\n"
						L"PlayResY: %d\n"
						L"[V4++ Styles]\n"
						L"Style: Text,Arial,12,&H00ffffff,&H0000ffff,&H00000000,&H80000000,0,0,0,0,100,100,0,0.00,3,0,0,2,0,0,0,0,1,1\n",
						// L"Style: Text,Arial,12,&H00ffffff,&H0000ffff,&H00000000,&H80000000,0,0,0,0,100,100,0,0.00,1,0,0,2,0,0,0,0,1,1\n",
						m_framesize.cx,
						m_framesize.cy);

					SUBTITLEINFO* si = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + hdr.size());

					memset(si, 0, mt.FormatLength());

					si->dwOffset = sizeof(SUBTITLEINFO);

					strcpy_s(si->IsoLang, countof(si->IsoLang), std::string(lang.begin(), lang.end()).c_str());
					wcscpy_s(si->TrackName, countof(si->TrackName), name.c_str());

					std::string s(hdr.begin(), hdr.end());

					memcpy(si + 1, s.c_str(), s.size());

					mts.push_back(mt);
				}
			}
		}
		else if(AP4_Avc1SampleEntry* avc1 = dynamic_cast<AP4_Avc1SampleEntry*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd/avc1")))
		{
			if(AP4_AvcCAtom* avcC = dynamic_cast<AP4_AvcCAtom*>(avc1->GetChild(AP4_ATOM_TYPE_AVCC)))
			{
				const AP4_DataBuffer* di = avcC->GetDecoderInfo();

				if(di == NULL) di = &empty;

				const AP4_Byte* data = di->GetData();
				AP4_Size size = di->GetDataSize();
/*
				mt.majortype = MEDIATYPE_Video;
				mt.subtype = MEDIASUBTYPE_H264_TRANSPORT;
				mt.formattype = FORMAT_VideoInfo2;

				VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));

				memset(vih, 0, mt.FormatLength());

				vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
				vih->bmiHeader.biWidth = (LONG)avc1->GetWidth();
				vih->bmiHeader.biHeight = (LONG)avc1->GetHeight();
				vih->bmiHeader.biCompression = '462H';
				vih->bmiHeader.biPlanes = 1;
				vih->bmiHeader.biBitCount = 24;
				vih->dwPictAspectRatioX = vih->bmiHeader.biWidth;
				vih->dwPictAspectRatioY = vih->bmiHeader.biHeight;

				mts.push_back(mt);
*/
				mt.majortype = MEDIATYPE_Video;
				mt.subtype = FOURCCMap('1cva');
				mt.formattype = FORMAT_MPEG2Video;

				MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + size - 7);

				memset(vih, 0, mt.FormatLength());

				vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
				vih->hdr.bmiHeader.biWidth = (LONG)avc1->GetWidth();
				vih->hdr.bmiHeader.biHeight = (LONG)avc1->GetHeight();
				vih->hdr.bmiHeader.biCompression = '1cva';
				vih->hdr.bmiHeader.biPlanes = 1;
				vih->hdr.bmiHeader.biBitCount = 24;
				vih->hdr.dwPictAspectRatioX = vih->hdr.bmiHeader.biWidth;
				vih->hdr.dwPictAspectRatioY = vih->hdr.bmiHeader.biHeight;
				vih->dwProfile = data[1];
				vih->dwLevel = data[3];
				vih->dwFlags = (data[4] & 3) + 1;
				vih->hdr.dwReserved1 = (data[4] & 3) + 1;

				vih->cbSequenceHeader = 0;

				BYTE* src = (BYTE*)data + 5;
				BYTE* dst = (BYTE*)vih->dwSequenceHeader;

				BYTE* src_end = (BYTE*)data + size;
				BYTE* dst_end = (BYTE*)vih->dwSequenceHeader + size;

				for(int i = 0; i < 2; i++)
				{
					for(int n = *src++ & 0x1f; n > 0; n--)
					{
						int len = ((src[0] << 8) | src[1]) + 2;

						if(src + len > src_end || dst + len > dst_end)
						{
							ASSERT(0); 
							break;
						}

						memcpy(dst, src, len);
						
						src += len; 
						dst += len;
						
						vih->cbSequenceHeader += len;
					}
				}

				mts.push_back(mt);

				mt.subtype = FOURCCMap(vih->hdr.bmiHeader.biCompression = '1CVA');

				mts.push_back(mt);
			}
		}
		else if(AP4_StsdAtom* stsd = dynamic_cast<AP4_StsdAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd")))
		{
			const AP4_DataBuffer& db = stsd->GetDataBuffer();

			for(AP4_List<AP4_Atom>::Item* item = stsd->GetChildren().FirstItem(); item; item = item->GetNext())
			{
				AP4_Atom* atom = item->GetData();
				AP4_Atom::Type type = atom->GetType();

				if((type & 0xffff0000) == AP4_ATOM_TYPE('m', 's', 0, 0))
				{
					type &= 0xffff;
				}
				else if(type == AP4_ATOM_TYPE('a', 'c', '-', '3')) 
				{
					type = WAVE_FORMAT_DOLBY_AC3;
				}
				else if(type == AP4_ATOM_TYPE__MP3)
				{
					type = 0x0055;
				}
				/*
				else if(type == AP4_ATOM_TYPE_TWOS)
				{
					type = WAVE_FORMAT_PCM;
				}
				else if(type == AP4_ATOM_TYPE_SOWT)
				{
					type = WAVE_FORMAT_PCM;
				}*/
				else
				{
					type = 
						((type >> 24) & 0x000000ff) |
						((type >>  8) & 0x0000ff00) |
						((type <<  8) & 0x00ff0000) |
						((type << 24) & 0xff000000);
				}

				if(AP4_VisualSampleEntry* vse = dynamic_cast<AP4_VisualSampleEntry*>(atom))
				{
					mt.majortype = MEDIATYPE_Video;
					mt.subtype = FOURCCMap(type);
					mt.formattype = FORMAT_VideoInfo;

					vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)+db.GetDataSize());

					memset(vih, 0, mt.FormatLength());

					vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
					vih->bmiHeader.biWidth = (LONG)vse->GetWidth();
					vih->bmiHeader.biHeight = (LONG)vse->GetHeight();
					vih->bmiHeader.biCompression = type;
					
					memcpy(vih + 1, db.GetData(), db.GetDataSize());

					mts.push_back(mt);

					char buff[5];
					memcpy(buff, &type, 4);
					buff[4] = 0;

					strlwr((char*)&buff);

					AP4_Atom::Type typelwr = *(AP4_Atom::Type*)buff;

					if(typelwr != type)
					{
						mt.subtype = FOURCCMap(vih->bmiHeader.biCompression = typelwr);

						mts.push_back(mt);
					}

					strupr((char*)&buff);

					AP4_Atom::Type typeupr = *(AP4_Atom::Type*)buff;

					if(typeupr != type)
					{
						mt.subtype = FOURCCMap(vih->bmiHeader.biCompression = typeupr);
					
						mts.push_back(mt);
					}

					break;
				}
				else if(AP4_AudioSampleEntry* ase = dynamic_cast<AP4_AudioSampleEntry*>(atom))
				{
					mt.majortype = MEDIATYPE_Audio;
					mt.subtype = FOURCCMap(type);
					mt.formattype = FORMAT_WaveFormatEx;

					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + db.GetDataSize());

					memset(wfe, 0, mt.FormatLength());

					if((type & 0xffff0000) == 0) wfe->wFormatTag = (WORD)type;
					wfe->nSamplesPerSec = ase->GetSampleRate();
					wfe->nChannels = ase->GetChannelCount();
					wfe->wBitsPerSample = ase->GetSampleSize();
					wfe->nBlockAlign = ase->GetBytesPerFrame();
					wfe->cbSize = (WORD)db.GetDataSize();

					if(wfe->wFormatTag == WAVE_FORMAT_PCM)
					{
						wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;
					}

					memcpy(wfe + 1, db.GetData(), db.GetDataSize());
					
					mts.push_back(mt);
					
					break;
				}
				else if(type == WAVE_FORMAT_DOLBY_AC3 && db.GetDataSize() >= 0x2f) 
				{
					// TODO: AC3SampleEntry, EC3SampleEntry (ETSI TS 102 366 V1.2.1 Annex F)

					const AP4_Byte* p = db.GetData();

					/*
					0x1075F3C0  00 00 00 2f 61 63 2d 33 00 00 00 00 00 00 00 01  .../ac-3........
					0x1075F3D0  00 00 00 00 00 00 00 00 00 02 00 10 00 00 00 00  ................
					0x1075F3E0  bb 80 00 00 00 00 00 0b 64 61 63 33 10 11 60 fd  »€......dac3..`ý
					*/

					static int channels[] = {2, 1, 2, 3, 3, 4, 4, 5};
					static int freq[] = {48000, 44100, 32000, 0};
					static int rate[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};

					BYTE fscod = (p[44] >> 6) & 3;
					BYTE bsid = (p[44] >> 1) & 31;
					BYTE acmod = (p[45] >> 3) & 7;
					BYTE lfeon = (p[45] >> 2) & 1;
					BYTE frmsizecod = ((p[45] & 3) << 3) | ((p[46] >> 5) & 7);

					WAVEFORMATEX wfe;

					memset(&wfe, 0, sizeof(wfe));

					wfe.wFormatTag = WAVE_FORMAT_DOLBY_AC3;
					wfe.nChannels = channels[acmod] + lfeon;
					wfe.nSamplesPerSec = freq[fscod];

					switch(bsid)
					{
					case 9: wfe.nSamplesPerSec >>= 1; break;
					case 10: wfe.nSamplesPerSec >>= 2; break;
					case 11: wfe.nSamplesPerSec >>= 3; break;
					default: break;
					}

					wfe.nAvgBytesPerSec = rate[frmsizecod] * 1000 / 8;
					wfe.nBlockAlign = (WORD)(1536 * wfe.nAvgBytesPerSec / wfe.nSamplesPerSec);

					mt.majortype = MEDIATYPE_Audio;
					mt.subtype = MEDIASUBTYPE_DOLBY_AC3;
					mt.formattype = FORMAT_WaveFormatEx;
					mt.SetFormat((BYTE*)&wfe, sizeof(wfe));

					mts.push_back(mt);

					break;
				}
			}
		}

		if(mts.empty()) continue;

		REFERENCE_TIME rtDuration = 10000i64 * track->GetDurationMs();

		auto type_dur = dur.find(track->GetType());

		if(type_dur->second < rtDuration)
		{
			type_dur->second = rtDuration;
		}

		for(auto i = mts.begin(); i != mts.end(); i++)
		{
			BITMAPINFOHEADER bih;

			if(CMediaTypeEx(*i).ExtractBIH(&bih))
			{
				m_framesize.cx = bih.biWidth;
				m_framesize.cy = abs(bih.biHeight);
			}
		}

		CBaseSplitterOutputPin* pin = new CBaseSplitterOutputPin(mts, name.c_str(), this, &hr);

		if(!name.empty()) pin->SetProperty(L"NAME", name.c_str());
		if(!lang.empty()) pin->SetProperty(L"LANG", lang.c_str());

		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pin)));

		m_trackpos[id] = TrackPosition();

		if(mts.size() == 1 && mts[0].subtype == MEDIASUBTYPE_ASS2)
		{
			LPCWSTR postfix = L" (plain text)";

			mts[0].subtype = MEDIASUBTYPE_UTF8;

			SUBTITLEINFO* si = (SUBTITLEINFO*)mts[0].ReallocFormatBuffer(sizeof(SUBTITLEINFO));

			wcscat(si->TrackName, postfix);

			id ^= 0x80402010; // FIXME: until fixing, let's hope there won't be another track like this...

			CBaseSplitterOutputPin* pin = new CBaseSplitterOutputPin(mts, (name + postfix).c_str(), this, &hr);

			if(!name.empty()) pin->SetProperty(L"NAME", (name + postfix).c_str());
			if(!lang.empty()) pin->SetProperty(L"LANG", lang.c_str());

			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pin)));
		}
	}

	if(AP4_ChplAtom* chpl = dynamic_cast<AP4_ChplAtom*>(movie->GetMoovAtom()->FindChild("udta/chpl")))
	{
		AP4_Array<AP4_ChplAtom::AP4_Chapter>& chapters = chpl->GetChapters();

		for(AP4_Cardinal i = 0; i < chapters.ItemCount(); i++)
		{
			AP4_ChplAtom::AP4_Chapter& chapter = chapters[i];

			std::string name = Util::ConvertMBCS(chapter.Name.c_str(), ANSI_CHARSET, CP_UTF8); // this is b0rked, thx to nero :P

			ChapAppend(chapter.Time, Util::UTF8To16(name.c_str()).c_str());
		}

		ChapSort();
	}

	if(AP4_ContainerAtom* ilst = dynamic_cast<AP4_ContainerAtom*>(movie->GetMoovAtom()->FindChild("udta/meta/ilst")))
	{
		std::wstring title, artist, writer, album, year, appl, desc, gen, track;

		for(AP4_List<AP4_Atom>::Item* item = ilst->GetChildren().FirstItem(); item; item = item->GetNext())
		{
			AP4_ContainerAtom* atom = dynamic_cast<AP4_ContainerAtom*>(item->GetData());

			if(atom == NULL) continue;

			AP4_DataAtom* data = dynamic_cast<AP4_DataAtom*>(atom->GetChild(AP4_ATOM_TYPE_DATA));

			if(data == NULL) continue;

			const AP4_DataBuffer* db = data->GetData();

			if(atom->GetType() == AP4_ATOM_TYPE_TRKN)
			{
				if(db->GetDataSize() >= 4)
				{
					WORD n = (db->GetData()[2] << 8) | db->GetData()[3];

					track = Util::Format(n > 0 && n < 100 ? L"%02d" : L"%d", n);
				}
			}
			else
			{
				std::string value((LPCSTR)db->GetData(), db->GetDataSize());

				std::wstring str = Util::UTF8To16(value.c_str());

				switch(atom->GetType())
				{
				case AP4_ATOM_TYPE_NAM: title = str; break;
				case AP4_ATOM_TYPE_ART: artist = str; break;
				case AP4_ATOM_TYPE_WRT: writer = str; break;
				case AP4_ATOM_TYPE_ALB: album = str; break;
				case AP4_ATOM_TYPE_DAY: year = str; break;
				case AP4_ATOM_TYPE_TOO: appl = str; break;
				case AP4_ATOM_TYPE_CMT: desc = str; break;
				case AP4_ATOM_TYPE_GEN: gen = str; break;
				}
			}
		}

		if(!title.empty())
		{
			if(!track.empty()) title = track + L" - " + title;
			if(!album.empty()) title = album + L" - " + title;
			if(!year.empty()) title += L" - " +  year;
			if(!gen.empty()) title += L" - " + gen;

			SetProperty(L"TITL", title.c_str());
		}

		if(!artist.empty()) 
		{
			SetProperty(L"AUTH", artist.c_str());
		}
		else if(!writer.empty()) 
		{
			SetProperty(L"AUTH", writer.c_str());
		}

		if(!appl.empty())
		{
			SetProperty(L"APPL", appl.c_str());
		}

		if(!desc.empty()) 
		{
			SetProperty(L"DESC", desc.c_str());
		}
	}
	/*
	m_rtDuration = _I64_MAX;
	
	if(dur[AP4_Track::TYPE_VIDEO] > 0 && dur[AP4_Track::TYPE_VIDEO] < m_rtDuration)
	{
		m_rtDuration = dur[AP4_Track::TYPE_VIDEO];
	}

	if(dur[AP4_Track::TYPE_AUDIO] > 0 && dur[AP4_Track::TYPE_AUDIO] < m_rtDuration)
	{
		m_rtDuration = dur[AP4_Track::TYPE_AUDIO];
	}

	if(m_rtDuration == _I64_MAX)
	{
		if(dur[AP4_Track::TYPE_TEXT] > 0)
		{
			m_rtDuration = dur[AP4_Track::TYPE_TEXT];
		}
		else if(dur[AP4_Track::TYPE_SUBP] > 0)
		{
			m_rtDuration = dur[AP4_Track::TYPE_SUBP];
		}
	}
	*/

	if(dur[AP4_Track::TYPE_VIDEO] > 0)
	{
		m_rtDuration = dur[AP4_Track::TYPE_VIDEO];
	}

	if(dur[AP4_Track::TYPE_AUDIO] > 0 && dur[AP4_Track::TYPE_AUDIO] > m_rtDuration)
	{
		m_rtDuration = dur[AP4_Track::TYPE_AUDIO];
	}

	if(m_rtDuration == 0)
	{
		if(dur[AP4_Track::TYPE_TEXT] > 0)
		{
			m_rtDuration = dur[AP4_Track::TYPE_TEXT];
		}
		else if(dur[AP4_Track::TYPE_SUBP] > 0)
		{
			m_rtDuration = dur[AP4_Track::TYPE_SUBP];
		}
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool CMP4SplitterFilter::DemuxInit()
{
	AP4_Movie* movie = (AP4_Movie*)m_file->GetMovie();

	for(auto i = m_trackpos.begin(); i != m_trackpos.end(); i++)
	{
		i->second.index = 0;
		i->second.ts = 0;

		AP4_Track* track = movie->GetTrack(i->first);

		AP4_Sample sample;

		if(AP4_SUCCEEDED(track->GetSample(0, sample)))
		{
			i->second.ts = sample.GetCts();
		}
	}

	return true;
}

void CMP4SplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	AP4_TimeStamp ts = (AP4_TimeStamp)(rt / 10000);

	AP4_Movie* movie = (AP4_Movie*)m_file->GetMovie();

	for(auto i = m_trackpos.begin(); i != m_trackpos.end(); i++)
	{
		AP4_Track* track = movie->GetTrack(i->first);

		if(AP4_FAILED(track->GetSampleIndexForTimeStampMs(ts, i->second.index)))
		{
			i->second.index = 0;
		}

		AP4_Sample sample;

		if(AP4_SUCCEEDED(track->GetSample(i->second.index, sample)))
		{
			i->second.ts = sample.GetCts();
		}

		// FIXME: slow search & stss->m_Entries is private

		AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss"));

		if(stss == NULL || stss->m_Entries.ItemCount() == 0)
		{
			continue;
		}

		AP4_Cardinal entry = -1;

		while(++entry < stss->m_Entries.ItemCount() && stss->m_Entries[entry] - 1 <= i->second.index);

		if(entry > 0) entry--;

		i->second.index = stss->m_Entries[entry] - 1;
	}
}
/*
struct SSACharacter {CString pre, post; WCHAR c;}; 

static CStringW ConvertTX3GToSSA(
	CStringW str, 
	const AP4_Tx3gSampleEntry::AP4_Tx3gDescription& desc, 
	CStringW font,
	const AP4_Byte* mods, 
	int size, 
	CSize framesize,
	CPoint translation,
	int durationms,
	CRect& rbox)
{
	int str_len = str.GetLength();

	SSACharacter* chars = new SSACharacter[str_len];
	for(int i = 0; i < str_len; i++) chars[i].c = str[i];
	str.Empty();

	//

	rbox.SetRect(desc.TextBox.Left, desc.TextBox.Top, desc.TextBox.Right, desc.TextBox.Bottom);

	int align = 2;
	signed char h = (signed char)desc.HorizontalJustification;
	signed char v = (signed char)desc.VerticalJustification;
	if(h == 0 && v < 0) align = 1;
	else if(h > 0 && v < 0) align = 2;
	else if(h < 0 && v < 0) align = 3;
	else if(h == 0 && v > 0) align = 4;
	else if(h > 0 && v > 0) align = 5;
	else if(h < 0 && v > 0) align = 6;
	else if(h == 0 && v == 0) align = 7;
	else if(h > 0 && v == 0) align = 8;
	else if(h < 0 && v == 0) align = 9;
	str.Format(L"{\\an%d}%s", align, CStringW(str));

	if(!font.CompareNoCase(L"serif")) font = L"Times New Roman";
	else if(!font.CompareNoCase(L"sans-serif")) font = L"Arial";
	else if(!font.CompareNoCase(L"monospace")) font = L"Courier New";
	str.Format(L"{\\fn%s}%s", font, CStringW(str));	

	const AP4_Byte* fclr = (const AP4_Byte*)&desc.Style.Font.Color;

	CStringW font_color;
	font_color.Format(L"{\\1c%02x%02x%02x\\1a%02x}", fclr[2], fclr[1], fclr[0], 255 - fclr[3]);
	str = font_color + str;

	str.Format(L"%s{\\2c%02x%02x%02x\\2a%02x}", CString(str), fclr[2], fclr[1], fclr[0], 255 - fclr[3]);

	CStringW font_size;
	font_size.Format(L"{\\fs%d}", desc.Style.Font.Size);
	str = font_size + str;

	CStringW font_flags;
	font_flags.Format(L"{\\b%d\\i%d\\u%d}", 
		(desc.Style.Font.Face&1) ? 1 : 0,
		(desc.Style.Font.Face&2) ? 1 : 0,
		(desc.Style.Font.Face&4) ? 1 : 0);
	str = font_flags + str;

	//

	const AP4_Byte* hclr = (const AP4_Byte*)&desc.BackgroundColor;

	while(size > 8)
	{
		DWORD tag_size = (mods[0]<<24)|(mods[1]<<16)|(mods[2]<<8)|(mods[3]); mods += 4;
		DWORD tag = (mods[0]<<24)|(mods[1]<<16)|(mods[2]<<8)|(mods[3]); mods += 4;

		size -= tag_size;
		tag_size -= 8;
		const AP4_Byte* next = mods + tag_size;

		if(tag == 'styl')
		{
			WORD styl_count = (mods[0]<<8)|(mods[1]); mods += 2;

			while(styl_count-- > 0)
			{
				WORD start = (mods[0]<<8)|(mods[1]); mods += 2;
				WORD end = (mods[0]<<8)|(mods[1]); mods += 2;
				WORD font_id = (mods[0]<<8)|(mods[1]); mods += 2;
				WORD flags = mods[0]; mods += 1;
				WORD size = mods[0]; mods += 1;
				const AP4_Byte* color = mods; mods += 4;

				if(end > str_len) end = str_len;

				if(start < end)
				{
					CStringW s;

					s.Format(L"{\\1c%02x%02x%02x\\1a%02x}", color[2], color[1], color[0], 255 - color[3]);
					chars[start].pre += s;
					chars[end-1].post += font_color;

					s.Format(L"{\\fs%d}", size);
					chars[start].pre += s;
					chars[end-1].post += font_size;

					s.Format(L"{\\b%d\\i%d\\u%d}", (flags&1) ? 1 : 0, (flags&2) ? 1 : 0, (flags&4) ? 1 : 0);
					chars[start].pre += s; 
					chars[end-1].post += font_flags;
				}
			}
		}
		else if(tag == 'hclr')
		{
			hclr = mods;
		}
		else if(tag == 'hlit')
		{
			WORD start = (mods[0]<<8)|(mods[1]); mods += 2;
			WORD end = (mods[0]<<8)|(mods[1]); mods += 2;

			if(end > str_len) end = str_len;

			if(start < end)
			{
				CStringW s;

				s.Format(L"{\\3c%02x%02x%02x\\3a%02x}", hclr[2], hclr[1], hclr[0], 255 - hclr[3]);
				chars[start].pre += s;
				chars[end-1].post += font_color;

				chars[start].pre += L"{\\bord0.1}";
				chars[end-1].post += L"{\\bord}";
			}
		}
		else if(tag == 'blnk')
		{
			WORD start = (mods[0]<<8)|(mods[1]); mods += 2;
			WORD end = (mods[0]<<8)|(mods[1]); mods += 2;

			if(end > str_len) end = str_len;

			if(start < end)
			{
				// cheap...

				for(int i = 0, alpha = 255; i < durationms; i += 750, alpha = 255 - alpha)
				{
					CStringW s;
					s.Format(L"{\\t(%d, %d, \\alpha&H%02x&)}", i, i + 750, alpha);
					chars[start].pre += s;
				}

				chars[end-1].post += L"{\\alpha}";
			}
		}
		else if(tag == 'tbox')
		{
			rbox.top = (mods[0]<<8)|(mods[1]); mods += 2;
			rbox.left = (mods[0]<<8)|(mods[1]); mods += 2;
			rbox.bottom = (mods[0]<<8)|(mods[1]); mods += 2;
			rbox.right = (mods[0]<<8)|(mods[1]); mods += 2;
		}
		else if(tag == 'krok' && !(desc.DisplayFlags & 0x800))
		{
			DWORD start_time = (mods[0]<<24)|(mods[1]<<16)|(mods[2]<<8)|(mods[3]); mods += 4;
			WORD krok_count = (mods[0]<<8)|(mods[1]); mods += 2;

			while(krok_count-- > 0)
			{
				DWORD end_time = (mods[0]<<24)|(mods[1]<<16)|(mods[2]<<8)|(mods[3]); mods += 4;
				WORD start = (mods[0]<<8)|(mods[1]); mods += 2;
				WORD end = (mods[0]<<8)|(mods[1]); mods += 2;

				if(end > str_len) end = str_len;

				if(start < end)
				{
					CStringW s;

					s.Format(L"{\\kt%d\\kf%d}", start_time/10, (end_time - start_time)/10);
					chars[start].pre += s;
					s.Format(L"{\\1c%02x%02x%02x\\1a%02x}", hclr[2], hclr[1], hclr[0], 255 - hclr[3]);
					chars[start].pre += s;
					chars[end-1].post += L"{\\kt}" + font_color;
				}

				start_time = end_time;
			}
		}

		mods = next;
	}

	// continous karaoke

	if(desc.DisplayFlags & 0x800)
	{
		CStringW s;

		s.Format(L"{\\1c%02x%02x%02x\\1a%02x}", hclr[2], hclr[1], hclr[0], 255 - hclr[3]);
		str += s;

		int breaks = 0;

		for(int i = 0, j = 0; i <= str_len; i++)
		{
			if(chars[i].c == '\n') // || chars[i].c == ' ')
			{
				breaks++;
			}
		}

		if(str_len > breaks)
		{
			float dur = (float)max(durationms - 500, 0) / 10;

			for(int i = 0, j = 0; i <= str_len; i++)
			{
				if(i == str_len || chars[i].c == '\n') //|| chars[i].c == ' ')
				{
					s.Format(L"{\\kf%d}", (int)(dur * (i - j) / (str_len - breaks)));
					chars[j].pre += s;
					j = i+1;
				}
			}
		}
	}

	//

	for(int i = 0; i < str_len; i++) 
	{
		str += chars[i].pre;
		str += chars[i].c;
		if(desc.DisplayFlags & 0x20000) str += L"\\N";
		str += chars[i].post;
	}

	delete [] chars;

	//

	if(rbox.IsRectEmpty()) rbox.SetRect(0, 0, framesize.cx, framesize.cy);
	rbox.OffsetRect(translation);

	CRect rbox2 = rbox;
	rbox2.DeflateRect(2, 2);

	CRect r(0,0,0,0);
	if(rbox2.Height() > 0) {r.top = rbox2.top; r.bottom = framesize.cy - rbox2.bottom;}
	if(rbox2.Width() > 0) {r.left = rbox2.left; r.right = framesize.cx - rbox2.right;}

	CStringW hdr;
	hdr.Format(L"0,0,Text,,%d,%d,%d,%d,,{\\clip(%d,%d,%d,%d)}", 
		r.left, r.right, r.top, r.bottom,
		rbox.left, rbox.top, rbox.right, rbox.bottom);

	//

	return hdr + str;
}
*/

bool CMP4SplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	AP4_Movie* movie = (AP4_Movie*)m_file->GetMovie();

	// bool paused = false;

	while(SUCCEEDED(hr) && !CheckRequest(NULL))
	{
		std::map<DWORD, TrackPosition>::iterator iNext = m_trackpos.end();
		REFERENCE_TIME rtNext = 0;

		for(auto i = m_trackpos.begin(); i != m_trackpos.end(); i++)
		{
			AP4_Track* track = movie->GetTrack(i->first);

			CBaseSplitterOutputPin* pPin = GetOutputPin((DWORD)track->GetId());

			if(!pPin->IsConnected()) continue;

			REFERENCE_TIME rt = (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * i->second.ts);

			if(i->second.index < track->GetSampleCount() && (iNext == m_trackpos.end() || rtNext > rt))
			{
				iNext = i;
				rtNext = rt;
			}
		}

		if(iNext == m_trackpos.end()) break;

		AP4_Track* track = movie->GetTrack(iNext->first);

		CBaseSplitterOutputPin* pin = GetOutputPin((DWORD)track->GetId());

		AP4_Sample sample;
		AP4_DataBuffer data;

		if(pin != NULL && pin->IsConnected() && AP4_SUCCEEDED(track->ReadSample(iNext->second.index, sample, data)))
		{
			const CMediaType& mt = pin->CurrentMediaType();

			CPacket* p = new CPacket();

			p->id = (DWORD)track->GetId();

			p->start = (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetCts());
			p->stop = p->start + (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetDuration());

			p->flags |= CPacket::SyncPoint | CPacket::TimeValid;

			// FIXME: slow search & stss->m_Entries is private

			if(AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss")))
			{
				if(stss->m_Entries.ItemCount() > 0)
				{
					p->flags &= ~CPacket::SyncPoint;

					AP4_Cardinal i = -1;

					while(++i < stss->m_Entries.ItemCount())
					{
						if(stss->m_Entries[i] - 1 == iNext->second.index)
						{
							p->flags |= CPacket::SyncPoint;
						}
					}
				}
			}

			//

			if(track->GetType() == AP4_Track::TYPE_AUDIO && data.GetDataSize() == 1)
			{
				WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

				int nBlockAlign = 1200;

				if(wfe->nBlockAlign > 1)
				{
					nBlockAlign = wfe->nBlockAlign;

					iNext->second.index -= iNext->second.index % wfe->nBlockAlign;
				}

				p->stop = p->start;

				int first = true;

				while(AP4_SUCCEEDED(track->ReadSample(iNext->second.index, sample, data)))
				{
					const AP4_Byte* ptr = data.GetData();

					for(int i = 0, j = data.GetDataSize(); i < j; i++) 
					{
						p->buff.push_back(ptr[i]);
					}

					if(first) 
					{
						p->start = p->stop = (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetCts()); 

						first = false;
					}

					p->stop += (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetDuration());

					if(iNext->second.index + 1 >= track->GetSampleCount() || p->buff.size() >= nBlockAlign)
					{
						break;
					}

					iNext->second.index++;
				}
			}
/*
			else if(track->GetType() == AP4_Track::TYPE_TEXT)
			{
				CStringA dlgln_bkg, dlgln_plaintext;

				const AP4_Byte* ptr = data.GetData();
				AP4_Size avail = data.GetDataSize();

				if(avail > 2)
				{
					AP4_UI16 size = (ptr[0] << 8) | ptr[1];

					if(size <= avail-2)
					{
						CStringA str;

						if(size >= 2 && ptr[2] == 0xfe && ptr[3] == 0xff)
						{
							CStringW wstr = CStringW((LPCWSTR)&ptr[2], size/2);
							for(int i = 0; i < wstr.GetLength(); i++) wstr.SetAt(i, ((WORD)wstr[i] >> 8) | ((WORD)wstr[i] << 8));
							str = UTF16To8(wstr);
						}
						else
						{
							str = CStringA((LPCSTR)&ptr[2], size);
						}

						CStringA dlgln = str;

						if(mt.subtype == MEDIASUBTYPE_ASS2)
						{
							AP4_SampleDescription* desc = track->GetSampleDescription(sample.GetDescriptionIndex());

							dlgln = "0,0,Text,,0000,0000,0000,0000,," + str;
							dlgln_plaintext = str;

							CPoint translation(0, 0);
							if(AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD)))
							{
								AP4_Float x, y;
								tkhd->GetTranslation(x, y);
								translation.SetPoint((int)x, (int)y);
							}

							if(AP4_UnknownSampleDescription* unknown_desc = dynamic_cast<AP4_UnknownSampleDescription*>(desc)) // TEMP
							{
								AP4_SampleEntry* sample_entry = unknown_desc->GetSampleEntry();

								if(AP4_TextSampleEntry* text = dynamic_cast<AP4_TextSampleEntry*>(sample_entry))
								{
									const AP4_TextSampleEntry::AP4_TextDescription& d = text->GetDescription();

									// TODO
								}
								else if(AP4_Tx3gSampleEntry* tx3g = dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry))
								{
									const AP4_Tx3gSampleEntry::AP4_Tx3gDescription& desc = tx3g->GetDescription();

									CStringW font = L"Arial";

									if(AP4_FtabAtom* ftab = dynamic_cast<AP4_FtabAtom*>(tx3g->GetChild(AP4_ATOM_TYPE_FTAB)))
									{
										AP4_String Name;
										if(AP4_SUCCEEDED(ftab->LookupFont(desc.Style.Font.Id, Name)))
											font = Name.c_str();
									}					

									CRect rbox;
									CStringW ssa = ConvertTX3GToSSA(
										UTF8To16(str), desc, font,
										ptr + (2 + size), avail - (2 + size), 
										m_framesize, translation, 
										(p->rtStop - p->rtStart)/10000,
										rbox);
									dlgln = UTF16To8(ssa);

									const AP4_Byte* bclr = (const AP4_Byte*)&desc.BackgroundColor;

									if(bclr[3])
									{
										CPoint tl = rbox.TopLeft();
										rbox.OffsetRect(-tl.x, -tl.y);

										dlgln_bkg.Format(
											"0,-1,Text,,0,0,0,0,,{\\an7\\pos(%d,%d)\\1c%02x%02x%02x\\1a%02x\\bord0\\shad0}{\\p1}m %d %d l %d %d l %d %d l %d %d {\\p0}", 
											tl.x, tl.y, 
											bclr[2], bclr[1], bclr[0], 
											255 - bclr[3],
											rbox.left, rbox.top, 
											rbox.right, rbox.top,
											rbox.right, rbox.bottom,
											rbox.left, rbox.bottom);
									}
								}
							}
						}

						dlgln.Replace("\r", "");
						dlgln.Replace("\n", "\\N");

						p->SetData((LPCSTR)dlgln, dlgln.GetLength());
					}
				}

				if(!dlgln_bkg.IsEmpty())
				{
					CAutoPtr<Packet> p2(new Packet());
					p2->TrackNumber = p->TrackNumber;
					p2->rtStart = p->rtStart;
					p2->rtStop = p->rtStop;
					p2->bSyncPoint = p->bSyncPoint;
					p2->SetData((LPCSTR)dlgln_bkg, dlgln_bkg.GetLength());
					hr = DeliverPacket(p2);
				}

				if(!dlgln_plaintext.IsEmpty())
				{
					CAutoPtr<Packet> p2(new Packet());
					p2->TrackNumber = p->TrackNumber ^ 0x80402010;
					p2->rtStart = p->rtStart;
					p2->rtStop = p->rtStop;
					p2->bSyncPoint = p->bSyncPoint;
					p2->SetData((LPCSTR)dlgln_plaintext, dlgln_plaintext.GetLength());
					hr = DeliverPacket(p2);
				}
			}
*/
			else
			{
				const CMediaType& mt = pin->CurrentMediaType();

				if(mt.subtype == MEDIASUBTYPE_H264_TRANSPORT)
				{
					p->buff.resize(data.GetDataSize() + 4);

					BYTE* dst = &p->buff[0];

					for(int i = 0; i < 3; i++) dst[i] = 0;

					dst[3] = 1;

					memcpy(&dst[4], data.GetData() + 2, data.GetDataSize() - 2);
				}
				else if(mt.subtype == FOURCCMap('1cva') || mt.subtype == FOURCCMap('1CVA'))
				{
					MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.pbFormat;

					DWORD n = vih->hdr.dwReserved1;
					DWORD m = vih->dwFlags;

					p->buff.resize(data.GetDataSize() + m - n);

					BYTE* dst = &p->buff[0];

					for(int i = 0; i < m - n; i++)
					{
						dst[i] = 0;
					}

					memcpy(&dst[m - n], data.GetData(), data.GetDataSize());
				}
				else
				{
					p->SetData(data.GetData(), data.GetDataSize());
				}
			}

			hr = DeliverPacket(p);
		}

		{
			AP4_Sample sample;

			if(AP4_SUCCEEDED(track->GetSample(++iNext->second.index, sample)))
			{
				iNext->second.ts = sample.GetCts();
			}
		}
/*
		{
			bool gotenough = false;

			int index = 0;

			for(auto i = m_pActivePins.begin(); i != m_pActivePins.end(); i++)
			{
				printf("[%i] %I64d\n", index++, (*i)->QueueDuration());

				if((*i)->QueueDuration() >= 10000000)
				{
					gotenough = true;

					break;
				}
			}

			if(!paused)
			{
				if(!gotenough)
				{
					NotifyEvent(EC_USER, EC_USER_MAGIC, EC_USER_PAUSE);

					paused = true;
				}
			}
			else
			{
				if(gotenough)
				{
					NotifyEvent(EC_USER, EC_USER_MAGIC, EC_USER_RESUME);

					paused = false;
				}
			}
		}
*/
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CMP4SplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_file, E_UNEXPECTED);

	AP4_Movie* movie = (AP4_Movie*)m_file->GetMovie();

	for(auto i = m_trackpos.begin(); i != m_trackpos.end(); i++)
	{
		AP4_Track* track = movie->GetTrack(i->first);

		if(track->GetType() != AP4_Track::TYPE_VIDEO)
		{
			continue;
		}

		AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss"));

		if(stss == NULL)
		{
			continue;
		}

		nKFs = stss->m_Entries.ItemCount();
		
		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CMP4SplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_file, E_UNEXPECTED);

	if(*pFormat != TIME_FORMAT_MEDIA_TIME) 
	{
		return E_INVALIDARG;
	}

	AP4_Movie* movie = (AP4_Movie*)m_file->GetMovie();

	for(auto i = m_trackpos.begin(); i != m_trackpos.end(); i++)
	{
		AP4_Track* track = movie->GetTrack(i->first);

		if(track->GetType() != AP4_Track::TYPE_VIDEO)
		{
			continue;
		}

		AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss"));

		if(stss == NULL)
		{
			continue;
		}

		nKFs = 0;

		for(AP4_Cardinal i = 0; i < stss->m_Entries.ItemCount(); i++)
		{
			AP4_Sample sample;

			if(AP4_SUCCEEDED(track->GetSample(stss->m_Entries[i] - 1, sample)))
			{
				pKFs[nKFs++] = 10000000i64 * sample.GetCts() / track->GetMediaTimeScale();
			}
		}

		return S_OK;
	}

	return E_FAIL;
}

// CMP4SourceFilter

CMP4SourceFilter::CMP4SourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMP4SplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}
