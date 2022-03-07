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

#include <string>
#include "../3rdparty/baseclasses/baseclasses.h"
#include "HTTPReader.h"

[uuid("6DDB4EE7-45A0-4459-A508-BD77B32C91B2")]
interface ISyncReader : public IUnknown
{
	STDMETHOD_(void, SetBreakEvent) (HANDLE hBreakEvent) = 0;
	STDMETHOD_(bool, HasErrors) () = 0;
	STDMETHOD_(void, ClearErrors) () = 0;
};

[uuid("7D55F67A-826E-40B9-8A7D-3DF0CBBD272D")]
interface IFileHandle : public IUnknown
{
	STDMETHOD_(HANDLE, GetFileHandle)() = 0;
};

class CAsyncFileReader : public CUnknown, public IAsyncReader, public ISyncReader, public IFileHandle
{
protected:
	std::wstring m_fn;
	HANDLE m_file;
	__int64 m_len;
	HANDLE m_hBreakEvent;
	LONG m_lOsError; // CFileException::m_lOsError

public:
	CAsyncFileReader(LPCWSTR fn, HRESULT& hr);
	virtual ~CAsyncFileReader();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAsyncReader

	STDMETHODIMP RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual) {return E_NOTIMPL;}
    STDMETHODIMP Request(IMediaSample* pSample, DWORD_PTR dwUser) {return E_NOTIMPL;}
    STDMETHODIMP WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser) {return E_NOTIMPL;}
	STDMETHODIMP SyncReadAligned(IMediaSample* pSample) {return E_NOTIMPL;}
	STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
	STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
	STDMETHODIMP BeginFlush() {return E_NOTIMPL;}
	STDMETHODIMP EndFlush() {return E_NOTIMPL;}

	// ISyncReader

	STDMETHODIMP_(void) SetBreakEvent(HANDLE hBreakEvent) {m_hBreakEvent = hBreakEvent;}
	STDMETHODIMP_(bool) HasErrors() {return m_lOsError != 0;}
	STDMETHODIMP_(void) ClearErrors() {m_lOsError = 0;}

	// IFileHandle

	STDMETHODIMP_(HANDLE) GetFileHandle();
};

class CAsyncMemReader : public CUnknown, public IAsyncReader
{
	const BYTE* m_buff;
	int m_size;

public:
	CAsyncMemReader(const BYTE* buff, int size);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAsyncReader

	STDMETHODIMP RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual);
    STDMETHODIMP Request(IMediaSample* pSample, DWORD_PTR dwUser);
	STDMETHODIMP WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser);
	STDMETHODIMP SyncReadAligned(IMediaSample* pSample);
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
	STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
	STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
};

class CAsyncHttpStreamReader : public CUnknown, public IAsyncReader
{
	CHttpStream* m_stream;

public:
	CAsyncHttpStreamReader(CHttpStream* stream);
	~CAsyncHttpStreamReader();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAsyncReader

	STDMETHODIMP RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual);
    STDMETHODIMP Request(IMediaSample* pSample, DWORD_PTR dwUser);
	STDMETHODIMP WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser);
	STDMETHODIMP SyncReadAligned(IMediaSample* pSample);
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
	STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
	STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
};
