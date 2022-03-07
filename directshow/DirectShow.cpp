#include "stdafx.h"
#include "DirectShow.h"
#include "DDK/devioctl.h"
#include "DDK/ntddcdrm.h"
#include <initguid.h>
#include "moreuuids.h"

IPin* DirectShow::GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
	IPin* first = NULL;

	Foreach(pBF, [&] (IPin* pin) -> HRESULT
	{
		PIN_DIRECTION d;

		if(SUCCEEDED(pin->QueryDirection(&d)) && d == dir)
		{
			first = pin;

			return S_OK;
		}

		return S_CONTINUE;
	});

	return first;
}

IPin* DirectShow::GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
	IPin* first = NULL;

	Foreach(pBF, [&] (IPin* pin) -> HRESULT
	{
		CComPtr<IPin> pinto;

		PIN_DIRECTION d;

		if(SUCCEEDED(pin->QueryDirection(&d)) && d == dir && pin->ConnectedTo(&pinto) && pinto == NULL)
		{
			first = pin;

			return S_OK;
		}

		return S_CONTINUE;
	});

	return first;
}

int DirectShow::GetPinCount(IBaseFilter* pBF, PIN_COUNT& count)
{
	memset(&count, 0, sizeof(count));

	Foreach(pBF, [&count] (IPin* pin) -> HRESULT
	{
		PIN_DIRECTION dir;

		if(SUCCEEDED(pin->QueryDirection(&dir)))
		{
			CComPtr<IPin> pinto;

			bool connected = SUCCEEDED(pin->ConnectedTo(&pinto)) && pinto != NULL;

			switch(dir)
			{
			case PINDIR_INPUT: 
				count.in++; 
				if(connected) count.connected.in++;
				else count.disconnected.in++;
				break;
			case PINDIR_OUTPUT: 
				count.out++; 
				if(connected) count.connected.out++;
				else count.disconnected.out++;
				break;
			}
		}

		return S_CONTINUE;
	});

	return count.in + count.out;
}

IBaseFilter* DirectShow::GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin)
{
	return GetFilter(GetUpStreamPin(pBF, pInputPin));
}

IPin* DirectShow::GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin)
{
	if(pInputPin == NULL)
	{
		pInputPin = GetFirstPin(pBF);
	}

	if(pInputPin != NULL)
	{
		CComPtr<IPin> pin;

		if(SUCCEEDED(pInputPin->ConnectedTo(&pin)) && pin != NULL)
		{
			return pin;
		}
	}

	return NULL;
}

IPin* DirectShow::GetConnected(IPin* pPin)
{
	CComPtr<IPin> pPinTo;

	pPin->ConnectedTo(&pPinTo);

	return pPinTo;
}

PIN_DIRECTION DirectShow::GetDirection(IPin* pPin)
{
	PIN_DIRECTION dir = (PIN_DIRECTION)-1;

	if(pPin != NULL)
	{
		pPin->QueryDirection(&dir);
	}

	return dir;
}

IFilterGraph* DirectShow::GetGraph(IBaseFilter* pBF)
{
	CFilterInfo fi;

	if(pBF != NULL && SUCCEEDED(pBF->QueryFilterInfo(&fi)))
	{
		return fi.pGraph;
	}

	return NULL;
}

IBaseFilter* DirectShow::GetFilter(IPin* pPin)
{
	CPinInfo pi;

	if(pPin != NULL && SUCCEEDED(pPin->QueryPinInfo(&pi)))
	{
		return pi.pFilter;
	}

	return NULL;
}

CLSID DirectShow::GetCLSID(IBaseFilter* pBF)
{
	CLSID clsid = GUID_NULL;
	if(pBF) pBF->GetClassID(&clsid);
	return clsid;
}

CLSID DirectShow::GetCLSID(IPin* pPin)
{
	return GetCLSID(GetFilter(pPin));
}

std::wstring DirectShow::GetName(IBaseFilter* pBF)
{
	std::wstring s;

	CFilterInfo fi;

	if(pBF != NULL && SUCCEEDED(pBF->QueryFilterInfo(&fi)))
	{
		s = fi.achName;
	}

	return s;
}

std::wstring DirectShow::GetName(IPin* pPin)
{
	std::wstring s;

	CPinInfo pi;

	if(pPin != NULL && SUCCEEDED(pPin->QueryPinInfo(&pi)))
	{
		s = pi.achName;
	}

	return s;
}

bool DirectShow::IsSplitter(IBaseFilter* pBF, bool connected)
{
	PIN_COUNT count;

	GetPinCount(pBF, count);

	return connected ? count.connected.out > 1 : count.out > 1;
}

bool DirectShow::IsMultiplexer(IBaseFilter* pBF, bool connected)
{
	PIN_COUNT count;

	GetPinCount(pBF, count);

	return connected ? count.connected.in > 1 : count.in > 1;
}

bool DirectShow::IsStreamStart(IBaseFilter* pBF)
{
	CComQIPtr<IAMFilterMiscFlags> pAMMF = pBF;

	if(pAMMF != NULL && pAMMF->GetMiscFlags() & AM_FILTER_MISC_FLAGS_IS_SOURCE)
	{
		return true;
	}

	PIN_COUNT count;

	GetPinCount(pBF, count);

	if(count.out > 1)
	{
		return true;
	}

	CMediaType mt;

	IPin* pin = GetFirstPin(pBF);

	return count.out > 0 && count.in == 1 && pin && SUCCEEDED(pin->ConnectionMediaType(&mt)) && mt.majortype == MEDIATYPE_Stream;
}

bool DirectShow::IsStreamEnd(IBaseFilter* pBF)
{
	PIN_COUNT count;

	GetPinCount(pBF, count);

	return count.out == 0;
}

bool DirectShow::IsVideoRenderer(IBaseFilter* pBF)
{
	CLSID clsid = GetCLSID(pBF);

	if(clsid == CLSID_VideoRenderer || clsid == CLSID_VideoRendererDefault)
	{
		return true;
	}

	PIN_COUNT count;

	GetPinCount(pBF, count);

	if(count.connected.in > 0 && count.out == 0)
	{
		HRESULT hr;

		hr = Foreach(pBF, [] (IPin* pin) -> HRESULT
		{
			CMediaType mt;

			if(S_OK == pin->ConnectionMediaType(&mt))
			{
				return mt.majortype == MEDIATYPE_Video ? S_OK : S_FALSE; // && (mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_VideoInfo2)
			}

			return S_CONTINUE;
		});

		if(hr == S_OK)
		{
			return true;
		}
	}

	return false;
}

bool DirectShow::IsAudioWaveRenderer(IBaseFilter* pBF)
{
	CLSID clsid = GetCLSID(pBF);

	if(clsid == CLSID_DSoundRender || clsid == CLSID_AudioRender || clsid == CLSID_ReClock)
	{
		return true;
	}

	CComQIPtr<IBasicAudio> pBA = pBF;

	PIN_COUNT count;

	GetPinCount(pBF, count);

	if(count.connected.in > 0 && count.out == 0 && pBA != NULL)
	{
		HRESULT hr;

		hr = Foreach(pBF, [] (IPin* pin) -> HRESULT
		{
			CMediaType mt;

			if(S_OK == pin->ConnectionMediaType(&mt))
			{
				return mt.majortype == MEDIATYPE_Audio ? S_OK : S_FALSE; /*&& (mt.formattype == FORMAT_WaveFormatEx);*/
			}

			return S_CONTINUE;
		});

		if(hr == S_OK)
		{
			return true;
		}
	}

	return false;
}

int DirectShow::AACInitData(BYTE* data, int profile, int freq, int channels)
{
	int srate_idx;

	if(92017 <= freq) srate_idx = 0;
	else if(75132 <= freq) srate_idx = 1;
	else if(55426 <= freq) srate_idx = 2;
	else if(46009 <= freq) srate_idx = 3;
	else if(37566 <= freq) srate_idx = 4;
	else if(27713 <= freq) srate_idx = 5;
	else if(23004 <= freq) srate_idx = 6;
	else if(18783 <= freq) srate_idx = 7;
	else if(13856 <= freq) srate_idx = 8;
	else if(11502 <= freq) srate_idx = 9;
	else if(9391 <= freq) srate_idx = 10;
	else srate_idx = 11;

	data[0] = ((abs(profile) + 1) << 3) | ((srate_idx & 0xe) >> 1);
	data[1] = ((srate_idx & 0x1) << 7) | (channels << 3);

	int size = 2;

	if(profile < 0)
	{
		freq *= 2;

		if(92017 <= freq) srate_idx = 0;
		else if(75132 <= freq) srate_idx = 1;
		else if(55426 <= freq) srate_idx = 2;
		else if(46009 <= freq) srate_idx = 3;
		else if(37566 <= freq) srate_idx = 4;
		else if(27713 <= freq) srate_idx = 5;
		else if(23004 <= freq) srate_idx = 6;
		else if(18783 <= freq) srate_idx = 7;
		else if(13856 <= freq) srate_idx = 8;
		else if(11502 <= freq) srate_idx = 9;
		else if(9391 <= freq) srate_idx = 10;
		else srate_idx = 11;

		data[2] = 0x2b7 >> 3;
		data[3] = (BYTE)((0x2b7 << 5) | 5);
		data[4] = (1 << 7) | (srate_idx << 3);

		size = 5;
	}

	return size;
}

bool DirectShow::Mpeg2InitData(CMediaType& mt, BYTE* seqhdr, int len, int w, int h)
{
	if(len < 4 || *(DWORD*)seqhdr != 0xb3010000) 
	{
		return false;
	}

	BYTE* seqhdr_ext = NULL;
	BYTE* seqhdr_end = seqhdr + 11;

	if(seqhdr_end - seqhdr > len) return false;
	if(*seqhdr_end & 0x02) seqhdr_end += 64;
	if(seqhdr_end - seqhdr > len) return false;
	if(*seqhdr_end & 0x01) seqhdr_end += 64;
	if(seqhdr_end - seqhdr > len) return false;
	seqhdr_end++;
	if(seqhdr_end - seqhdr > len) return false;
	if(len - (seqhdr_end - seqhdr) > 4 && *(DWORD*)seqhdr_end == 0xb5010000) {seqhdr_ext = seqhdr_end; seqhdr_end += 10;}
	if(seqhdr_end - seqhdr > len) return false;

	len = seqhdr_end - seqhdr;
	
	mt.InitMediaType();

	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
	mt.formattype = FORMAT_MPEG2Video;

	MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + len);

	memset(mt.Format(), 0, mt.FormatLength());
	
	vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
	vih->hdr.bmiHeader.biWidth = w;
	vih->hdr.bmiHeader.biHeight = h;

	vih->cbSequenceHeader = len;
	
	memcpy(vih->dwSequenceHeader, seqhdr, len);

	static char profile[8] = 
	{
		0, AM_MPEG2Profile_High, AM_MPEG2Profile_SpatiallyScalable, AM_MPEG2Profile_SNRScalable, 
		AM_MPEG2Profile_Main, AM_MPEG2Profile_Simple, 0, 0
	};

	static char level[16] =
	{
		0, 0, 0, 0, 
		AM_MPEG2Level_High, 0, AM_MPEG2Level_High1440, 0, 
		AM_MPEG2Level_Main, 0, AM_MPEG2Level_Low, 0, 
		0, 0, 0, 0
	};

	if(seqhdr_ext && (seqhdr_ext[4] & 0xf0) == 0x10)
	{
		vih->dwProfile = profile[seqhdr_ext[4] & 0x07];
		vih->dwLevel = level[seqhdr_ext[5] >> 4];
	}

	return true;
}

bool DirectShow::GetTempFileName(std::wstring& path, LPCWSTR subdir)
{
	WCHAR dir[MAX_PATH];
	WCHAR fn[MAX_PATH];

	if(::GetTempPath(MAX_PATH, dir))
	{
		if(subdir != NULL)
		{
			PathCombine(fn, dir, subdir);

			wcscpy(dir, fn);

			CreateDirectory(dir, NULL);
		}

		if(::GetTempFileName(dir, L"homesys", 0, fn))
		{
			path = fn;

			return true;
		}
	}

	return false;
}

std::wstring DirectShow::StripDVBName(const char* str, int len)
{
	DWORD cp = 28591; // iso-6937?

	std::string s;

	if(str != NULL && len > 0)
	{
		int i = 0;

		if(str[0] < 0x20)
		{
			switch(str[i++])
			{
			case 1: cp = 28595; break; // iso-8859-1
			case 2: cp = 28596; break; // iso-8859-2
			case 3: cp = 28597; break; // iso-8859-3
			case 4: cp = 28598; break; // iso-8859-4
			case 5: cp = 28599; break; // iso-8859-5
			case 0x10: cp = 28590 + str[i + 1]; i += 2; break; // iso-8859-n
			case 0x11: cp = 1200; break; // utf-16 (TODO: test this)
			default: ASSERT(0); break;
			}
		}

		for(; i < len; i++)
		{
			BYTE c = (BYTE)str[i];

			if(c == 0xe0 && i < len - 1 && str[i + 1] >= 0x80 && str[i + 1] < 0xa0)
			{
				i++;

				continue;
			}

			if(c == 0x0a)
			{
				c = ' ';
			}

			if(c >= 0x80 && c < 0xa0)
			{
				continue;
			}

			s += c;
		}
	}

	return Util::ConvertMBCS(s, cp);
}

bool DirectShow::Match(HANDLE file, const std::wstring& pattern)
{
	std::vector<std::wstring> s;

	Util::Explode(pattern, s, L",");

	if(s.size() < 4 || (s.size() & 3) != 0)
	{
		return false;
	}

	LARGE_INTEGER size;

	size.QuadPart = 0;

	GetFileSizeEx(file, &size);

	for(int i = 0; i < s.size(); i += 4)
	{
		std::wstring offsetstr = s[i];
		std::wstring cbstr = s[i + 1];
		std::wstring maskstr = s[i + 2];
		std::wstring valstr = s[i + 3];

		long cb = _wtol(cbstr.c_str());

		if(offsetstr.empty() || cbstr.empty() 
		|| valstr.empty() || (valstr.size() & 1)
		|| cb * 2 != valstr.size())
		{
			return false;
		}

		LARGE_INTEGER offset;

		offset.QuadPart = _wtoi64(offsetstr.c_str());

		if(offset.QuadPart < 0) 
		{
			offset.QuadPart = size.QuadPart - offset.QuadPart;
		}

		SetFilePointerEx(file, offset, NULL, FILE_BEGIN);

		while(maskstr.size() < valstr.size())
		{
			maskstr += L"F"; // LAME
		}

		std::vector<BYTE> mask;
		std::vector<BYTE> val;

		Util::ToBin(maskstr.c_str(), mask);
		Util::ToBin(valstr.c_str(), val);

		for(int i = 0; i < val.size(); i++)
		{
			BYTE b;
			DWORD read;

			if(!ReadFile(file, &b, 1, &read, NULL) || (b & mask[i]) != val[i])
			{
				return false;
			}
		}
	}

	return true;
}

UINT64 DirectShow::GetFileVersion(LPCWSTR fn)
{
	UINT64 ver = 0;

	DWORD zero; // a variable that GetFileVersionInfoSize sets to zero (but why is it needed ?????????????????????????????? :)

	DWORD len = GetFileVersionInfoSize(fn, &zero);
	
	if(len > 0)
	{
		WCHAR* block = new WCHAR[len];

		VS_FIXEDFILEINFO* pvsf = NULL;

        UINT uLen;

        if(GetFileVersionInfo(fn, 0, len, block) && VerQueryValue(block, L"\\", (void**)&pvsf, &uLen))
        {
			ver = ((UINT64)pvsf->dwFileVersionMS << 32) | pvsf->dwFileVersionLS;
        }

        delete [] block;
	}

	return ver;
}

struct ExternalObject
{
	std::wstring path;
	HINSTANCE hInst;
	CLSID clsid;
};

static std::list<ExternalObject> s_extobjs; // TODO: access not thread safe

HRESULT DirectShow::LoadExternalObject(LPCWSTR path, REFCLSID clsid, REFIID iid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	std::wstring fullpath = path; // TODO: MakeFullPath(path);

	HINSTANCE hInst = NULL;

	bool found = false;

	for(auto i = s_extobjs.begin(); i != s_extobjs.end(); i++)
	{
		const ExternalObject& eo = *i;
		
		if(wcsicmp(eo.path.c_str(), path) == 0)
		{
			hInst = eo.hInst;
			found = true;
			break;
		}
	}

	if(hInst == NULL)
	{
		hInst = CoLoadLibrary(CComBSTR(fullpath.c_str()), TRUE);
	}

	HRESULT hr = E_FAIL;

	if(hInst != NULL)
	{
		typedef HRESULT (__stdcall * PDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

		PDllGetClassObject p = (PDllGetClassObject)GetProcAddress(hInst, "DllGetClassObject");

		if(p != NULL && FAILED(hr = p(clsid, iid, ppv)))
		{
			CComPtr<IClassFactory> pCF;

			hr = p(clsid, __uuidof(IClassFactory), (void**)&pCF);

			if(SUCCEEDED(hr) && pCF != NULL)
			{
				hr = pCF->CreateInstance(NULL, iid, ppv);
			}
		}
	}

	if(FAILED(hr) && hInst && !found)
	{
		CoFreeLibrary(hInst);

		return hr;
	}

	if(hInst && !found)
	{
		ExternalObject eo;
		
		eo.path = fullpath;
		eo.hInst = hInst;
		eo.clsid = clsid;

		s_extobjs.push_back(eo);
	}

	return hr;
}

static int Eval_Exception(int n_except)
{
    if(n_except == STATUS_ACCESS_VIOLATION)
	{
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

static void MyOleCreatePropertyFrame(HWND hwndOwner, UINT x, UINT y, LPCOLESTR lpszCaption, ULONG cObjects, LPUNKNOWN FAR* lplpUnk, ULONG cPages, LPCLSID lpPageClsID, LCID lcid, DWORD dwReserved, LPVOID lpvReserved)
{
	__try
	{
		OleCreatePropertyFrame(hwndOwner, x, y, lpszCaption, cObjects, lplpUnk, cPages, lpPageClsID, lcid, dwReserved, lpvReserved);
	}
	__except(Eval_Exception(GetExceptionCode()))
	{
		// No code; this block never executed.
	}
}

void DirectShow::ShowPropertyPage(IUnknown* pUnk, HWND hParentWnd)
{
	CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk;

	if(!pSPP) return;

	std::wstring str;

	CComQIPtr<IBaseFilter> pBF = pSPP;

	CFilterInfo fi;

	CComQIPtr<IPin> pPin = pSPP;

	CPinInfo pi;

	if(pBF && SUCCEEDED(pBF->QueryFilterInfo(&fi)))
	{
		str = fi.achName;
	}
	else if(pPin && SUCCEEDED(pPin->QueryPinInfo(&pi)))
	{
		str = pi.achName;
	}

	CAUUID caGUID;
	
	caGUID.pElems = NULL;

	if(SUCCEEDED(pSPP->GetPages(&caGUID)))
	{
		IUnknown* lpUnk = NULL;

		pSPP.QueryInterface(&lpUnk);

		MyOleCreatePropertyFrame(
			hParentWnd, 0, 0, str.c_str(), 
			1, (IUnknown**)&lpUnk, 
			caGUID.cElems, caGUID.pElems, 
			0, 0, NULL);

		lpUnk->Release();

		if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);
	}
}

std::wstring DirectShow::GetDriveLabel(wchar_t drive)
{
	std::wstring label;

	wchar_t VolumeNameBuffer[MAX_PATH];
	wchar_t FileSystemNameBuffer[MAX_PATH];
	DWORD VolumeSerialNumber;
	DWORD MaximumComponentLength;
	DWORD FileSystemFlags;
	
	if(GetVolumeInformation(
		Util::Format(L"%c:\\", drive).c_str(), 
		VolumeNameBuffer, MAX_PATH, &VolumeSerialNumber, &MaximumComponentLength, 
		&FileSystemFlags, FileSystemNameBuffer, MAX_PATH))
	{
		label = VolumeNameBuffer;
	}

	return(label);
}

static void FindFiles(LPCWSTR path, std::list<std::wstring>& files)
{
	std::wstring dir = Util::RemoveFileSpec(path);
	
	WIN32_FIND_DATA fd;

	HANDLE h = FindFirstFile(dir.c_str(), &fd);
	
	if(h != INVALID_HANDLE_VALUE)
	{
		do 
		{
			files.push_back(dir + L"\\" + fd.cFileName);
		}
		while(FindNextFile(h, &fd));

		FindClose(h);
	}
}

int DirectShow::GetDriveType(wchar_t drive)
{
	std::list<std::wstring> files;

	return GetDriveType(drive, files);
}

int DirectShow::GetDriveType(wchar_t drive, std::list<std::wstring>& files)
{
	files.clear();

	std::wstring path = Util::Format(L"%c:", drive);

	std::wstring dir = path + L"\\";

	if(::GetDriveType(dir.c_str()) == DRIVE_CDROM && GetVolumeInformation(dir.c_str(), NULL, NULL, NULL, NULL, NULL, NULL, 0))
	{
		FindFiles((path + L"\\mpegav\\avseq??.dat").c_str(), files);
		FindFiles((path + L"\\mpegav\\avseq??.mpg").c_str(), files);
		FindFiles((path + L"\\mpeg2\\avseq??.dat").c_str(), files);
		FindFiles((path + L"\\mpeg2\\avseq??.mpg").c_str(), files);
		FindFiles((path + L"\\mpegav\\music??.dat").c_str(), files);
		FindFiles((path + L"\\mpegav\\music??.mpg").c_str(), files);
		FindFiles((path + L"\\mpeg2\\music??.dat").c_str(), files);
		FindFiles((path + L"\\mpeg2\\music??.mpg").c_str(), files);

		if(!files.empty()) 
		{
			return Drive_VideoCD;
		}

		FindFiles((path + L"\\VIDEO_TS\\video_ts.ifo").c_str(), files);

		if(!files.empty())
		{
			return Drive_DVDVideo;
		}

		if(!(GetVersion() & 0x80000000))
		{
			HANDLE hDrive = CreateFile((L"\\\\.\\" + path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, (HANDLE)NULL);

			if(hDrive != INVALID_HANDLE_VALUE)
			{
				CDROM_TOC TOC;
				DWORD BytesReturned;

				if(DeviceIoControl(hDrive, IOCTL_CDROM_READ_TOC, NULL, 0, &TOC, sizeof(TOC), &BytesReturned, 0))
				{
					for(int i = TOC.FirstTrack; i <= TOC.LastTrack; i++)
					{
						// MMC-3 Draft Revision 10g: Table 222 EQ Sub-channel control field
						
						TOC.TrackData[i - 1].Control &= 5;
						
						if(TOC.TrackData[i - 1].Control == 0 || TOC.TrackData[i - 1].Control == 1) 
						{
							files.push_back(path + Util::Format(L"\\track%02d.cda", i));
						}
					}
				}

				CloseHandle(hDrive);
			}
		}

		if(!files.empty()) 
		{
			return Drive_Audio;
		}

		// nothing special

		return Drive_Unknown;
	}
	
	return Drive_NotFound;
}

DVD_HMSF_TIMECODE DirectShow::RT2HMSF(REFERENCE_TIME rt, double fps)
{
	DVD_HMSF_TIMECODE hmsf;

	rt /= 10000;

	hmsf.bFrames = (BYTE)(1.0 * (rt % 1000) * fps / 1000);

	rt /= 1000;

	hmsf.bSeconds = rt % 60;

	rt /= 60;

	hmsf.bMinutes = rt % 60;
	
	rt /= 60;

	hmsf.bHours = rt;

	return hmsf;
}

REFERENCE_TIME DirectShow::HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps)
{
	if(fps == 0) 
	{
		hmsf.bFrames = 0; 

		fps = 1;
	}

	REFERENCE_TIME rt;

	rt = hmsf.bHours;
	rt = rt * 60 + hmsf.bMinutes;
	rt = rt * 60 + hmsf.bSeconds;
	rt = rt * 1000 + (REFERENCE_TIME)(1.0 * hmsf.bFrames * 1000 / fps + 0.5);

	return rt * 10000;
}

bool DirectShow::DeleteRegKey(LPCWSTR pszKey, LPCWSTR pszSubkey)
{
	bool ok = false;

	HKEY hKey;

	LONG ec = RegOpenKeyEx(HKEY_CLASSES_ROOT, pszKey, 0, KEY_ALL_ACCESS, &hKey);

	if(ec == ERROR_SUCCESS)
	{
		if(pszSubkey != 0)
		{
			ec = ::RegDeleteKey(hKey, pszSubkey);
		}

		ok = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return ok;
}

bool DirectShow::SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValueName, LPCWSTR pszValue)
{
	bool ok = false;

	std::wstring szKey = pszKey;

	if(pszSubkey != 0)
	{
		szKey += L"\\";
		szKey += pszSubkey;
	}

	HKEY hKey;

	LONG ec = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey.c_str(), 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);

	if(ec == ERROR_SUCCESS)
	{
		if(pszValue != 0)
		{
			ec = ::RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE*)pszValue, (wcslen(pszValue) + 1) * sizeof(WCHAR));
		}

		ok = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return ok;
}

bool DirectShow::SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValue)
{
	return SetRegKeyValue(pszKey, pszSubkey, 0, pszValue);
}

void DirectShow::RegisterSourceFilter(const CLSID& clsid2, const GUID& subtype2, LPCWSTR chkbytes, LPCWSTR ext, ...)
{
	std::wstring clsid, null, majortype, subtype;

	Util::ToString(clsid2, clsid);
	Util::ToString(GUID_NULL, null);
	Util::ToString(MEDIATYPE_Stream, majortype);
	Util::ToString(subtype2, subtype);

	SetRegKeyValue((L"Media Type\\" + majortype).c_str(), subtype.c_str(), L"0", chkbytes);
	SetRegKeyValue((L"Media Type\\" + majortype).c_str(), subtype.c_str(), L"Source Filter", clsid.c_str());
	
	DeleteRegKey((L"Media Type\\" + null).c_str(), subtype.c_str());

	va_list marker;
	va_start(marker, ext);

	for(; ext; ext = va_arg(marker, LPCWSTR))
	{
		DeleteRegKey(L"Media Type\\Extensions", ext);
	}

	va_end(marker);
}

void DirectShow::RegisterSourceFilter(const CLSID& clsid2, const GUID& subtype2, const std::list<std::wstring>& chkbytes, LPCTSTR ext, ...)
{
	std::wstring clsid, null, majortype, subtype;

	Util::ToString(clsid2, clsid);
	Util::ToString(GUID_NULL, null);
	Util::ToString(MEDIATYPE_Stream, majortype);
	Util::ToString(subtype2, subtype);

	for(auto i = chkbytes.begin(); i != chkbytes.end(); i++)
	{
		SetRegKeyValue((L"Media Type\\" + majortype).c_str(), subtype.c_str(), Util::Format(L"%d", i).c_str(), i->c_str());
	}

	SetRegKeyValue((L"Media Type\\" + majortype).c_str(), subtype.c_str(), L"Source Filter", clsid.c_str());
	
	DeleteRegKey((L"Media Type\\" + null).c_str(), subtype.c_str());

	va_list marker;
	va_start(marker, ext);

	for(; ext; ext = va_arg(marker, LPCTSTR))
	{
		DeleteRegKey(L"Media Type\\Extensions", ext);
	}

	va_end(marker);
}

void DirectShow::UnRegisterSourceFilter(const GUID& subtype2)
{
	std::wstring majortype, subtype;

	Util::ToString(MEDIATYPE_Stream, majortype);
	Util::ToString(subtype2, subtype);

	DeleteRegKey((L"Media Type\\" + majortype).c_str(), subtype.c_str());
}
