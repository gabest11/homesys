#include "StdAfx.h"
#include "DirectShow.h"
#include "MediaTypeEx.h"
#include <initguid.h>
#include "moreuuids.h"

CMediaTypeEx::CMediaTypeEx()
{
}

CMediaTypeEx::CMediaTypeEx(const AM_MEDIA_TYPE& mt)
	: CMediaType(mt)
{
}

CMediaTypeEx::CMediaTypeEx(IPin* pPin)
{
	AM_MEDIA_TYPE mt;

	memset(&mt, 0, sizeof(mt));

	if(pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt)))
	{
		CopyMediaType(this, &mt);

		FreeMediaType(mt);
	}
}

std::wstring CMediaTypeEx::ToString(IPin* pPin)
{
	std::wstring packing, type, codec, dim, rate, dur;

	// TODO

	if(majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
	{
		packing = L"Encrypted MPEG2 Pack";
	}
	else if(majortype == MEDIATYPE_MPEG2_PACK)
	{
		packing = L"MPEG2 Pack";
	}
	else if(majortype == MEDIATYPE_MPEG2_PES)
	{
		packing = L"MPEG2 PES";
	}
	
	if(majortype == MEDIATYPE_Video)
	{
		type = L"Video";

		BITMAPINFOHEADER bih;

		if(ExtractBIH(&bih) && bih.biCompression != 0)
		{
			codec = GetVideoCodecName(subtype, bih.biCompression);
		}

		if(codec.empty())
		{
			if(formattype == FORMAT_MPEGVideo) codec = L"MPEG1 Video";
			else if(formattype == FORMAT_MPEG2_VIDEO) codec = L"MPEG2 Video";
			else if(formattype == FORMAT_DiracVideoInfo) codec = L"Dirac Video";
		}

		Vector4i v;

		if(ExtractDim(v))
		{
			dim = Util::Format(L"%dx%d", v.x, v.y);

			if(v.x * v.w != v.y * v.z) 
			{
				dim = Util::Format(L"%s (%d:%d)", dim.c_str(), v.z, v.w);
			}
		}

		if(formattype == FORMAT_VideoInfo || formattype == FORMAT_MPEGVideo)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pbFormat;

			if(vih->AvgTimePerFrame) 
			{
				rate = Util::Format(L"%0.2f fps ", 10000000.0f / vih->AvgTimePerFrame);
			}

			if(vih->dwBitRate)
			{
				rate += Util::Format(L"%d Kbps", vih->dwBitRate / 1000);
			}
		}
		else if(formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO || formattype == FORMAT_DiracVideoInfo)
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pbFormat;

			if(vih->AvgTimePerFrame)
			{
				rate = Util::Format(L"%0.2f fps ", 10000000.0f / vih->AvgTimePerFrame);
			}

			if(vih->dwBitRate)
			{
				rate += Util::Format(L"%d Kbps", vih->dwBitRate / 1000);
			}
		}

		rate = Util::Trim(rate);

		if(subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
		{
			type = L"Subtitle";
			codec = L"DVD Subpicture";
		}
	}
	else if(majortype == MEDIATYPE_Audio)
	{
		type = L"Audio";

		if(formattype == FORMAT_WaveFormatEx)
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)Format();

			if(wfe->wFormatTag/* > WAVE_FORMAT_PCM && wfe->wFormatTag < WAVE_FORMAT_EXTENSIBLE
			&& wfe->wFormatTag != WAVE_FORMAT_IEEE_FLOAT*/
			|| subtype != GUID_NULL)
			{
				codec = GetAudioCodecName(subtype, wfe->wFormatTag);
				dim = Util::Format(L"%d Hz", wfe->nSamplesPerSec);
				
				if(wfe->nChannels == 1) dim += L" mono";
				else if(wfe->nChannels == 2) dim += L" stereo";
				else dim += Util::Format(L" %d channels", wfe->nChannels);
				
				if(wfe->nAvgBytesPerSec) rate = Util::Format(L"%d Kbps", (wfe->nAvgBytesPerSec * 8 + 500) / 1000);
			}
		}
		else if(formattype == FORMAT_VorbisFormat)
		{
			VORBISFORMAT* vf = (VORBISFORMAT*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim = Util::Format(L"%d Hz", vf->nSamplesPerSec);

			if(vf->nChannels == 1) dim += L" mono";
			else if(vf->nChannels == 2) dim += L" stereo";
			else dim += Util::Format(L" %d channels", vf->nChannels);

			if(vf->nAvgBitsPerSec) rate = Util::Format(L"%d Kbps", (vf->nAvgBitsPerSec + 500) / 1000);
		}
		else if(formattype == FORMAT_VorbisFormat2)
		{
			VORBISFORMAT2* vf = (VORBISFORMAT2*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim = Util::Format(L"%d Hz", vf->SamplesPerSec);

			if(vf->Channels == 1) dim += L" mono";
			else if(vf->Channels == 2) dim += L" stereo";
			else dim += Util::Format(L" %d channels", vf->Channels);
		}				
	}
	else if(majortype == MEDIATYPE_Text)
	{
		type = L"Text";
	}
	else if(majortype == MEDIATYPE_Subtitle)
	{
		type = L"Subtitle";

		codec = GetSubtitleCodecName(subtype);
	}
	else if(majortype == MEDIATYPE_VBI)
	{
		type = L"VBI";

		// codec = GetVBICodecName(subtype); // TODO
	}
	else if(majortype == MEDIATYPE_Stream)
	{
		type = L"Stream";
	}
	else
	{
		type = L"Unknown";
	}

	if(CComQIPtr<IMediaSeeking> pMS = pPin)
	{
		REFERENCE_TIME rtDur = 0;

		if(SUCCEEDED(pMS->GetDuration(&rtDur)) && rtDur > 0)
		{
			rtDur /= 10000000;
			int s = (int)(rtDur % 60);
			rtDur /= 60;
			int m = (int)(rtDur % 60);
			rtDur /= 60;
			int h = (int)(rtDur);

			if(h) dur = Util::Format(L"%d:%02d:%02d", h, m, s);
			else if(m) dur = Util::Format(L"%02d:%02d", m, s);
			else if(s) dur = Util::Format(L"%ds", s);
		}
	}

	std::wstring str;

	if(!codec.empty()) str += codec + L" ";
	if(!dim.empty()) str += dim + L" ";
	if(!rate.empty()) str += rate + L" ";
	if(!dur.empty()) str += dur + L" ";

	str = Util::Trim(str, L" ,");
	
	if(!str.empty()) str = /*type + L": ") +*/ str;
	else str = type;

	return str;
}

std::wstring CMediaTypeEx::GetVideoCodecName(const GUID& subtype, DWORD biCompression)
{
	static std::unordered_map<DWORD, std::wstring> names;

	if(names.empty())
	{
		names['WMV1'] = L"WMV 7";
		names['WMV2'] = L"WMV 8";
		names['WMV3'] = L"WMV 9";
		names['DIV3'] = L"DivX 3";
		names['DX50'] = L"DivX 5";
		names['MP4V'] = L"MPEG4 Video";
		names['AVC1'] = L"H.264";
		names['H264'] = L"H.264";
		names['RV10'] = L"RealVideo 1";
		names['RV20'] = L"RealVideo 2";
		names['RV30'] = L"RealVideo 3";
		names['RV40'] = L"RealVideo 4";
		names['FLV1'] = L"Flash Video 1";
		// names[''] = L"";
	}

	std::wstring str;

	if(biCompression < 256) str = Util::Format(L"%d", biCompression);
	else str = Util::Format(L"%4.4hs", &biCompression);

	if(subtype == MEDIASUBTYPE_DiracVideo) str = L"Dirac Video";
	// else if(subtype == ) str = "";
	
	if(biCompression > 0)
	{
		BYTE* b = (BYTE*)&biCompression;

		for(int i = 0; i < 4; i++)
		{
			if(b[i] >= 'a' && b[i] <= 'z') 
			{
				b[i] = toupper(b[i]);
			}
		}

		auto i = names.find(MAKEFOURCC(b[3], b[2], b[1], b[0]));

		if(i != names.end())
		{
			str = i->second;
		}
	}

	return str;
}

std::wstring CMediaTypeEx::GetAudioCodecName(const GUID& subtype, WORD wFormatTag)
{
	static std::unordered_map<WORD, std::wstring> names;

	if(names.empty())
	{
		names[WAVE_FORMAT_PCM] = L"PCM";
		names[WAVE_FORMAT_EXTENSIBLE] = L"PCM";
		names[WAVE_FORMAT_IEEE_FLOAT] = L"IEEE Float";
		names[WAVE_FORMAT_ADPCM] = L"MS ADPCM";
		names[WAVE_FORMAT_ALAW] = L"aLaw";
		names[WAVE_FORMAT_MULAW] = L"muLaw";
		names[WAVE_FORMAT_DRM] = L"DRM";
		names[WAVE_FORMAT_OKI_ADPCM] = L"OKI ADPCM";
		names[WAVE_FORMAT_DVI_ADPCM] = L"DVI ADPCM";
		names[WAVE_FORMAT_IMA_ADPCM] = L"IMA ADPCM";
		names[WAVE_FORMAT_MEDIASPACE_ADPCM] = L"Mediaspace ADPCM";
		names[WAVE_FORMAT_SIERRA_ADPCM] = L"Sierra ADPCM";
		names[WAVE_FORMAT_G723_ADPCM] = L"G723 ADPCM";
		names[WAVE_FORMAT_DIALOGIC_OKI_ADPCM] = L"Dialogic OKI ADPCM";
		names[WAVE_FORMAT_MEDIAVISION_ADPCM] = L"Media Vision ADPCM";
		names[WAVE_FORMAT_YAMAHA_ADPCM] = L"Yamaha ADPCM";
		names[WAVE_FORMAT_DSPGROUP_TRUESPEECH] = L"DSP Group Truespeech";
		names[WAVE_FORMAT_DOLBY_AC2] = L"Dolby AC2";
		names[WAVE_FORMAT_GSM610] = L"GSM610";
		names[WAVE_FORMAT_MSNAUDIO] = L"MSN Audio";
		names[WAVE_FORMAT_ANTEX_ADPCME] = L"Antex ADPCME";
		names[WAVE_FORMAT_CS_IMAADPCM] = L"Crystal Semiconductor IMA ADPCM";
		names[WAVE_FORMAT_ROCKWELL_ADPCM] = L"Rockwell ADPCM";
		names[WAVE_FORMAT_ROCKWELL_DIGITALK] = L"Rockwell Digitalk";
		names[WAVE_FORMAT_G721_ADPCM] = L"G721";
		names[WAVE_FORMAT_G728_CELP] = L"G728";
		names[WAVE_FORMAT_MSG723] = L"MSG723";
		names[WAVE_FORMAT_MPEG] = L"MPEG Audio";
		names[WAVE_FORMAT_MPEGLAYER3] = L"MP3";
		names[WAVE_FORMAT_LUCENT_G723] = L"Lucent G723";
		names[WAVE_FORMAT_VOXWARE] = L"Voxware";
		names[WAVE_FORMAT_G726_ADPCM] = L"G726";
		names[WAVE_FORMAT_G722_ADPCM] = L"G722";
		names[WAVE_FORMAT_G729A] = L"G729A";
		names[WAVE_FORMAT_MEDIASONIC_G723] = L"MediaSonic G723";
		names[WAVE_FORMAT_ZYXEL_ADPCM] = L"ZyXEL ADPCM";
		names[WAVE_FORMAT_RHETOREX_ADPCM] = L"Rhetorex ADPCM";
		names[WAVE_FORMAT_VIVO_G723] = L"Vivo G723";
		names[WAVE_FORMAT_VIVO_SIREN] = L"Vivo Siren";
		names[WAVE_FORMAT_DIGITAL_G723] = L"Digital G723";
		names[WAVE_FORMAT_SANYO_LD_ADPCM] = L"Sanyo LD ADPCM";
		names[WAVE_FORMAT_CREATIVE_ADPCM] = L"Creative ADPCM";
		names[WAVE_FORMAT_CREATIVE_FASTSPEECH8] = L"Creative Fastspeech 8";
		names[WAVE_FORMAT_CREATIVE_FASTSPEECH10] = L"Creative Fastspeech 10";
		names[WAVE_FORMAT_UHER_ADPCM] = L"UHER ADPCM";
		names[WAVE_FORMAT_DOLBY_AC3] = L"Dolby AC3";
		names[WAVE_FORMAT_DVD_DTS] = L"DTS";
		names[WAVE_FORMAT_AAC] = L"AAC";
		names[WAVE_FORMAT_LATM] = L"AAC (LATM)";
		names[WAVE_FORMAT_FLAC] = L"FLAC";
		names[WAVE_FORMAT_TTA1] = L"TTA";
		names[WAVE_FORMAT_14_4] = L"RealAudio 14.4";
		names[WAVE_FORMAT_28_8] = L"RealAudio 28.8";
		names[WAVE_FORMAT_ATRC] = L"RealAudio ATRC";
		names[WAVE_FORMAT_COOK] = L"RealAudio COOK";
		names[WAVE_FORMAT_DNET] = L"RealAudio DNET";
		names[WAVE_FORMAT_RAAC] = L"RealAudio RAAC";
		names[WAVE_FORMAT_RACP] = L"RealAudio RACP";
		names[WAVE_FORMAT_SIPR] = L"RealAudio SIPR";
		names[WAVE_FORMAT_PS2_PCM] = L"PS2 PCM";
		names[WAVE_FORMAT_PS2_ADPCM] = L"PS2 ADPCM";
		names[0x0160] = L"WMA";
		names[0x0161] = L"WMA";
		names[0x0162] = L"WMA";
		names[0x0163] = L"WMA";
		// names[] = L"";
	}

	std::wstring str = Util::Format(L"0x%04x", wFormatTag);

	if(subtype == MEDIASUBTYPE_Vorbis) str = L"Vorbis (deprecated)";
	else if(subtype == MEDIASUBTYPE_Vorbis2) str = L"Vorbis";
	else if(subtype == MEDIASUBTYPE_MP4A) str = L"MPEG4 Audio";
	else if(subtype == MEDIASUBTYPE_FLAC_FRAMED) str = L"FLAC (framed)";
	else if(subtype == MEDIASUBTYPE_DOLBY_AC3) str += L"Dolby AC3";
	else if(subtype == MEDIASUBTYPE_DTS) str += L"DTS";
	else if(subtype == FOURCCMap(MAKEFOURCC('A','S','W','F'))) str = L"Flash ADPCM";
	// else if(subtype == ) str = L"";

	auto i = names.find(wFormatTag);

	if(i != names.end())
	{
		str = i->second;
	}

	if(wFormatTag == WAVE_FORMAT_PCM)
	{
		if(subtype == MEDIASUBTYPE_DOLBY_AC3) str += L" (AC3)";
		else if(subtype == MEDIASUBTYPE_DTS) str += L" (DTS)";
	}

	return str;
}

std::wstring CMediaTypeEx::GetSubtitleCodecName(const GUID& subtype)
{
	std::wstring str;

	if(subtype == MEDIASUBTYPE_UTF8)
	{
		str = L"UTF-8";
	}
	else if(subtype == MEDIASUBTYPE_SSA)
	{
		str = L"SubStation Alpha";
	}
	else if(subtype == MEDIASUBTYPE_ASS)
	{
		str = L"Advanced SubStation Alpha";
	}
	else if(subtype == MEDIASUBTYPE_ASS2)
	{
		str = L"Advanced SubStation Alpha";
	}
	else if(subtype == MEDIASUBTYPE_SSF)
	{
		str = L"Stuctured Subtitle Format";
	}
	else if(subtype == MEDIASUBTYPE_USF)
	{
		str = L"Universal Subtitle Format";
	}
	else if(subtype == MEDIASUBTYPE_VOBSUB)
	{
		str = L"VobSub";
	}

	return str;
}

void CMediaTypeEx::Dump(std::list<std::wstring>& sl)
{
	std::wstring str;

	sl.clear();

	int fmtsize = 0;

	std::wstring major;
	std::wstring sub;
	std::wstring format;

	Util::ToString(majortype, major);
	Util::ToString(subtype, sub);
	Util::ToString(formattype, format);

	sl.push_back(ToString() + L"\n");
	sl.push_back(L"AM_MEDIA_TYPE: ");
	sl.push_back(Util::Format(L"majortype: %s %s", CString(GuidNames[majortype]), major));
	sl.push_back(Util::Format(L"subtype: %s %s", CString(GuidNames[subtype]), sub));
	sl.push_back(Util::Format(L"formattype: %s %s", CString(GuidNames[formattype]), format));
	sl.push_back(Util::Format(L"bFixedSizeSamples: %d", bFixedSizeSamples));
	sl.push_back(Util::Format(L"bTemporalCompression: %d", bTemporalCompression));
	sl.push_back(Util::Format(L"lSampleSize: %d", lSampleSize));
	sl.push_back(Util::Format(L"cbFormat: %d", cbFormat));
	sl.push_back(L"");

	if(formattype == FORMAT_VideoInfo || formattype == FORMAT_VideoInfo2
	|| formattype == FORMAT_MPEGVideo || formattype == FORMAT_MPEG2_VIDEO)
	{
		fmtsize = 
			formattype == FORMAT_VideoInfo ? sizeof(VIDEOINFOHEADER) :
			formattype == FORMAT_VideoInfo2 ? sizeof(VIDEOINFOHEADER2) :
			formattype == FORMAT_MPEGVideo ? sizeof(MPEG1VIDEOINFO) - 1 :
			formattype == FORMAT_MPEG2_VIDEO ? sizeof(MPEG2VIDEOINFO) - 4 :
			0;

		VIDEOINFOHEADER& vih = *(VIDEOINFOHEADER*)pbFormat;
		BITMAPINFOHEADER* bih = &vih.bmiHeader;

		sl.push_back(L"VIDEOINFOHEADER:");
		sl.push_back(Util::Format(L"rcSource: (%d,%d)-(%d,%d)", vih.rcSource));
		sl.push_back(Util::Format(L"rcTarget: (%d,%d)-(%d,%d)", vih.rcTarget));
		sl.push_back(Util::Format(L"dwBitRate: %d", vih.dwBitRate));
		sl.push_back(Util::Format(L"dwBitErrorRate: %d", vih.dwBitErrorRate));
		sl.push_back(Util::Format(L"AvgTimePerFrame: %I64d", vih.AvgTimePerFrame));
		sl.push_back(L"");

		if(formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO)
		{
			VIDEOINFOHEADER2& vih = *(VIDEOINFOHEADER2*)pbFormat;
			bih = &vih.bmiHeader;

			sl.push_back(L"VIDEOINFOHEADER2:");
			sl.push_back(Util::Format(L"dwInterlaceFlags: 0x%08x", vih.dwInterlaceFlags));
			sl.push_back(Util::Format(L"dwCopyProtectFlags: 0x%08x", vih.dwCopyProtectFlags));
			sl.push_back(Util::Format(L"dwPictAspectRatioX: %d", vih.dwPictAspectRatioX));
			sl.push_back(Util::Format(L"dwPictAspectRatioY: %d", vih.dwPictAspectRatioY));
			sl.push_back(Util::Format(L"dwControlFlags: 0x%08x", vih.dwControlFlags));
			sl.push_back(Util::Format(L"dwReserved2: 0x%08x", vih.dwReserved2));
			sl.push_back(L"");
		}

		if(formattype == FORMAT_MPEGVideo)
		{
			MPEG1VIDEOINFO& mvih = *(MPEG1VIDEOINFO*)pbFormat;

			sl.push_back(L"MPEG1VIDEOINFO:");
			sl.push_back(Util::Format(L"dwStartTimeCode: %d", mvih.dwStartTimeCode));
			sl.push_back(Util::Format(L"cbSequenceHeader: %d", mvih.cbSequenceHeader));
			sl.push_back(L"");
		}
		else if(formattype == FORMAT_MPEG2_VIDEO)
		{
			MPEG2VIDEOINFO& mvih = *(MPEG2VIDEOINFO*)pbFormat;

			sl.push_back(L"MPEG2VIDEOINFO:");
			sl.push_back(Util::Format(L"dwStartTimeCode: %d", mvih.dwStartTimeCode));
			sl.push_back(Util::Format(L"cbSequenceHeader: %d", mvih.cbSequenceHeader));
			sl.push_back(Util::Format(L"dwProfile: 0x%08x", mvih.dwProfile));
			sl.push_back(Util::Format(L"dwLevel: 0x%08x", mvih.dwLevel));
			sl.push_back(Util::Format(L"dwFlags: 0x%08x", mvih.dwFlags));
			sl.push_back(L"");
		}

		sl.push_back(L"BITMAPINFOHEADER:");
		sl.push_back(Util::Format(L"biSize: %d", bih->biSize));
		sl.push_back(Util::Format(L"biWidth: %d", bih->biWidth));
		sl.push_back(Util::Format(L"biHeight: %d", bih->biHeight));
		sl.push_back(Util::Format(L"biPlanes: %d", bih->biPlanes));
		sl.push_back(Util::Format(L"biBitCount: %d", bih->biBitCount));
		if(bih->biCompression < 256) str = Util::Format(L"biCompression: %d", bih->biCompression);
		else str = Util::Format(L"biCompression: %4.4hs", &bih->biCompression);
		sl.push_back(str);
		sl.push_back(Util::Format(L"biSizeImage: %d", bih->biSizeImage));
		sl.push_back(Util::Format(L"biXPelsPerMeter: %d", bih->biXPelsPerMeter));
		sl.push_back(Util::Format(L"biYPelsPerMeter: %d", bih->biYPelsPerMeter));
		sl.push_back(Util::Format(L"biClrUsed: %d", bih->biClrUsed));
		sl.push_back(Util::Format(L"biClrImportant: %d", bih->biClrImportant));
		sl.push_back(L"");
    }
	else if(formattype == FORMAT_WaveFormatEx)
	{
		fmtsize = sizeof(WAVEFORMATEX);

		WAVEFORMATEX& wfe = *(WAVEFORMATEX*)pbFormat;

		sl.push_back(L"WAVEFORMATEX:");
		sl.push_back(Util::Format(L"wFormatTag: 0x%04x", wfe.wFormatTag));
		sl.push_back(Util::Format(L"nChannels: %d", wfe.nChannels));
		sl.push_back(Util::Format(L"nSamplesPerSec: %d", wfe.nSamplesPerSec));
		sl.push_back(Util::Format(L"nAvgBytesPerSec: %d", wfe.nAvgBytesPerSec));
		sl.push_back(Util::Format(L"nBlockAlign: %d", wfe.nBlockAlign));
		sl.push_back(Util::Format(L"wBitsPerSample: %d", wfe.wBitsPerSample));
		sl.push_back(Util::Format(L"cbSize: %d (extra bytes)", wfe.cbSize));
		sl.push_back(L"");

		if(wfe.wFormatTag != WAVE_FORMAT_PCM && wfe.cbSize > 0)
		{
			if(wfe.wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe.cbSize == sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
			{
				fmtsize = sizeof(WAVEFORMATEXTENSIBLE);

				WAVEFORMATEXTENSIBLE& wfe = *(WAVEFORMATEXTENSIBLE*)pbFormat;

				sl.push_back(L"WAVEFORMATEXTENSIBLE:");
				if(wfe.Format.wBitsPerSample != 0) str = Util::Format(L"wValidBitsPerSample: %d", wfe.Samples.wValidBitsPerSample);
				else str = Util::Format(L"wSamplesPerBlock: %d", wfe.Samples.wSamplesPerBlock);
				sl.push_back(str);
				sl.push_back(Util::Format(L"dwChannelMask: 0x%08x", wfe.dwChannelMask));
				Util::ToString(wfe.SubFormat, str);
				sl.push_back(Util::Format(L"SubFormat: %s", str));
				sl.push_back(L"");
			}
			else if(wfe.wFormatTag == WAVE_FORMAT_DOLBY_AC3 && wfe.cbSize == sizeof(DOLBYAC3WAVEFORMAT)-sizeof(WAVEFORMATEX))
			{
				fmtsize = sizeof(DOLBYAC3WAVEFORMAT);

				DOLBYAC3WAVEFORMAT& wfe = *(DOLBYAC3WAVEFORMAT*)pbFormat;

				sl.push_back(L"DOLBYAC3WAVEFORMAT:");
				sl.push_back(Util::Format(L"bBigEndian: %d", wfe.bBigEndian));
				sl.push_back(Util::Format(L"bsid: %d", wfe.bsid));
				sl.push_back(Util::Format(L"lfeon: %d", wfe.lfeon));
				sl.push_back(Util::Format(L"copyrightb: %d", wfe.copyrightb));
				sl.push_back(Util::Format(L"nAuxBitsCode: %d", wfe.nAuxBitsCode));
				sl.push_back(L"");
			}
		}
	}
	else if(formattype == FORMAT_VorbisFormat)
	{
		fmtsize = sizeof(VORBISFORMAT);

		VORBISFORMAT& vf = *(VORBISFORMAT*)pbFormat;

		sl.push_back(L"VORBISFORMAT:");
		sl.push_back(Util::Format(L"nChannels: %d", vf.nChannels));
		sl.push_back(Util::Format(L"nSamplesPerSec: %d", vf.nSamplesPerSec));
		sl.push_back(Util::Format(L"nMinBitsPerSec: %d", vf.nMinBitsPerSec));
		sl.push_back(Util::Format(L"nAvgBitsPerSec: %d", vf.nAvgBitsPerSec));
		sl.push_back(Util::Format(L"nMaxBitsPerSec: %d", vf.nMaxBitsPerSec));
		sl.push_back(Util::Format(L"fQuality: %.3f", vf.fQuality));
		sl.push_back(L"");
	}
	else if(formattype == FORMAT_VorbisFormat2)
	{
		fmtsize = sizeof(VORBISFORMAT2);

		VORBISFORMAT2& vf = *(VORBISFORMAT2*)pbFormat;

		sl.push_back(L"VORBISFORMAT:");
		sl.push_back(Util::Format(L"Channels: %d", vf.Channels));
		sl.push_back(Util::Format(L"SamplesPerSec: %d", vf.SamplesPerSec));
		sl.push_back(Util::Format(L"BitsPerSample: %d", vf.BitsPerSample));
		sl.push_back(Util::Format(L"HeaderSize: {%d, %d, %d}", vf.HeaderSize[0], vf.HeaderSize[1], vf.HeaderSize[2]));
		sl.push_back(L"");
	}
	else if(formattype == FORMAT_SubtitleInfo)
	{
		fmtsize = sizeof(SUBTITLEINFO);

		SUBTITLEINFO& si = *(SUBTITLEINFO*)pbFormat;

		sl.push_back(L"SUBTITLEINFO:");
		sl.push_back(Util::Format(L"dwOffset: %d", si.dwOffset));
		sl.push_back(Util::Format(L"IsoLang: %s", CString(CStringA(si.IsoLang, sizeof(si.IsoLang) - 1))));
		sl.push_back(Util::Format(L"TrackName: %s", CString(CStringW(si.TrackName, sizeof(si.TrackName) - 1))));
		sl.push_back(L"");
	}

	if(cbFormat > 0)
	{
		sl.push_back(L"pbFormat:");

		for(int i = 0, j = (cbFormat + 15) & ~15; i < j; i += 16)
		{
			str = Util::Format(L"%04x:", i);

			for(int k = i, l = min(i + 16, (int)cbFormat); k < l; k++)
			{
				str += Util::Format(L"%c%02x", fmtsize > 0 && fmtsize == k ? '|' : ' ', pbFormat[k]);
			}

			for(int k = min(i + 16, (int)cbFormat), l = i + 16; k < l; k++)
			{
				str += L"   ";
			}

			str += ' ';

			for(int k = i, l = min(i + 16, (int)cbFormat); k < l; k++)
			{
				unsigned char c = (unsigned char)pbFormat[k];

				str += Util::Format(L"%c", c >= 0x20 ? c : '.');
			}

			sl.push_back(str);
		}

		sl.push_back(L"");
	}
}

bool CMediaTypeEx::ExtractBIH(BITMAPINFOHEADER* bih)
{
	if(bih == NULL)
	{
		return false;
	}

	memset(bih, 0, sizeof(*bih));

	if(formattype == FORMAT_VideoInfo)
	{
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pbFormat;

		memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));

		return true;
	}
	else if(formattype == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pbFormat;

		memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));

		return true;
	}
	else if(formattype == FORMAT_MPEGVideo)
	{
		VIDEOINFOHEADER* vih = &((MPEG1VIDEOINFO*)pbFormat)->hdr;

		memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));

		return true;
	}
	else if(formattype == FORMAT_MPEG2_VIDEO)
	{
		VIDEOINFOHEADER2* vih = &((MPEG2VIDEOINFO*)pbFormat)->hdr;

		memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));

		return true;
	}
	else if(formattype == FORMAT_DiracVideoInfo)
	{
		VIDEOINFOHEADER2* vih = &((DIRACINFOHEADER*)pbFormat)->hdr;

		memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));

		return true;
	}
	
	return false;
}

bool CMediaTypeEx::ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih)
{
	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED(pMS->GetMediaType(&pmt)) && pmt)
	{
		CMediaTypeEx mt(*pmt);

		bool ret = mt.ExtractBIH(bih);

		DeleteMediaType(pmt);

		return ret;
	}
	
	return false;
}

bool CMediaTypeEx::ExtractDim(Vector4i& dim)
{
	dim = Vector4i::zero();

	if(formattype == FORMAT_VideoInfo || formattype == FORMAT_MPEGVideo)
	{
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pbFormat;

		dim.x = vih->bmiHeader.biWidth;
		dim.y = abs(vih->bmiHeader.biHeight);
		dim.z = dim.x * vih->bmiHeader.biYPelsPerMeter;
		dim.w = dim.y * vih->bmiHeader.biXPelsPerMeter;
	}
	else if(formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO || formattype == FORMAT_DiracVideoInfo)
	{
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pbFormat;

		dim.x = vih->bmiHeader.biWidth;
		dim.y = abs(vih->bmiHeader.biHeight);
		dim.z = vih->dwPictAspectRatioX;
		dim.w = vih->dwPictAspectRatioY;
	}
	else
	{
		return false;
	}

	if((dim == Vector4i::zero()).mask() & 0xff00)
	{
		dim = dim.xyxy();
	}

	BYTE* ptr = NULL;
	DWORD len = 0;

	if(formattype == FORMAT_MPEGVideo)
	{
		ptr = ((MPEG1VIDEOINFO*)pbFormat)->bSequenceHeader;
		len = ((MPEG1VIDEOINFO*)pbFormat)->cbSequenceHeader;

		if(ptr && len >= 8 && *(DWORD*)ptr == 0xb3010000)
		{
			dim.x = (ptr[4] << 4) | (ptr[5] >> 4);
			dim.y = ((ptr[5] & 0xf) << 8) | ptr[6];

			static float ar_presets[] = 
			{
				1.0000f, 1.0000f, 0.6735f, 0.7031f,
				0.7615f, 0.8055f, 0.8437f, 0.8935f,
				0.9157f, 0.9815f, 1.0255f, 1.0695f,
				1.0950f, 1.1575f, 1.2015f, 1.0000f,
			};

			dim.z = (int)((float)dim.x / ar_presets[ptr[7] >> 4] + 0.5);
			dim.w = dim.y;
		}
	}
	else if(formattype == FORMAT_MPEG2_VIDEO)
	{
		ptr = (BYTE*)((MPEG2VIDEOINFO*)pbFormat)->dwSequenceHeader; 
		len = ((MPEG2VIDEOINFO*)pbFormat)->cbSequenceHeader;

		if(ptr && len >= 8 && *(DWORD*)ptr == 0xb3010000)
		{
			dim.x = (ptr[4] << 4) | (ptr[5] >> 4);
			dim.y = ((ptr[5] & 0xf) << 8) | ptr[6];

			Vector2i ar_presets[5];
			
			ar_presets[0] = dim.tl;
			ar_presets[1] = Vector2i(4, 3);
			ar_presets[2] = Vector2i(16, 9);
			ar_presets[3] = Vector2i(221, 100);
			ar_presets[4] = dim.tl;

			int i = min(max(ptr[7] >> 4, 1), 5) - 1;

			dim.br = ar_presets[i];
		}
	}

	if(ptr && len >= 8)
	{

	}

	DWORD a = dim.z, b = dim.w;
    while(a) {int tmp = a; a = b % tmp; b = tmp;}
	if(b) {dim.z /= b; dim.w /= b;}

	return true;
}

#pragma pack(push, 1)

struct VIH
{
	VIDEOINFOHEADER vih;
	UINT mask[3];
	int size;
	const GUID* subtype;
};

struct VIH2
{
	VIDEOINFOHEADER2 vih;
	UINT mask[3];
	int size;
	const GUID* subtype;
};

#pragma pack(pop)

#define VIH_NORMAL (sizeof(VIDEOINFOHEADER))
#define VIH_BITFIELDS (sizeof(VIDEOINFOHEADER) + 3 * sizeof(RGBQUAD))
#define VIH2_NORMAL (sizeof(VIDEOINFOHEADER2))
#define VIH2_BITFIELDS (sizeof(VIDEOINFOHEADER2) + 3 * sizeof(RGBQUAD))
#define BIH_SIZE (sizeof(BITMAPINFOHEADER))

static const VIH s_vihs[] =
{
	// YUY2
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_YUY2												// subtype
	},
	// YV12
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 12, MAKEFOURCC('Y','V','1','2'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_YV12												// subtype
	},
	// I420
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 12, MAKEFOURCC('I','4','2','0'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_I420												// subtype
	},
	// IYUV
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 12, MAKEFOURCC('I','Y','U','V'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_IYUV												// subtype
	},
	// 8888 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_RGB32												// subtype
	},
	// 8888 bitf 
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xFF0000, 0x00FF00, 0x0000FF},									// mask[3]
		VIH_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB32												// subtype
	},
	// A888 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_ARGB32											// subtype
	},
	// A888 bitf (I'm not sure if this exist...)
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xFF0000, 0x00FF00, 0x0000FF},									// mask[3]
		VIH_BITFIELDS,													// size
		&MEDIASUBTYPE_ARGB32											// subtype
	},
	// 888 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 24, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_RGB24												// subtype
	},
	// 888 bitf 
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 24, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xFF0000, 0x00FF00, 0x0000FF},									// mask[3]
		VIH_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB24												// subtype
	},
	// 565 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_RGB565											// subtype
	},
	// 565 bitf
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xF800, 0x07E0, 0x001F},										// mask[3]
		VIH_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB565											// subtype
	},
	// 555 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH_NORMAL,														// size
		&MEDIASUBTYPE_RGB555											// subtype
	},
	// 555 bitf
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0x7C00, 0x03E0, 0x001F},										// mask[3]
		VIH_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB555											// subtype
	},
};

static const VIH2 s_vih2s[] =
{
	// YUY2
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_YUY2												// subtype
	},
	// YV12
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 12, MAKEFOURCC('Y','V','1','2'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_YV12												// subtype
	},
	// I420
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 12, MAKEFOURCC('I','4','2','0'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_I420												// subtype
	},
	// IYUV
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 12, MAKEFOURCC('I','Y','U','V'), 0, 0, 0, 0, 0}		// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_IYUV												// subtype
	},
	// 8888 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_RGB32												// subtype
	},
	// 8888 bitf 
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xFF0000, 0x00FF00, 0x0000FF},									// mask[3]
		VIH2_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB32												// subtype
	},
	// A888 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_ARGB32											// subtype
	},
	// A888 bitf (I'm not sure if this exist...)
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 32, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xFF0000, 0x00FF00, 0x0000FF},									// mask[3]
		VIH2_BITFIELDS,													// size
		&MEDIASUBTYPE_ARGB32											// subtype
	},
	// 888 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 24, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_RGB24												// subtype
	},
	// 888 bitf 
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 24, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xFF0000, 0x00FF00, 0x0000FF},									// mask[3]
		VIH2_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB24												// subtype
	},
	// 565 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_RGB565											// subtype
	},
	// 565 bitf
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0xF800, 0x07E0, 0x001F},										// mask[3]
		VIH2_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB565											// subtype
	},
	// 555 normal
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_RGB, 0, 0, 0, 0, 0}			// bmiHeader
		}, 
		{0, 0, 0},														// mask[3]
		VIH2_NORMAL,													// size
		&MEDIASUBTYPE_RGB555											// subtype
	},
	// 555 bitf
	{
		{					
			{0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			{BIH_SIZE, 0, 0, 1, 16, BI_BITFIELDS, 0, 0, 0, 0, 0}	// bmiHeader
		}, 
		{0x7C00, 0x03E0, 0x001F},										// mask[3]
		VIH2_BITFIELDS,													// size
		&MEDIASUBTYPE_RGB555											// subtype
	},
};

void CMediaTypeEx::FixMediaType()
{
	CMediaType mt(*this);

	if(formattype == FORMAT_VideoInfo)
	{
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pbFormat;

		for(int i = 0; i < sizeof(s_vihs) / sizeof(s_vihs[0]); i++)
		{
			if(subtype == *s_vihs[i].subtype && vih->bmiHeader.biCompression == s_vihs[i].vih.bmiHeader.biCompression)
			{
				AllocFormatBuffer(s_vihs[i].size);
				memcpy(pbFormat, &s_vihs[i], s_vihs[i].size);
				memcpy(pbFormat, mt.pbFormat, sizeof(VIDEOINFOHEADER));
				break;
			}
		}
	}
	else if(mt.formattype == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.pbFormat;

		for(int i = 0; i < sizeof(s_vih2s) / sizeof(s_vih2s[0]); i++)
		{
			if(subtype == *s_vih2s[i].subtype && vih2->bmiHeader.biCompression == s_vih2s[i].vih.bmiHeader.biCompression)
			{
				AllocFormatBuffer(s_vih2s[i].size);
				memcpy(pbFormat, &s_vih2s[i], s_vih2s[i].size);
				memcpy(pbFormat, mt.pbFormat, sizeof(VIDEOINFOHEADER2));
				break;
			}
		}
	}
}