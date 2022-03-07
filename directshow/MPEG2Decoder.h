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

#include <dvdmedia.h>
#include <Videoacc.h>
#include "IMPEG2Decoder.h"
#include "DeCSSInputPin.h"
#include "BaseVideo.h"
#include "libmpeg2.h"
#include "../3rdparty/baseclasses/baseclasses.h"

class CSubpicInputPin;
class CClosedCaptionOutputPin;
struct libavcodec;

[uuid("39F498AF-1A09-4275-B193-673B0BA3D478")]
class CMPEG2DecoderFilter 
	: public CBaseVideoFilter
	, public IMPEG2DecoderFilter
{
	CSubpicInputPin* m_pSubpicInput;
	CClosedCaptionOutputPin* m_pClosedCaptionOutput;

	CLibMPEG2* m_libmpeg2;

	REFERENCE_TIME m_atpf;
	int m_waitkf;
	bool m_film;
	AM_SimpleRateChange m_rate;

	class FrameBuffer 
	{
	public:        
		int w, h, pitch;
		BYTE* base;
		BYTE* buff[6];
		REFERENCE_TIME start, stop;
		DWORD flags;

		FrameBuffer()
		{
			w = h = pitch = 0;
			base = NULL;
			memset(&buff, 0, sizeof(buff));
			start = stop = 0;
			flags = 0;
		}
        
		virtual ~FrameBuffer() 
		{
			Free();
		}

		void Init(int w, int h, int pitch)
		{
			this->w = w; 
			this->h = h; 
			this->pitch = pitch;

			int size = pitch * h;

			base = (BYTE*)_aligned_malloc(size * 3 + 6 * 32, 32);

			BYTE* p = base;
			
			buff[0] = p; p += (size + 31) & ~31;
			buff[3] = p; p += (size + 31) & ~31;
			buff[1] = p; p += (size / 4 + 31) & ~31;
			buff[4] = p; p += (size / 4 + 31) & ~31;
			buff[2] = p; p += (size / 4 + 31) & ~31;
			buff[5] = p; p += (size / 4 + 31) & ~31;
		}

		void Free()
		{
			if(base != NULL)
			{
				_aligned_free(base); 
			}

			base = NULL;
		}

	} m_fb;

	struct InterlaceSettings
	{
		DeinterlaceMethod di;
		bool interlaced;
		bool weave;
		bool tff;
		bool rff;
	} m_is;

	void OnDiscontinuity(bool transform);

	HRESULT DeliverPicture();

protected:
	HRESULT Transform(IMediaSample* pIn);

public:
	CMPEG2DecoderFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMPEG2DecoderFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT DeliverFast();
	HRESULT DeliverNormal();
	HRESULT Deliver();

	int GetPinCount();
	CBasePin* GetPin(int n);

    HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	HRESULT CheckConnect(PIN_DIRECTION dir, IPin* pPin);
    HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

protected:
	CCritSec m_csProps;
	DeinterlaceMethod m_di;
	bool m_forcedsubs;
	bool m_planarYUV;

public:

	// IMPEG2DecoderFilter

	STDMETHODIMP SetDeinterlaceMethod(DeinterlaceMethod di);
	STDMETHODIMP_(DeinterlaceMethod) GetDeinterlaceMethod();

	STDMETHODIMP EnableForcedSubtitles(bool enabled);
	STDMETHODIMP_(bool) IsForcedSubtitlesEnabled();

	STDMETHODIMP EnablePlanarYUV(bool enabled);
	STDMETHODIMP_(bool) IsPlanarYUVEnabled();
};

class CMPEG2DecoderInputPin : public CDeCSSInputPin
{
	LONG m_CorrectTS;

public:
    CMPEG2DecoderInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);

	CCritSec m_csRateLock;
	AM_SimpleRateChange m_ratechange;

	// IKsPropertySet

    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CSubpicInputPin : public CMPEG2DecoderInputPin
{
	class spu
	{
	public:
		std::vector<BYTE> m_buff;
		bool m_forced;
		REFERENCE_TIME m_start, m_stop; 
		DWORD m_offset[2];
		AM_PROPERTY_SPHLI m_sphli; // parsed
		AM_PROPERTY_SPHLI* m_psphli; // for the menu (optional)
		spu() : m_psphli(NULL) {memset(&m_sphli, 0, sizeof(m_sphli)); m_forced = false; m_start = m_stop = 0;}
		virtual ~spu() {delete m_psphli;}
		virtual bool Parse() = 0;
		virtual void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal) = 0;
	};

	class dvdspu : public spu
	{
	public:
		struct offset_t {REFERENCE_TIME rt; AM_PROPERTY_SPHLI sphli;};
		std::list<offset_t> m_offsets;
		bool Parse();
		void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal);
	};

	class cvdspu : public spu
	{
	public:
		AM_DVD_YUV m_sppal[2][4];
		cvdspu() {memset(m_sppal, 0, sizeof(m_sppal));}
		bool Parse();
		void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal);
	};

	class svcdspu : public spu
	{
	public:
		AM_DVD_YUV m_sppal[4];
		svcdspu() {memset(m_sppal, 0, sizeof(m_sppal));}
		bool Parse();
		void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal);
	};

	CCritSec m_csReceive;

	AM_PROPERTY_COMPOSIT_ON m_spon;
	AM_DVD_YUV m_sppal[16];
	bool m_fsppal;
	AM_PROPERTY_SPHLI* m_sphli; // temp
	std::list<spu*> m_sps;

protected:
	HRESULT Transform(IMediaSample* pSample);

public:
	CSubpicInputPin(CTransformFilter* pFilter, HRESULT* phr);
	virtual ~CSubpicInputPin();

	bool HasAnythingToRender(REFERENCE_TIME rt);
	void RenderSubpics(REFERENCE_TIME rt, BYTE** p, int w, int h);

    HRESULT CheckMediaType(const CMediaType* mtIn);
	HRESULT SetMediaType(const CMediaType* mtIn);

	// we shouldn't pass these to the filter from this pin

	STDMETHODIMP EndOfStream() {return S_OK;}
    STDMETHODIMP BeginFlush() {return S_OK;}
    STDMETHODIMP EndFlush();
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {return S_OK;}

	// IKsPropertySet

    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CClosedCaptionOutputPin : public CBaseOutputPin
{
public:
	CClosedCaptionOutputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

    HRESULT CheckMediaType(const CMediaType* mtOut);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

	HRESULT Deliver(const void* ptr, int len);
};
