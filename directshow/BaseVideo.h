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
#include "../util/Vector.h"

class CBaseVideoFilter : public CTransformFilter
{
    HRESULT Receive(IMediaSample* pIn);

	// these are private for a reason, don't bother them

	Vector2i m_isize;
	Vector2i m_osize;
	Vector2i m_iar;
	Vector2i m_oar;

	int m_buffers;

	HRESULT ResetAllocator();

protected:
	CCritSec m_csReceive;

	bool m_dvd;
	bool m_evr;
	int m_late;

	Vector2i m_size;
	Vector2i m_ar;
	Vector2i m_realsize;

	HRESULT GetDeliveryBuffer(int w, int h, IMediaSample** ppOut);
	HRESULT CopyBuffer(BYTE* pOut, BYTE* pIn, int w, int h, int pitchIn, const GUID& subtype, bool interlaced = false);
	HRESULT CopyBuffer(BYTE* pOut, BYTE** ppIn, int w, int h, int pitchIn, const GUID& subtype, bool interlaced = false);

	struct VIDEO_OUTPUT_FORMATS
	{
		const GUID* subtype;
		WORD biPlanes;
		WORD biBitCount;
		DWORD biCompression;
	};

	static VIDEO_OUTPUT_FORMATS m_formats[];

	virtual void GetOutputSize(Vector2i& size, Vector2i& ar, Vector2i& realsize) {}
	virtual void GetOutputFormats(int& count, VIDEO_OUTPUT_FORMATS** formats);
	virtual HRESULT Transform(IMediaSample* pIn) = 0;
	virtual bool CanResetAllocator() {return true;}

	void CorrectInputSize(const Vector2i& size) {m_isize = size;}
	void SetInterlaceFlags(IMediaSample* pMS, bool weave, bool tff, bool rff);

public:
	CBaseVideoFilter(TCHAR* pName, LPUNKNOWN lpunk, HRESULT* phr, REFCLSID clsid, long buffers = 1);
	virtual ~CBaseVideoFilter();

	int GetPinCount();
	CBasePin* GetPin(int n);

    HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckOutputType(const CMediaType& mtOut);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt);
	HRESULT CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin);
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT AlterQuality(Quality q);

	HRESULT ReconnectOutput(int w, int h);
	void SetAspectRatio(const Vector2i& ar);
};

class CBaseVideoInputAllocator : public CMemAllocator
{
	CMediaType m_mt;

public:
	CBaseVideoInputAllocator(HRESULT* phr);
	void SetMediaType(const CMediaType& mt);
	STDMETHODIMP GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags);
};

class CBaseVideoInputPin : public CTransformInputPin
{
	CBaseVideoInputAllocator* m_pAllocator;

public:
	CBaseVideoInputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);
	virtual ~CBaseVideoInputPin();

	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
};

class CBaseVideoOutputPin : public CTransformOutputPin
{
public:
	CBaseVideoOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);

    HRESULT CheckMediaType(const CMediaType* mtOut);
};
