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

namespace MatroskaReader
{
	class CMatroskaNode;

	class CANSI : public std::string 
	{
	public: 
		HRESULT Parse(CMatroskaNode* pMN);

		void operator = (LPCSTR s)
		{
			__super::operator = (s);
		}
	};

	class CUTF8 : public std::wstring 
	{
	public: 
		HRESULT Parse(CMatroskaNode* pMN);

		void operator = (LPCWSTR s)
		{
			__super::operator = (s);
		}
	};

	template<class T, class BASE>
	class CSimpleVar
	{
	protected:
		T m_val;
		bool m_valid;

	public:
		CSimpleVar(T val = 0) : m_val(val), m_valid(false) {}
		BASE& operator = (const BASE& v) {m_val = v.m_val; m_valid = true; return *this;}
		BASE& operator = (T val) {m_val = val; m_valid = true; return *this;}
		operator T() const {return m_val;}
		BASE& Set(T val) {m_val = val; m_valid = true; return *(BASE*)this;}
		bool IsValid() {return m_valid;}
		virtual HRESULT Parse(CMatroskaNode* pMN);
	};

	class CUInt : public CSimpleVar<UINT64, CUInt> {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CInt : public CSimpleVar<INT64, CInt> {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CByte : public CSimpleVar<BYTE, CByte> {};
	class CShort : public CSimpleVar<short, CShort> {};
	class CFloat : public CSimpleVar<double, CFloat> {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CID : public CSimpleVar<DWORD, CID> {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CLength : public CSimpleVar<UINT64, CLength> {bool m_sign; public: CLength(bool sign = false) : m_sign(sign) {} HRESULT Parse(CMatroskaNode* pMN);};
	class CSignedLength : public CLength {public: CSignedLength() : CLength(true) {}};

	class ContentCompression;

	class CBinary : public std::vector<BYTE>
	{
	public:
		std::string ToString();
		bool Compress(ContentCompression& cc);
		bool Decompress(ContentCompression& cc);
		HRESULT Parse(CMatroskaNode* pMN);
	};

	template<class T>
	class CNode : public std::list<T*> 
	{
	public: 
		virtual ~CNode()
		{
			for(auto i = begin(); i != end(); i++)
			{
				delete *i;
			}
		}

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class EBML
	{
	public:
		CUInt EBMLVersion, EBMLReadVersion;
		CUInt EBMLMaxIDLength, EBMLMaxSizeLength;
		CANSI DocType;
		CUInt DocTypeVersion, DocTypeReadVersion;

		HRESULT Parse(CMatroskaNode* pMN);
	};

		class Info
		{
		public:
			CBinary SegmentUID, PrevUID, NextUID;
			CUTF8 SegmentFilename, PrevFilename, NextFilename;
			CUInt TimeCodeScale; // [ns], default: 1.000.000
			CFloat Duration;
			CInt DateUTC;
			CUTF8 Title, MuxingApp, WritingApp;

			Info() {TimeCodeScale.Set(1000000ui64);}
			HRESULT Parse(CMatroskaNode* pMN);
		};

			class SeekHead
			{
			public:
				CID SeekID;
				CUInt SeekPosition;

				HRESULT Parse(CMatroskaNode* pMN);
			};

		class Seek
		{
		public:
			CNode<SeekHead> SeekHeads;

			HRESULT Parse(CMatroskaNode* pMN);
		};

				class TimeSlice
				{
				public:
					CUInt LaceNumber, FrameNumber;
					CUInt Delay, Duration;

					HRESULT Parse(CMatroskaNode* pMN);
				};

				class SimpleBlock
				{
				public:
					CLength TrackNumber;
					CInt TimeCode;
					CByte Lacing;
					std::list<CBinary*> BlockData;

					virtual ~SimpleBlock()
					{
						for(auto i = BlockData.begin(); i != BlockData.end(); i++)
						{
							delete *i;
						}
					}

					HRESULT Parse(CMatroskaNode* pMN, bool fFull);
				};

					class BlockMore
					{
					public:
						CInt BlockAddID;
						CBinary BlockAdditional;

						BlockMore() {BlockAddID.Set(1);}
						HRESULT Parse(CMatroskaNode* pMN);
					};

				class BlockAdditions
				{
				public:
					CNode<BlockMore> bm;

					HRESULT Parse(CMatroskaNode* pMN);
				};

			class BlockGroup
			{
			public:
				SimpleBlock Block;
//				BlockVirtual
				CUInt BlockDuration;
				CUInt ReferencePriority;
				CInt ReferenceBlock;
				CInt ReferenceVirtual;
				CBinary CodecState;
				CNode<TimeSlice> TimeSlices;
				BlockAdditions ba;

				HRESULT Parse(CMatroskaNode* pMN, bool fFull);
			};

			class CBlockGroupNode : public CNode<BlockGroup>
			{
			public:
				HRESULT Parse(CMatroskaNode* pMN, bool fFull);
			};

			class CSimpleBlockNode : public CNode<SimpleBlock>
			{
			public:
				HRESULT Parse(CMatroskaNode* pMN, bool fFull);
			};

		class Cluster
		{
		public:
			CUInt TimeCode, Position, PrevSize;
			CBlockGroupNode BlockGroups;
			CSimpleBlockNode SimpleBlocks;

			HRESULT Parse(CMatroskaNode* pMN);
			HRESULT ParseTimeCode(CMatroskaNode* pMN);
		};

				class Video
				{
				public:
					CUInt FlagInterlaced, StereoMode;
					CUInt PixelWidth, PixelHeight, DisplayWidth, DisplayHeight, DisplayUnit;
					CUInt AspectRatioType;
					CUInt ColourSpace;
					CFloat GammaValue;
					CFloat FramePerSec;

					HRESULT Parse(CMatroskaNode* pMN);
				};

				class Audio
				{
				public:
					CFloat SamplingFrequency;
					CFloat OutputSamplingFrequency;
					CUInt Channels;
					CBinary ChannelPositions;
					CUInt BitDepth;

					Audio() {SamplingFrequency.Set(8000.0); Channels.Set(1);}
					HRESULT Parse(CMatroskaNode* pMN);
				};

						class ContentCompression
						{
						public:
							CUInt ContentCompAlgo; enum {ZLIB, BZLIB, LZO1X, HDRSTRIP};
							CBinary ContentCompSettings;

							ContentCompression() {ContentCompAlgo.Set(ZLIB);}
							HRESULT Parse(CMatroskaNode* pMN);
						};

						class ContentEncryption
						{
						public:
							CUInt ContentEncAlgo; enum {UNKE, DES, THREEDES, TWOFISH, BLOWFISH, AES};
							CBinary ContentEncKeyID, ContentSignature, ContentSigKeyID;
							CUInt ContentSigAlgo; enum {UNKS, RSA};
							CUInt ContentSigHashAlgo; enum {UNKSH, SHA1_160, MD5};

							ContentEncryption() {ContentEncAlgo.Set(0); ContentSigAlgo.Set(0); ContentSigHashAlgo.Set(0);}
							HRESULT Parse(CMatroskaNode* pMN);
						};

					class ContentEncoding
					{
					public:
						CUInt ContentEncodingOrder;
						CUInt ContentEncodingScope; enum {AllFrameContents = 1, TracksPrivateData = 2};
						CUInt ContentEncodingType; enum {Compression, Encryption};
						ContentCompression cc;
						ContentEncryption ce;

						ContentEncoding() {ContentEncodingOrder.Set(0); ContentEncodingScope.Set(AllFrameContents); ContentEncodingType.Set(Compression);}
						HRESULT Parse(CMatroskaNode* pMN);
					};

				class ContentEncodings
				{
				public:
					CNode<ContentEncoding> ce;

					ContentEncodings() {}
					HRESULT Parse(CMatroskaNode* pMN);
				};

			class TrackEntry
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
				CFloat TrackTimecodeScale;
				enum {NoDesc = 0, DescVideo = 1, DescAudio = 2};
				int DescType;
				Video v;
				Audio a;
				ContentEncodings ces;
				TrackEntry() {DescType = NoDesc; FlagEnabled.Set(1); FlagDefault.Set(1); FlagLacing.Set(1); }
				HRESULT Parse(CMatroskaNode* pMN);

				bool Expand(CBinary& data, UINT64 Scope);
			};

		class Track
		{
		public:
			CNode<TrackEntry> TrackEntries;

			HRESULT Parse(CMatroskaNode* pMN);
		};

				class CueReference
				{
				public:
					CUInt CueRefTime, CueRefCluster, CueRefNumber, CueRefCodecState;

					HRESULT Parse(CMatroskaNode* pMN);
				};

			class CueTrackPosition
			{
			public:
				CUInt CueTrack, CueClusterPosition, CueBlockNumber, CueCodecState;
				CNode<CueReference> CueReferences;

				HRESULT Parse(CMatroskaNode* pMN);
			};

		class CuePoint
		{
		public:
			CUInt CueTime;
			CNode<CueTrackPosition> CueTrackPositions;

			HRESULT Parse(CMatroskaNode* pMN);
		};

		class Cue
		{
		public:
			CNode<CuePoint> CuePoints;

			HRESULT Parse(CMatroskaNode* pMN);
		};

			class AttachedFile
			{
			public:
				CUTF8 FileDescription;
				CUTF8 FileName;
				CANSI FileMimeType;
				UINT64 FileDataPos, FileDataLen; // BYTE* FileData
				CUInt FileUID;

				AttachedFile() {FileDataPos = FileDataLen = 0;}
				HRESULT Parse(CMatroskaNode* pMN);
			};

		class Attachment
		{
		public:
			CNode<AttachedFile> AttachedFiles;

			HRESULT Parse(CMatroskaNode* pMN);
		};

					class ChapterDisplay
					{
					public:
						CUTF8 ChapString;
						CANSI ChapLanguage;
						CANSI ChapCountry;

						ChapterDisplay() {ChapLanguage = "eng";}
						HRESULT Parse(CMatroskaNode* pMN);
					};

				class ChapterAtom
				{
				public:
					CUInt ChapterUID;
					CUInt ChapterTimeStart, ChapterTimeEnd, ChapterFlagHidden, ChapterFlagEnabled;
//					CNode<CUInt> ChapterTracks; // TODO
					CNode<ChapterDisplay> ChapterDisplays;
					CNode<ChapterAtom> ChapterAtoms;
					
					ChapterAtom() {ChapterUID.Set(rand());ChapterFlagHidden.Set(0);ChapterFlagEnabled.Set(1);}
					HRESULT Parse(CMatroskaNode* pMN);
					ChapterAtom* FindChapterAtom(UINT64 id);
				};

			class EditionEntry : public ChapterAtom
			{
			public:
				HRESULT Parse(CMatroskaNode* pMN);
			};

		class Chapter
		{
		public:
			CNode<EditionEntry> EditionEntries;

			HRESULT Parse(CMatroskaNode* pMN);
		};

	class Segment
	{
	public:
		UINT64 pos, len;
		Info SegmentInfo;
		CNode<Seek> MetaSeekInfo;
		CNode<Cluster> Clusters;
		CNode<Track> Tracks;
		CNode<Cue> Cues;
		CNode<Attachment> Attachments;
		CNode<Chapter> Chapters;
		// TODO: Chapters
		// TODO: Tags

		HRESULT Parse(CMatroskaNode* pMN);
		HRESULT ParseMinimal(CMatroskaNode* pMN);

		UINT64 GetMasterTrack();

		REFERENCE_TIME GetRefTime(INT64 t) {return t*(REFERENCE_TIME)(SegmentInfo.TimeCodeScale)/100;}
		ChapterAtom* FindChapterAtom(UINT64 id, int nEditionEntry = 0);
	};

	class CMatroskaReaderFile : public CBaseSplitterFile
	{
	public:
		CMatroskaReaderFile(IAsyncReader* pAsyncReader, HRESULT& hr);
		virtual ~CMatroskaReaderFile() {}

		HRESULT Init();

		//using CBaseSplitterFile::Read;
		template <class T> HRESULT Read(T& var);

		EBML m_ebml;
		Segment m_segment;
		REFERENCE_TIME m_start;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CMatroskaNode
	{
		CMatroskaNode* m_pParent;
		CMatroskaReaderFile* m_pMF;

		bool Resync();

	public:
		CID m_id;
		CLength m_len;
		UINT64 m_filepos, m_start;

		HRESULT Parse();

	public:
		CMatroskaNode(CMatroskaReaderFile* pMF); // creates the root
		CMatroskaNode(CMatroskaNode* pParent);

		CMatroskaNode* Parent() {return m_pParent;}
		CMatroskaNode* Child(DWORD id = 0, bool search = true); // *
		bool Next(bool same = false, DWORD id_to_stop_at = 0);
		bool Find(DWORD id, bool search = true);

		UINT64 FindPos(DWORD id, UINT64 start = 0);

		void SeekTo(UINT64 pos);
		UINT64 GetPos(), GetLength();
		template <class T> HRESULT Read(T& var);
		HRESULT Read(BYTE* pData, UINT64 len);

		CMatroskaNode* Copy(); // *

		CMatroskaNode* GetFirstBlock(); // *
		bool NextBlock();
	};
}
