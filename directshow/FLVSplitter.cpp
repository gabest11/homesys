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
#include "FLVSplitter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// CFLVSplitterFilter

CFLVSplitterFilter::CFLVSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CFLVSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
{
}

CFLVSplitterFilter::~CFLVSplitterFilter()
{
	for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++) 
	{
		delete *i;
	}

	delete m_file;
}

bool CFLVSplitterFilter::Sync(__int64& pos)
{
	m_file->Seek(pos);

	__int64 limit = 0;
	__int64 cur = m_file->GetPos();
	__int64 len = m_file->GetLength();

	while((limit = len - cur) >= 11)
	{
		BYTE b;
		do {b = m_file->BitRead(8); cur++;}
		while(b != 8 && b != 9 && limit-- > 0);

		pos = cur;

		UINT32 DataSize = (UINT32)m_file->BitRead(24);
		UINT32 TimeStamp = (UINT32)m_file->BitRead(24);
		UINT32 TimeStampExt = (UINT32)m_file->BitRead(8);
		UINT32 StreamID = (UINT32)m_file->BitRead(24);

		if(StreamID == 0)
		{
			__int64 next = pos + 10 + DataSize;
			
			if(next <= len)
			{
				m_file->Seek(next);

				if(next == len || m_file->BitRead(32) == DataSize + 11)
				{
					pos -= 5;

					m_file->Seek(pos);

					return true;
				}
			}
		}
		
		cur = pos + 1;

		m_file->Seek(cur);
	}

	return false;
}

HRESULT CFLVSplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	delete m_file;

	m_file = new CBaseSplitterFile(pAsyncReader, hr);

	if(FAILED(hr)) 
	{
		delete m_file;
		
		m_file = NULL;
		
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	if(m_file->BitRead(24) != 'FLV' || m_file->BitRead(8) != 1)
	{
		return E_FAIL;
	}

	EXECUTE_ASSERT(m_file->BitRead(5) == 0); // TypeFlagsReserved

	bool fTypeFlagsAudio = !!m_file->BitRead(1);

	EXECUTE_ASSERT(m_file->BitRead(1) == 0); // TypeFlagsReserved

	bool fTypeFlagsVideo = !!m_file->BitRead(1);

	m_offset = (UINT32)m_file->BitRead(32);

	// doh, these flags aren't always telling the truth

	fTypeFlagsAudio = fTypeFlagsVideo = true;

	m_file->Seek(m_offset);

	CBaseSplitterFile::FLVPacket t;

	for(int i = 0; m_file->Read(t) && (fTypeFlagsVideo || fTypeFlagsAudio) && i < 100; i++)
	{
		UINT64 next = m_file->GetPos() + t.size;

		std::wstring name;

		CMediaType mt;

		mt.SetSampleSize(1);

		if(t.type == CBaseSplitterFile::FLVPacket::Audio && fTypeFlagsAudio)
		{
			fTypeFlagsAudio = false;

			name = L"Audio";

			CBaseSplitterFile::FLVAudioPacket at;
			
			m_file->Read(at, t.size, &mt);
		}
		else if(t.type == CBaseSplitterFile::FLVPacket::Video && fTypeFlagsVideo)
		{
			fTypeFlagsVideo = false;

			name = L"Video";

			CBaseSplitterFile::FLVVideoPacket vt;
			
			m_file->Read(vt, t.size, &mt);
		}

		if(mt.subtype != GUID_NULL)
		{
			std::vector<CMediaType> mts;

			mts.push_back(mt);

			CBaseSplitterOutputPin* pin = new CBaseSplitterOutputPin(mts, name.c_str(), this, &hr);

			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(t.type, pin)));
		}

		m_file->Seek(next);
	}

	__int64 pos = max(m_offset, m_file->GetLength() - 64*1024);

	if(Sync(pos))
	{
		CBaseSplitterFile::FLVPacket t;
		CBaseSplitterFile::FLVVideoPacket vt;
		CBaseSplitterFile::FLVAudioPacket at;

		while(m_file->Read(t))
		{
			UINT64 next = m_file->GetPos() + t.size;

			if(t.type == 8 && m_file->Read(at, t.size) || t.type == 9 && m_file->Read(vt, t.size))
			{
				m_rtDuration = std::max<__int64>(m_rtDuration, 10000i64 * t.timestamp); 
			}

			m_file->Seek(next);
		}
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool CFLVSplitterFilter::DemuxInit()
{
	return true;
}

void CFLVSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if(m_rtDuration == 0 || rt <= 0)
	{
		m_file->Seek(m_offset);
	}
	else
	{
		bool fVideo = !!GetOutputPin(CBaseSplitterFile::FLVPacket::Video);
		bool fAudio = !!GetOutputPin(CBaseSplitterFile::FLVPacket::Audio);

		CBaseSplitterFile::FLVPacket t;

		__int64 start = m_offset;
		__int64 stop = m_file->GetLength();

		REFERENCE_TIME rtStart = 0;
		REFERENCE_TIME rtStop = m_rtDuration;

		for(int i = 0; i < 3 && rtStop > rtStart; i++)
		{
			__int64 pos = start + 1.0f * (rt - rtStart) / (rtStop - rtStart) * (stop - start);

			if(!Sync(pos))
			{
				ASSERT(0);

				m_file->Seek(m_offset);

				return;
			}

			if(m_file->Read(t))
			{
				REFERENCE_TIME ts = 10000i64 * t.timestamp;

				if(ts >= rt)
				{
					start = pos;
					rtStart = ts;
				}
				else
				{
					stop = pos;
					rtStop = ts;
				}
			}

			m_file->Seek(pos);
		}
/*
		__int64 pos = m_offset + 1.0 * rt / m_rtDuration * (m_file->GetAvailable() - m_offset);

		if(!Sync(pos))
		{
			ASSERT(0);
			m_file->Seek(m_offset);
			return;
		}

		Tag t;
*/
		while(m_file->Read(t))
		{
			if(10000i64 * t.timestamp >= rt)
			{
				m_file->Seek(m_file->GetPos() - 15);
				
				break;
			}

			m_file->Seek(m_file->GetPos() + t.size);
		}

		CBaseSplitterFile::FLVVideoPacket vt;
		CBaseSplitterFile::FLVAudioPacket at;

		while(m_file->GetPos() >= m_offset && (fAudio || fVideo) && m_file->Read(t))
		{
			UINT64 prev = m_file->GetPos() - 15 - t.prevsize - 4;

			if(10000i64 * t.timestamp <= rt)
			{
				switch(t.type)
				{
				case CBaseSplitterFile::FLVPacket::Video:
					if(m_file->Read(vt, t.size) && vt.type == CBaseSplitterFile::FLVVideoPacket::Intra)
						fVideo = false;
					break;
				case CBaseSplitterFile::FLVPacket::Audio:
					if(m_file->Read(at, t.size))
						fAudio = false;
					break;
				}
			}

			m_file->Seek(prev);
		}

		if(fAudio || fVideo)
		{
			ASSERT(0);

			m_file->Seek(m_offset);
		}
	}
}

bool CFLVSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	for(auto i = m_h264queue.begin(); i != m_h264queue.end(); i++) 
	{
		delete *i;
	}

	m_h264queue.clear();

	CBaseSplitterFile::FLVPacket t;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_file->GetPos() < m_file->GetLength())
	{
		if(!m_file->Read(t))
		{
			break;
		}

		if(t.size == 0)
		{
			continue;
		}

		UINT64 next = m_file->GetPos() + t.size;

		do
		{
			CBaseSplitterFile::FLVVideoPacket vt;
			CBaseSplitterFile::FLVAudioPacket at;

			if(t.type == CBaseSplitterFile::FLVPacket::Video && m_file->Read(vt, t.size)
			|| t.type == CBaseSplitterFile::FLVPacket::Audio && m_file->Read(at, t.size))
			{
				if(t.type == CBaseSplitterFile::FLVPacket::Video && vt.codec == CBaseSplitterFile::FLVVideoPacket::AVC
				|| t.type == CBaseSplitterFile::FLVPacket::Audio && at.format == CBaseSplitterFile::FLVAudioPacket::AAC)
				{
					if(m_file->BitRead(8) != 1)
					{
						continue;
					}
				}

				CPacket* p = new CPacket();

				p->id = t.type;

				p->start = 10000i64 * t.timestamp; 
				p->stop = p->start + 1;

				p->flags |= CPacket::TimeValid;

				if(t.type == CBaseSplitterFile::FLVPacket::Video && vt.type == CBaseSplitterFile::FLVVideoPacket::Intra
				|| t.type == CBaseSplitterFile::FLVPacket::Audio)
				{
					p->flags |= CPacket::SyncPoint;
				}

				if(t.type == CBaseSplitterFile::FLVPacket::Video)
				{
					switch(vt.codec)
					{
					case CBaseSplitterFile::FLVVideoPacket::VP6: 
						m_file->BitRead(8); 
						break;
					case CBaseSplitterFile::FLVVideoPacket::VP6A: 
						m_file->BitRead(32); 
						break;
					case CBaseSplitterFile::FLVVideoPacket::AVC: 
						m_file->BitRead(24); 
						break;
					}
				}

				p->buff.resize(next - m_file->GetPos());

				m_file->ByteRead(&p->buff[0], p->buff.size());

				m_rtDuration = max(m_rtDuration, p->stop);

				if(t.type == CBaseSplitterFile::FLVPacket::Video && vt.codec == CBaseSplitterFile::FLVVideoPacket::AVC) 
				{
					bool b_frame = false;

					if(vt.type == CBaseSplitterFile::FLVVideoPacket::Inter)
					{
						CBaseSplitterFile f(p->buff.data(), p->buff.size());

						while(1)
						{
							DWORD size = (DWORD)f.BitRead(32);
							DWORD pos = (DWORD)f.GetPos();

							if(pos + size > f.GetRemaining()) break;

							BYTE forbidden_zero_bit = f.BitRead(1);
							BYTE nal_ref_idc = f.BitRead(2);
							BYTE nal_unit_type = f.BitRead(5);

							if(nal_unit_type == 1 || nal_unit_type == 5)
							{
								BYTE first_mb_in_slice = f.UExpGolombRead();
								BYTE slice_type = f.UExpGolombRead();
								BYTE pic_patameter_set_id = f.UExpGolombRead();
								BYTE frame_num = f.BitRead(2);

								if(slice_type == 1 || slice_type == 6) 
								{
									b_frame = true;
								}

								break;
							}

							f.Seek(pos + size);
						}
					}

					if(b_frame)
					{
						if(!m_h264queue.empty())
						{
							CPacket* p2 = m_h264queue.front();

							REFERENCE_TIME atpf = p->start - p2->start;

							p2->start += atpf;
							p2->stop += atpf;

							p->start -= atpf;
							p->stop -= atpf;

							m_h264queue.push_back(p);
						}
					}
					else
					{
						hr = S_OK;

						while(SUCCEEDED(hr) && !m_h264queue.empty())
						{
							CPacket* p2 = m_h264queue.front();

							m_h264queue.pop_front();

							hr = DeliverPacket(p2); 
						}

						m_h264queue.push_back(p);
					}
				}
				else
				{
					hr = DeliverPacket(p); 
				}
			}
		}
		while(0);

		m_file->Seek(next);
	}

	return true;
}

// CFLVSourceFilter

CFLVSourceFilter::CFLVSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CFLVSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	
	delete m_pInput;

	m_pInput = NULL;
}
/*
__if_exists(CFLVVideoDecoder) {

//
// CFLVVideoDecoder
//

CFLVVideoDecoder::CFLVVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseVideoFilter(NAME("CFLVVideoDecoder"), lpunk, phr, __uuidof(this))
{
	if(FAILED(*phr)) return;
}

CFLVVideoDecoder::~CFLVVideoDecoder()
{
}

STDMETHODIMP CFLVVideoDecoder::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CFLVVideoDecoder::Transform(IMediaSample* pIn)
{
	HRESULT hr;

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn)))
		return hr;

	long len = pIn->GetActualDataLength();

	if(m_dec.decodePacket(pDataIn, len) < 0)
		return S_FALSE;

	REFERENCE_TIME rtStart = _I64_MIN, rtStop = _I64_MIN;
	hr = pIn->GetTime(&rtStart, &rtStop);

	if(pIn->IsPreroll() == S_OK || rtStart < 0)
		return S_OK;

	int w, h, arx, ary;
	m_dec.getImageSize(&w, &h);
	m_dec.getDisplaySize(&arx, &ary);

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(hr = GetDeliveryBuffer(w, h, &pOut)) || FAILED(hr = pOut->GetPointer(&pDataOut)))
		return hr;

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);

	BYTE* yuv[3];
	int pitch;
	m_dec.getYUV(yuv, &pitch);

	if(m_pInput->CurrentMediaType().subtype == MEDIASUBTYPE_VP62)
	{
		yuv[0] += pitch * (h-1);
		yuv[1] += (pitch/2) * ((h/2)-1);
		yuv[2] += (pitch/2) * ((h/2)-1);
		pitch = -pitch;
	}

	CopyBuffer(pDataOut, yuv, w, h, pitch, MEDIASUBTYPE_I420);

	return m_pOutput->Deliver(pOut);
}

HRESULT CFLVVideoDecoder::CheckInputType(const CMediaType* mtIn)
{
	return mtIn->majortype == MEDIATYPE_Video 
		&& (mtIn->subtype == MEDIASUBTYPE_FLV4
		|| mtIn->subtype == MEDIASUBTYPE_VP62)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

}
*/