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

#include "../3rdparty/baseclasses/baseclasses.h"
#include "../util/Socket.h"

struct HttpFilePart
{
	__int64 pos, len;
};

[uuid("99FD6F34-6249-42A0-A7A4-36E9F61FA193")]
interface IHttpReader : public IUnknown
{
	STDMETHOD(GetAvailable)(std::list<HttpFilePart>& parts, __int64& len) = 0;
	STDMETHOD(SetUserAgent)(LPCSTR useragent) = 0;
};

class CHttpStream 
	: public CAsyncStream
	, public CAMThread
	, public CCritSec
{
	CCritSec m_csReadLock;
	std::string m_url;
	std::string m_useragent;
	Util::Socket m_socket;
	struct {int size, count; LONG* buff;} m_chunk;
	std::wstring m_temp;
	HANDLE m_input;
	HANDLE m_output;
	__int64 m_pos;
	__int64 m_length;
	__int64 m_seek;
	bool m_error;

	bool Open(__int64 start = 0, int retry_count = 0);
	void Close();

	bool GetChunk(__int64 pos);
	void SetChunk(__int64 pos);

	enum {CMD_EXIT, CMD_START};
	
	DWORD ThreadProc();

	void FillBuffer();

	friend class CHttpReader;

public:
	CHttpStream();
	virtual ~CHttpStream();

	bool Load(LPCSTR url);

    HRESULT SetPointer(LONGLONG llPos);
    HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
    LONGLONG Size(LONGLONG* pSizeAvailable);
    DWORD Alignment();
    void Lock();
	void Unlock();
};

[uuid("BF245C0A-F9EC-4B7F-85C3-4407C6CE815E")]
class CHttpReader 
	: public CAsyncReader
	, public IFileSourceFilter
	, public IHttpReader
{
	CHttpStream m_stream;
	std::wstring m_url;

public:
    CHttpReader(IUnknown* pUnk, HRESULT* phr);
	~CHttpReader();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IHttpReader

	STDMETHODIMP GetAvailable(std::list<HttpFilePart>& parts, __int64& len);
	STDMETHODIMP SetUserAgent(LPCSTR useragent);
};
