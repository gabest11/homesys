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
#include "BaseVideo.h"
#include "../3rdparty/baseclasses/baseclasses.h"

struct libavcodec;

[uuid("1BD874E9-AC47-4117-BA8E-830141CD5571")]
class CH264DecoderFilter 
	: public CBaseVideoFilter
{
	libavcodec* m_libavcodec;

	REFERENCE_TIME m_atpf;
	int m_waitkf;

	class FrameBuffer 
	{
	public:        
		int w, h, pitch;
		BYTE* base;
		BYTE* buff[6];

		FrameBuffer()
		{
			w = h = pitch = 0;
			base = NULL;
			memset(&buff, 0, sizeof(buff));
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

	// h264

	class H264Buff 
	{
	public: 
		REFERENCE_TIME start, stop; 
		BYTE* buff; 
		int size, maxsize; 
		
		H264Buff() : buff(NULL), size(0), maxsize(0) {} 
		virtual ~H264Buff() {delete [] buff;}

	} m_h264;

	//

	void OnDiscontinuity(bool transform);

	HRESULT DecodeH264(BYTE* data, int len, REFERENCE_TIME start, REFERENCE_TIME stop);

protected:
	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt);
	HRESULT Transform(IMediaSample* pIn);

public:
	CH264DecoderFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CH264DecoderFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT EndFlush();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT StartStreaming();
	HRESULT StopStreaming();
};
