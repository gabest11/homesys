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

#define DEFAULT_CACHE_LENGTH (4096 * 16 * 2)

class CBaseSplitterFile
{
	CComPtr<IAsyncReader> m_reader;
	__int64 m_pos, m_len;
	struct {std::vector<BYTE> buff; __int64 pos, len;} m_cache;
	union {struct {UINT64 buff; int len;}; DWORD buff32;} m_bit;
	int m_tslen; // transport stream packet length (188 or 192 bytes, auto-detected)
	const BYTE* m_mem;

protected:
	__int64 GetRealPos() {return m_pos;}

	virtual HRESULT Read(BYTE* data, __int64 len); // use ByteRead

public:
	CBaseSplitterFile(IAsyncReader* reader, HRESULT& hr, int cachelen = DEFAULT_CACHE_LENGTH);
	CBaseSplitterFile(const BYTE* buff, int len);
	virtual ~CBaseSplitterFile() {}

	bool SetCacheSize(int len = DEFAULT_CACHE_LENGTH);
	void ClearCache();

	__int64 GetPos();
	__int64 GetLength();
	__int64 GetRemaining();

	virtual void Seek(__int64 pos);

	UINT64 UExpGolombRead();
	INT64 SExpGolombRead();

	BYTE BitRead();
	UINT64 BitRead(int bits, bool peek = false);
	void BitByteAlign();
	void BitFlush();
	HRESULT ByteRead(BYTE* data, __int64 len);

	HRESULT ReadString(std::string& s, int len);
	HRESULT ReadAString(std::wstring& s, int len);
	HRESULT ReadWString(std::wstring& s, int len);

	bool NextMpegStartCode(BYTE& b, int len = 65536, BYTE id = 0);
	bool NextNALU(BYTE& id, __int64& start, int len = -1);
	bool NextTSPacket(int& tslen, int len = -1);

	#pragma pack(push, 1)

	enum MpegType {MpegUnknown, Mpeg1, Mpeg2};

	struct PSHeader
	{
		MpegType type;

		UINT64 scr;
		UINT64 bitrate;
	};

	struct PSSystemHeader
	{
		DWORD rate_bound;
		BYTE video_bound;
		BYTE audio_bound;
		bool fixed_rate;
		bool csps;
		bool sys_video_loc_flag;
		bool sys_audio_loc_flag;
	};

	struct PESHeader
	{
		WORD len;

		BYTE type:2;
		BYTE fpts:1;
		BYTE fdts:1;

		REFERENCE_TIME pts;
		REFERENCE_TIME dts;

		// mpeg1 stuff

		UINT64 std_buff_size;

		// mpeg2 stuff

		BYTE scrambling:2, priority:1, alignment:1, copyright:1, original:1;
		BYTE escr:1, esrate:1, dsmtrickmode:1, morecopyright:1, crc:1, extension:1;
		BYTE hdrlen;

		struct PESHeader() 
		{
			memset(this, 0, sizeof(*this));
		}
	};

	struct MpegSequenceHeader
	{
		WORD width;
		WORD height;
		BYTE ar:4;
		DWORD ifps;
		DWORD bitrate;
		DWORD vbv;
		BYTE constrained:1;
		BYTE fiqm:1;
		BYTE iqm[64];
		BYTE fniqm:1;
		BYTE niqm[64];

		// ext

		BYTE startcodeid:4;
		BYTE profile_levelescape:1;
		BYTE profile:3;
		BYTE level:4;
		BYTE progressive:1;
		BYTE chroma:2;
		BYTE lowdelay:1;

		// misc

		int arx, ary;
	};

	struct VC1Header
	{
		BYTE profile;
		BYTE level;
		WORD width;
		WORD height;
		WORD disp_width;
		WORD disp_height;
		BYTE arx, ary;
		struct {WORD num, denum;} fps;
	};

	struct MpegAudioHeader
	{
		WORD sync:11;
		WORD version:2;
		WORD layer:2;
		WORD crc:1;
		WORD bitrate:4;
		WORD freq:2;
		WORD padding:1;
		WORD privatebit:1;
		WORD channels:2;
		WORD modeext:2;
		WORD copyright:1;
		WORD original:1;
		WORD emphasis:2;

		__int64 start;
		int bytes;
		
		int nSamplesPerSec, FrameSize, nBytesPerSec;
		REFERENCE_TIME rtDuration;
	};

	struct LATMHeader
	{
		DWORD rate;
		BYTE channels;
	};

	struct AACHeader
	{
		WORD sync:12;
		WORD version:1;
		WORD layer:2;
		WORD fcrc:1;
		WORD profile:5;
		WORD freq:4;
		WORD privatebit:1;
		WORD channels:3;
		WORD original:1;
		WORD home:1; // ?

		WORD copyright_id_bit:1;
		WORD copyright_id_start:1;
		WORD aac_frame_length:13;
		WORD adts_buffer_fullness:11;
		WORD no_raw_data_blocks_in_frame:2;

		WORD crc;

		__int64 start;
		int bytes;

		int FrameSize, nBytesPerSec;
		REFERENCE_TIME rtDuration;
	};

	struct AC3Header
	{
		WORD sync;
		WORD crc1;
		BYTE fscod:2;
		BYTE frmsizecod:6;
		BYTE bsid:5;
		BYTE bsmod:3;
		BYTE acmod:3;
		BYTE cmixlev:2;
		BYTE surmixlev:2;
		BYTE dsurmod:2;
		BYTE lfeon:1;

		// the rest is unimportant for us
	};

	struct DTSHeader
	{
		DWORD sync;
		BYTE frametype:1;
		BYTE deficitsamplecount:5;
        BYTE fcrc:1;
		BYTE nblocks:7;
		WORD framebytes;
		BYTE amode:6;
		BYTE sfreq:4;
		BYTE rate:5;
	};

	struct LPCMHeader
	{
		BYTE emphasis:1;
		BYTE mute:1;
		BYTE reserved1:1;
		BYTE framenum:5;
		BYTE quantwordlen:2;
		BYTE freq:2; // 48, 96, 44.1, 32
		BYTE reserved2:1;
		BYTE channels:3; // +1
		BYTE drc; // 0x80: off
	};

	struct BPCMHeader
	{
		WORD unknown; // == 03 c0 (headersize:8 + something:8 ?)
		BYTE channels:4;
		BYTE freq:4;
		BYTE bps:2;
		BYTE reserved:6;
	};

	struct DVDSubpictureHeader
	{
		// nothing ;)
	};
	
	struct SVCDSubpictureHeader
	{
		// nothing ;)
	};

	struct CVDSubpictureHeader
	{
		// nothing ;)
	};

	struct PS2AudioHeader
	{
		// 'SShd' + len (0x18)
		DWORD unk1;
		DWORD freq;
		DWORD channels;
		DWORD interleave; // bytes per channel
		// padding: FF .. FF
		// 'SSbd' + len
		// pcm or adpcm data
	};

	struct PS2SubpictureHeader
	{
		// nothing ;)
	};

	struct TSHeader
	{
		BYTE sync; // 0x47

		struct
		{
			WORD error:1;
			WORD payloadstart:1;
			WORD transportpriority:1;
			WORD pid:13;
		};

		union
		{
			struct
			{
				BYTE counter:4;
				BYTE payload:1;
				BYTE adapfield:1;
				BYTE scrambling:2;
			};

			BYTE flags;
		};

		// if adapfield set
		
		BYTE length;

		union
		{
			struct
			{
				BYTE extension:1;
				BYTE privatedata:1;
				BYTE splicingpoint:1;
				BYTE fopcr:1;
				BYTE fpcr:1;
				BYTE priority:1;
				BYTE randomaccess:1;
				BYTE discontinuity:1;
			};

			BYTE adapfield_value;
		};

		// TODO: add more fields here when the flags above are set (they aren't very interesting...)

		__int64 pcr;

		int bytes;
		__int64 start, next;
	};

	struct TSSectionHeader
	{
		BYTE table_id;
		WORD section_syntax_indicator:1;
		WORD zero:1;
		WORD reserved1:2;
		WORD section_length:12;
		WORD transport_stream_id;
		BYTE reserved2:2;
		BYTE version_number:5;
		BYTE current_next_indicator:1;
		BYTE section_number;
		BYTE last_section_number;
	};

	// http://www.technotrend.de/download/av_format_v1.pdf

	struct PVAHeader
	{
		WORD sync; // 'VA'
		BYTE streamid; // 1 - video, 2 - audio
		BYTE counter;
		BYTE res1; // 0x55
		BYTE res2:3;
		BYTE fpts:1;
		BYTE postbytes:2;
		BYTE prebytes:2;
		WORD length;
		REFERENCE_TIME pts;
	};

	struct H264SPS
	{
		BYTE profile;
		BYTE level;
		// TODO
		DWORD width, height;
		DWORD sar_width, sar_height;
		BYTE video_format;
		BYTE video_full_range;
	};

	struct AVCHeader
	{
		H264SPS sps;
	};

	struct H264Header
	{
		H264SPS sps;
	};

	struct H265Header
	{
		int TODO;
	};

	struct FLVPacket
	{
		DWORD size;
		DWORD prevsize;
		DWORD timestamp;
		DWORD streamid;
		BYTE type;

		enum {Audio = 8, Video = 9};
	};

	struct FLVVideoPacket
	{
		BYTE type;
		BYTE codec;

		enum {Intra = 1, Inter = 2, Discardable = 3, GenKf = 4, Command = 5};
		enum {H263 = 2, Screen = 3, VP6 = 4, VP6A = 5, Screen2 = 6, AVC = 7};
	};

	struct FLVAudioPacket
	{
		BYTE format;
		BYTE rate;
		BYTE bits;
		BYTE stereo;

		enum 
		{
			LPCMBE, ADPCM, MP3, LPCMLE, 
			Nelly16, Nelly8, Nelly, G711ALaw, 
			G711MuLaw, Reserved, AAC, Speex, 
			Unknown1, Unknown2, MP38KHz, DeviceSpecific
		};
	};

	struct MP4Box
	{
		DWORD tag;
		__int64 size;
		__int64 next;
		GUID id; // tag == 'uuid'
	};

	struct MP4BoxEx : public MP4Box
	{
		BYTE version;
		DWORD flags;
	};

	struct FLACStreamHeader
	{
		DWORD spb_min; // samples per block
		DWORD spb_max;
		DWORD fs_min; // frame size
		DWORD fs_max;
		DWORD freq;
		DWORD channels;
		DWORD bits;
		UINT64 samples;
		BYTE md5sig[16];
	};

	struct FLACFrameHeader
	{
		DWORD fs_variable;
		DWORD samples;
		DWORD freq;
		DWORD channel_config;
		DWORD bits;
		UINT64 pos;
		__int64 start;
	};

	#pragma pack(pop)

	bool Read(PSHeader& h);
	bool Read(PSSystemHeader& h);
	bool Read(PESHeader& h, BYTE code);
	bool Read(MpegSequenceHeader& h, int len, CMediaType* pmt = NULL);
	bool Read(VC1Header& h, int len, CMediaType* pmt = NULL);
	bool Read(MpegAudioHeader& h, int len, bool fAllowV25, CMediaType* pmt = NULL);
	bool Read(LATMHeader& h, int len, CMediaType* pmt = NULL, bool* nosync = NULL);
	bool Read(AACHeader& h, int len, CMediaType* pmt = NULL);
	bool Read(AC3Header& h, int len, CMediaType* pmt = NULL);
	bool Read(DTSHeader& h, int len, CMediaType* pmt = NULL);
	bool Read(LPCMHeader& h, CMediaType* pmt = NULL);
	bool Read(BPCMHeader& h, CMediaType* pmt = NULL);
	bool Read(DVDSubpictureHeader& h, CMediaType* pmt = NULL);
	bool Read(SVCDSubpictureHeader& h, CMediaType* pmt = NULL);
	bool Read(CVDSubpictureHeader& h, CMediaType* pmt = NULL);
	bool Read(PS2AudioHeader& h, CMediaType* pmt = NULL);
	bool Read(PS2SubpictureHeader& h, CMediaType* pmt = NULL);
	bool Read(TSHeader& h, bool sync = true);
	bool Read(TSSectionHeader& h);
	bool Read(PVAHeader& h, bool sync = true);
	bool Read(AVCHeader& h, int len, CMediaType* pmt = NULL);
	bool Read(H264Header& h, int len, CMediaType* pmt = NULL);
	bool Read(H264SPS& h);
	bool Read(H265Header& h, int len, CMediaType* pmt = NULL);
	bool Read(FLVPacket& h, bool first = false);
	bool Read(FLVVideoPacket& h, int len, CMediaType* pmt = NULL);
	bool Read(FLVAudioPacket& h, int len, CMediaType* pmt = NULL);
	bool Read(MP4Box& h);
	bool Read(MP4BoxEx& h);
	bool Read(FLACStreamHeader& h);
	bool Read(FLACFrameHeader& h, const FLACStreamHeader& s);
};
