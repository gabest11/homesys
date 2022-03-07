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

#include "stdafx.h"
#include "CDDAReader.h"
#include "DirectShow.h"
#include "IGraphBuilder2.h"

#define RAW_SECTOR_SIZE 2352
#define MSF2UINT(hgs) ((hgs[1] * 4500) + (hgs[2] * 75) + (hgs[3]))

// CCDDAReader

CCDDAReader::CCDDAReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(NAME("CCDDAReader"), pUnk, &m_stream, phr, __uuidof(this))
{
	if(phr) *phr = S_OK;

	if(GetVersion() & 0x80000000)
	{
		if(phr) *phr = E_NOTIMPL;

		return;
	}
}

CCDDAReader::~CCDDAReader()
{
}

STDMETHODIMP CCDDAReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IFileSourceFilter)
		QI2(IAMMediaContent)
		QI(IStreamBuilder)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IFileSourceFilter

STDMETHODIMP CCDDAReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt) 
{
	if(!m_stream.Load(pszFileName))
	{
		return E_FAIL;
	}

	m_fn = pszFileName;

	CMediaType mt;

	mt.majortype = MEDIATYPE_Stream;
	mt.subtype = MEDIASUBTYPE_WAVE;

	m_mt = mt;

	return S_OK;
}

STDMETHODIMP CCDDAReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.size() + 1) * sizeof(WCHAR))))
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_fn.c_str());

	return S_OK;
}

// IAMMediaContent

STDMETHODIMP CCDDAReader::GetTypeInfoCount(UINT* pctinfo) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_AuthorName(BSTR* pbstrAuthorName)
{
	CheckPointer(pbstrAuthorName, E_POINTER);

	std::wstring s = m_stream.m_track.artist;

	if(s.empty()) s = m_stream.m_disc.artist;

	*pbstrAuthorName = SysAllocString(s.c_str());

	return S_OK;
}

STDMETHODIMP CCDDAReader::get_Title(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	std::wstring s = m_stream.m_track.title;

	if(s.empty()) s = m_stream.m_disc.title;

	*pbstrTitle = SysAllocString(s.c_str());

	return S_OK;
}

STDMETHODIMP CCDDAReader::get_Rating(BSTR* pbstrRating) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_Description(BSTR* pbstrDescription) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_Copyright(BSTR* pbstrCopyright) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_BaseURL(BSTR* pbstrBaseURL) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_LogoURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_LogoIconURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_WatermarkURL(BSTR* pbstrWatermarkURL) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_MoreInfoURL(BSTR* pbstrMoreInfoURL) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) {return E_NOTIMPL;}
STDMETHODIMP CCDDAReader::get_MoreInfoText(BSTR* pbstrMoreInfoText) {return E_NOTIMPL;}

// IStreamBuilder

[uuid("D51BD5A1-7548-11CF-A520-0080C77EF58A")]
class WaveParser {};

STDMETHODIMP CCDDAReader::Render(IPin* ppinOut, IGraphBuilder* pGraph)
{
	CComPtr<IBaseFilter> pBF;

	if(FAILED(pBF.CoCreateInstance(__uuidof(WaveParser))))
	{
		return E_FAIL;
	}

	if(FAILED(pGraph->AddFilter(pBF, L"Wave Parser")))
	{
		return E_FAIL;
	}

	if(FAILED(pGraph->Connect(ppinOut, DirectShow::GetFirstPin(pBF))))
	{
		return E_FAIL;
	}

	if(FAILED(pGraph->Render(DirectShow::GetFirstPin(pBF, PINDIR_OUTPUT))))
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CCDDAReader::Backout(IPin* ppinOut, IGraphBuilder* pGraph)
{
	if(CComQIPtr<IGraphBuilder2> pGB = pGraph)
	{
		pGB->NukeDownstream(ppinOut);
	}

	return S_OK;
}

// CCDDAStream

CCDDAStream::CCDDAStream()
{
	m_drive = INVALID_HANDLE_VALUE;

	m_pos = m_len = 0;

	memset(&m_toc, 0, sizeof(m_toc));
	
	memset(&m_sector, 0, sizeof(m_sector));

	memset(&m_header, 0, sizeof(m_header));

	m_header.riff.hdr.chunkID = RIFFID;
	m_header.riff.WAVE = WAVEID;
	m_header.frm.hdr.chunkID = FormatID;
	m_header.frm.hdr.chunkSize = sizeof(m_header.frm.pcm);
	m_header.frm.pcm.wf.wFormatTag = WAVE_FORMAT_PCM;
	m_header.frm.pcm.wf.nSamplesPerSec = 44100;
	m_header.frm.pcm.wf.nChannels = 2;
	m_header.frm.pcm.wBitsPerSample = 16;
	m_header.frm.pcm.wf.nBlockAlign = m_header.frm.pcm.wf.nChannels * m_header.frm.pcm.wBitsPerSample / 8;
	m_header.frm.pcm.wf.nAvgBytesPerSec = m_header.frm.pcm.wf.nSamplesPerSec * m_header.frm.pcm.wf.nBlockAlign;
	m_header.data.hdr.chunkID = DataID;
}

CCDDAStream::~CCDDAStream()
{
	if(m_drive != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_drive);
	}
}

bool CCDDAStream::Load(LPCWSTR fn)
{
	std::wstring path(fn);

	std::wstring::size_type drivepos = path.find(L":\\");

	if(drivepos == std::wstring::npos)
	{
		return false;
	}

	std::wstring::size_type trackpos = Util::MakeLower(path).find(L".cda");

	if(trackpos == std::wstring::npos)
	{
		return false;
	}

	while(trackpos > 0 && _istdigit(path[trackpos - 1]))
	{
		trackpos--;
	}

	int track = _wtoi(&path[trackpos]);

	if(track == 0) 
	{
		return false;
	}

	if(m_drive != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_drive);

		m_drive = INVALID_HANDLE_VALUE;
	}

	std::wstring drive = std::wstring(L"\\\\.\\") + path[drivepos - 1] + L":";

	m_drive = CreateFile(drive.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(m_drive == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD BytesReturned;

	if(!DeviceIoControl(m_drive, IOCTL_CDROM_READ_TOC, NULL, 0, &m_toc, sizeof(m_toc), &BytesReturned, 0) || !(m_toc.FirstTrack <= track && track <= m_toc.LastTrack))
	{
		CloseHandle(m_drive);
		
		m_drive = INVALID_HANDLE_VALUE;

		return false;
	}

	// MMC-3 Draft Revision 10g: Table 222 – Q Sub-channel control field

	m_toc.TrackData[track - 1].Control &= 5;

	if(!(m_toc.TrackData[track - 1].Control == 0 || m_toc.TrackData[track - 1].Control == 1))
	{
		CloseHandle(m_drive);

		m_drive = INVALID_HANDLE_VALUE;

		return false;
	}

	if(m_toc.TrackData[track - 1].Control & 8) 
	{
		m_header.frm.pcm.wf.nChannels = 4;
	}

	m_sector.start = MSF2UINT(m_toc.TrackData[track - 1].Address) - 150; // MSF2UINT(m_TOC.TrackData[0].Address);
	m_sector.stop = MSF2UINT(m_toc.TrackData[track].Address) - 150; // MSF2UINT(m_TOC.TrackData[0].Address);

	m_len = (m_sector.stop - m_sector.start) * RAW_SECTOR_SIZE;

	m_header.riff.hdr.chunkSize = (long)(m_len + sizeof(m_header) - 8);
	m_header.data.hdr.chunkSize = (long)(m_len);

	do
	{
		CDROM_READ_TOC_EX TOCEx;

		memset(&TOCEx, 0, sizeof(TOCEx));

		TOCEx.Format = CDROM_READ_TOC_EX_FORMAT_CDTEXT;
		TOCEx.SessionTrack = track;

		WORD size = 0;

		ASSERT(MINIMUM_CDROM_READ_TOC_EX_SIZE == sizeof(size));

		if(!DeviceIoControl(m_drive, IOCTL_CDROM_READ_TOC_EX, &TOCEx, sizeof(TOCEx), &size, sizeof(size), &BytesReturned, 0))
		{
			break;
		}

		size = ((size >> 8) | (size << 8)) + sizeof(size);

		std::vector<BYTE> cdtext;

		cdtext.resize(size);

		if(!DeviceIoControl(m_drive, IOCTL_CDROM_READ_TOC_EX, &TOCEx, sizeof(TOCEx), &cdtext[0], size, &BytesReturned, 0))
		{
			break;
		}

		size = (WORD)(BytesReturned - sizeof(CDROM_TOC_CD_TEXT_DATA));

		CDROM_TOC_CD_TEXT_DATA_BLOCK* desc = ((CDROM_TOC_CD_TEXT_DATA*)(BYTE*)&cdtext[0])->Descriptors;

		std::vector<std::wstring> str[16];

		for(int i = 0; i < 16; i++) 
		{
			str[i].resize(1 + m_toc.LastTrack);
		}

		std::wstring last;

		for(int i = 0; size >= sizeof(CDROM_TOC_CD_TEXT_DATA_BLOCK); i++, size -= sizeof(CDROM_TOC_CD_TEXT_DATA_BLOCK), desc++)
		{
			if(desc->TrackNumber > m_toc.LastTrack)
			{
				continue;
			}

			std::wstring text;

			int len = countof(desc->Text);

			if(desc->Unicode)
			{
				text = std::wstring(desc->WText, len);
			}
			else
			{
				text = Util::ConvertMBCS(std::string((char*)desc->Text, len), ANSI_CHARSET);
			}

			int tlen = text.size();

			std::wstring tmp;

			if(tlen < 12 - 1)
			{
				int o = tlen + 1;

				if(desc->Unicode)
				{
					tmp = std::wstring(desc->WText + o, len - o);
				}
				else
				{
					tmp = Util::ConvertMBCS(std::string((char*)desc->Text + o, len - o), ANSI_CHARSET);
				}
			}

			if(desc->PackType >= 0x90) 
			{
				continue;
			}

			std::vector<std::wstring>& s = str[desc->PackType - 0x80];

			if(desc->CharacterPosition == 0) 
			{
				s[desc->TrackNumber] = text;
			}
			else if(desc->CharacterPosition < 16)
			{
				if(desc->CharacterPosition < 0xf && last.size() > 0)
				{
					s[desc->TrackNumber] = last + text;
				}
				else
				{
					s[desc->TrackNumber] += text;
				}
			}

			last = tmp;
		}

		m_disc.title = str[0][0];
		m_disc.artist = str[1][0];
		m_track.title = str[0][track];
		m_track.artist = str[1][track];
	}
	while(0);

	return true;
}

HRESULT CCDDAStream::SetPointer(LONGLONG llPos)
{
	if(llPos < 0 || llPos > m_len)
	{
		return S_FALSE;
	}

	m_pos = llPos;
	
	return S_OK;
}

HRESULT CCDDAStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	CAutoLock cAutoLock(&m_csReadLock);

	BYTE buff[RAW_SECTOR_SIZE];

	BYTE* pbBufferOrg = pbBuffer;

	LONGLONG pos = m_pos;

	size_t len = (size_t)dwBytesToRead;

	if(pos < sizeof(m_header) && len > 0)
	{
		size_t l = (size_t)min(len, sizeof(m_header) - pos);

		memcpy(pbBuffer, &((BYTE*)&m_header)[pos], l);

		pbBuffer += l;
		pos += l;
		len -= l;
	}

	pos -= sizeof(m_header);

	while(pos >= 0 && pos < m_len && len > 0)
	{
		RAW_READ_INFO rawreadinfo;

		rawreadinfo.SectorCount = 1;
		rawreadinfo.TrackMode = CDDA;

		UINT sector = m_sector.start + int(pos / RAW_SECTOR_SIZE);

		__int64 offset = pos % RAW_SECTOR_SIZE;

		rawreadinfo.DiskOffset.QuadPart = sector * 2048;

		DWORD BytesReturned = 0;

		BOOL b = DeviceIoControl(m_drive, IOCTL_CDROM_RAW_READ, &rawreadinfo, sizeof(rawreadinfo), buff, RAW_SECTOR_SIZE, &BytesReturned, 0);

		size_t l = (size_t)min(min(len, RAW_SECTOR_SIZE - offset), m_len - pos);

		memcpy(pbBuffer, &buff[offset], l);

		pbBuffer += l;
		pos += l;
		len -= l;
	}

	if(pdwBytesRead) 
	{
		*pdwBytesRead = pbBuffer - pbBufferOrg;
	}

	m_pos += pbBuffer - pbBufferOrg;

	return S_OK;
}

LONGLONG CCDDAStream::Size(LONGLONG* pSizeAvailable)
{
	LONGLONG size = sizeof(m_header) + m_len;

	if(pSizeAvailable) *pSizeAvailable = size;

    return size;
}

DWORD CCDDAStream::Alignment()
{
    return 1;
}

void CCDDAStream::Lock()
{
    m_csReadLock.Lock();
}

void CCDDAStream::Unlock()
{
    m_csReadLock.Unlock();
}

