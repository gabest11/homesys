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

#include "BaseSplitter.h"

namespace MatroskaWriter
{
	class CID
	{
	protected:
		DWORD m_id;
		UINT64 HeaderSize(UINT64 len);
		HRESULT HeaderWrite(IStream* pStream);

	public:
		CID(DWORD id);
		DWORD GetID() {return m_id;}
		virtual UINT64 Size(bool fWithHeader = true);
		virtual HRESULT Write(IStream* pStream);
	};

	class CLength : public CID
	{
		UINT64 m_len;

	public: 
		CLength(UINT64 len = 0) : CID(0), m_len(len) {}
		operator UINT64() {return m_len;}
		UINT64 Size(bool fWithHeader = false);
		HRESULT Write(IStream* pStream);
	};

	class CBinary : public std::vector<BYTE>, public CID
	{
	public:
		CBinary(DWORD id) : CID(id) {}
		CBinary& operator = (const CBinary& b) {__super::operator = ((std::vector<BYTE>&)b); return *this;}
		operator BYTE*() {return (BYTE*)data();}
		CBinary& Set(std::string s) {resize(s.size() + 1); strcpy((char*)data(), s.c_str()); return *this;}
		void SetAt(size_t index, BYTE b) {std::vector<BYTE>::operator[] (index) = b;}
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};

	class CANSI : public std::string, public CID
	{
	public: 
		CANSI(DWORD id) : CID(id) {}
		CANSI& Set(const std::string& s) {std::string::operator = (s); return *this;}
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};

	class CUTF8 : public std::wstring, public CID
	{
	public: 
		CUTF8(DWORD id) : CID(id) {}
		CUTF8& Set(const std::wstring& s) {std::wstring::operator = (s); return *this;}
		UINT64 Size(bool fWithHeader = true); 
		HRESULT Write(IStream* pStream);
	};

	template<class T, class BASE>
	class CSimpleVar : public CID
	{
	protected:
		T m_val;
		bool m_fSet;

	public:
		explicit CSimpleVar(DWORD id, T val = 0) : CID(id), m_val(val) {m_fSet = !!val;}
		operator T() {return m_val;}
		BASE& Set(T val) {m_val = val; m_fSet = true; return(*(BASE*)this);}
		void UnSet() {m_fSet = false;}
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};

	class CUInt : public CSimpleVar<UINT64, CUInt>
	{
	public:
		explicit CUInt(DWORD id, UINT64 val = 0) : CSimpleVar<UINT64, CUInt>(id, val) {}
		UINT64 Size(bool fWithHeader = true); 
		HRESULT Write(IStream* pStream);
	};

	class CInt : public CSimpleVar<INT64, CInt>
	{
	public: 
		explicit CInt(DWORD id, INT64 val = 0) : CSimpleVar<INT64, CInt>(id, val) {}
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};

	class CByte : public CSimpleVar<BYTE, CByte>
	{
	public: 
		explicit CByte(DWORD id, BYTE val = 0) : CSimpleVar<BYTE, CByte>(id, val) {}
	};

	class CShort : public CSimpleVar<short, CShort>
	{
	public: 
		explicit CShort(DWORD id, short val = 0) : CSimpleVar<short, CShort>(id, val) {}
	};

	class CFloat : public CSimpleVar<float, CFloat>
	{
	public: 
		explicit CFloat(DWORD id, float val = 0) : CSimpleVar<float, CFloat>(id, val) {}
	};

	template<class T>
	class CNode : public std::list<T*>
	{
	public: 
		virtual ~CNode()
		{
			RemoveAll();
		}

		UINT64 Size(bool fWithHeader = true)
		{
			UINT64 len = 0;
			for(auto i = begin(); i != end(); i++) len += (*i)->Size(fWithHeader);
			return len;
		}

		HRESULT Write(IStream* pStream)
		{
			HRESULT hr;
			for(auto i = begin(); i != end(); i++) if(FAILED(hr = (*i)->Write(pStream))) return hr;
			return S_OK;
		}

		void RemoveAll()
		{
			for(auto i = begin(); i != end(); i++)
			{
				delete *i;
			}

			clear();
		}
	};

	class EBML : public CID
	{
	public:
		CUInt EBMLVersion, EBMLReadVersion;
		CUInt EBMLMaxIDLength, EBMLMaxSizeLength;
		CANSI DocType;
		CUInt DocTypeVersion, DocTypeReadVersion;

		EBML(DWORD id = 0x1A45DFA3);
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};

		class Info : public CID
		{
		public:
			CBinary SegmentUID, PrevUID, NextUID;
			CUTF8 SegmentFilename, PrevFilename, NextFilename;
			CUInt TimeCodeScale; // [ns], default: 1.000.000
			CFloat Duration;
			CInt DateUTC;
			CUTF8 Title, MuxingApp, WritingApp;

			Info(DWORD id = 0x1549A966);
			UINT64 Size(bool fWithHeader = true);
			HRESULT Write(IStream* pStream);
		};

				class Video : public CID
				{
				public:
					CUInt FlagInterlaced, StereoMode;
					CUInt PixelWidth, PixelHeight, DisplayWidth, DisplayHeight, DisplayUnit;
					CUInt AspectRatioType;
					CUInt ColourSpace;
					CFloat GammaValue;
					CFloat FramePerSec;

					Video(DWORD id = 0xE0);
					UINT64 Size(bool fWithHeader = true);
					HRESULT Write(IStream* pStream);
				};

				class Audio : public CID
				{
				public:
					CFloat SamplingFrequency;
					CFloat OutputSamplingFrequency;
					CUInt Channels;
					CBinary ChannelPositions;
					CUInt BitDepth;

					Audio(DWORD id = 0xE1);
					UINT64 Size(bool fWithHeader = true);
					HRESULT Write(IStream* pStream);
				};

			class TrackEntry : public CID
			{
			public:
				enum {TypeVideo = 1, TypeAudio = 2, TypeComplex = 3, TypeLogo = 0x10, TypeSubtitle = 0x11, TypeControl = 0x20};
				CUInt TrackNumber, TrackUID, TrackType;
				CUInt FlagEnabled, FlagDefault, FlagLacing;
				CUInt MinCache, MaxCache;
				CUTF8 Name;
				CANSI Language;
				CBinary CodecID;
				CBinary CodecPrivate;
				CUTF8 CodecName;
				CUTF8 CodecSettings;
				CANSI CodecInfoURL;
				CANSI CodecDownloadURL;
				CUInt CodecDecodeAll;
				CUInt TrackOverlay;
				CUInt DefaultDuration;
				enum {NoDesc = 0, DescVideo = 1, DescAudio = 2};
				int DescType;
				Video v;
				Audio a;

				TrackEntry(DWORD id = 0xAE);
				UINT64 Size(bool fWithHeader = true);
				HRESULT Write(IStream* pStream);
			};

		class Track : public CID
		{
		public:
			CNode<TrackEntry> TrackEntries;

			Track(DWORD id = 0x1654AE6B);
			UINT64 Size(bool fWithHeader = true);
			HRESULT Write(IStream* pStream);
		};

				class CBlock : public CID
				{
				public:
					CLength TrackNumber;
					REFERENCE_TIME TimeCode, TimeCodeStop;
					CNode<CBinary> BlockData;

					CBlock(DWORD id = 0xA1);
					UINT64 Size(bool fWithHeader = true);
					HRESULT Write(IStream* pStream);
				};

			class BlockGroup : public CID
			{
			public:
				CUInt BlockDuration;
				CUInt ReferencePriority;
				CInt ReferenceBlock;
				CInt ReferenceVirtual;
				CBinary CodecState;
				CBlock Block;
//				CNode<TimeSlice> TimeSlices;

				BlockGroup(DWORD id = 0xA0);
				UINT64 Size(bool fWithHeader = true);
				HRESULT Write(IStream* pStream);
			};

		class Cluster : public CID
		{
		public:
			CUInt TimeCode, Position, PrevSize;
			CNode<BlockGroup> BlockGroups;

			Cluster(DWORD id = 0x1F43B675);
			UINT64 Size(bool fWithHeader = true);
			HRESULT Write(IStream* pStream);
		};

/*					class CueReference : public CID
					{
					public:
						CUInt CueRefTime, CueRefCluster, CueRefNumber, CueRefCodecState;

						CueReference(DWORD id = 0xDB);
						UINT64 Size(bool fWithHeader = true);
						HRESULT Write(IStream* pStream);
					};
*/
				class CueTrackPosition : public CID
				{
				public:
					CUInt CueTrack, CueClusterPosition, CueBlockNumber, CueCodecState;
//					CNode<CueReference> CueReferences;

					CueTrackPosition(DWORD id = 0xB7);
					UINT64 Size(bool fWithHeader = true);
					HRESULT Write(IStream* pStream);
				};

			class CuePoint : public CID
			{
			public:
				CUInt CueTime;
				CNode<CueTrackPosition> CueTrackPositions;

				CuePoint(DWORD id = 0xBB);
				UINT64 Size(bool fWithHeader = true);
				HRESULT Write(IStream* pStream);
			};

		class Cue : public CID
		{
		public:
			CNode<CuePoint> CuePoints;

			Cue(DWORD id = 0x1C53BB6B);
			UINT64 Size(bool fWithHeader = true);
			HRESULT Write(IStream* pStream);
		};

				class SeekID : public CID
				{
					CID m_id;
				public:
					SeekID(DWORD id = 0x53AB);
					void Set(DWORD id) {m_id = id;}
					UINT64 Size(bool fWithHeader = true);
					HRESULT Write(IStream* pStream);
				};

			class SeekHead : public CID
			{
			public:
				SeekID ID;
				CUInt Position;

				SeekHead(DWORD id = 0x4DBB);
				UINT64 Size(bool fWithHeader = true);
				HRESULT Write(IStream* pStream);
			};

		class Seek : public CID
		{
		public:
			CNode<SeekHead> SeekHeads;

			Seek(DWORD id = 0x114D9B74);
			UINT64 Size(bool fWithHeader = true);
			HRESULT Write(IStream* pStream);
		};

	class Segment : public CID
	{
	public:
		Segment(DWORD id = 0x18538067);
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};

	class Tags : public CID
	{
	public:
		// TODO

		Tags(DWORD id = 0x1254C367);
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};

	class Void : public CID
	{
		UINT64 m_len;
	public:
		Void(UINT64 len, DWORD id = 0xEC);
		UINT64 Size(bool fWithHeader = true);
		HRESULT Write(IStream* pStream);
	};
}
