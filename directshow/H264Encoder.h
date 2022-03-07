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

struct libavcodec;

[uuid("1549FDCA-6B5A-425F-BA0C-AD1FC5B47C61")]
class CH264EncoderFilter : public CTransformFilter
{
	libavcodec* m_libavcodec;

	std::vector<BYTE> m_buff;
	int m_size;

	std::list<REFERENCE_TIME> m_start;
	int m_frame;

	HRESULT Transform(IMediaSample* pIn);

    HRESULT StartStreaming();
    HRESULT StopStreaming();

public:
	CH264EncoderFilter();
	virtual ~CH264EncoderFilter();

	HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
    HRESULT Receive(IMediaSample* pIn);
};
