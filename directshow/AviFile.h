#pragma once

#include <Aviriff.h> // conflicts with vfw.h...
#include "BaseSplitter.h"

class CAVIFile : public CBaseSplitterFile
{
	HRESULT Init();
	HRESULT Parse(DWORD parentid, __int64 end);

public:
	CAVIFile(IAsyncReader* pAsyncReader, HRESULT& hr);
	virtual ~CAVIFile();

	template<typename T> HRESULT Read(T& var, int offset = 0)
	{
		memset(&var, 0, sizeof(var));

		return ByteRead((BYTE*)&var + offset, sizeof(var) - offset);
	}

	class Stream
	{
	public:
		AVISTREAMHEADER strh;
		std::vector<BYTE> strf;
		std::wstring strn;
		AVISUPERINDEX* indx;
		struct chunk {UINT64 fKeyFrame:1, fChunkHdr:1, size:62; UINT64 filepos; DWORD orgsize;};
		std::vector<chunk> cs;
		UINT64 totalsize;

	public:
		Stream() : indx(NULL) {}
		virtual ~Stream() {delete [] indx;}

		REFERENCE_TIME GetRefTime(DWORD frame, UINT64 size);
		int GetTime(DWORD frame, UINT64 size);
		int GetFrame(REFERENCE_TIME rt);
		int GetKeyFrame(REFERENCE_TIME rt);
		DWORD GetChunkSize(DWORD size);
		bool IsRawSubtitleStream();

		// tmp
		struct chunk2 {DWORD t; DWORD n;};
		std::vector<chunk2> cs2;
	};

	AVIMAINHEADER m_avih;
	struct ODMLExtendedAVIHeader {DWORD dwTotalFrames;} m_dmlh;
	std::vector<Stream*> m_strms;
	std::map<DWORD, std::wstring> m_info;
	AVIOLDINDEX* m_idx1;
	std::list<UINT64> m_movis;
	// VideoPropHeader m_vprp;

	REFERENCE_TIME GetTotalTime();
	HRESULT BuildIndex();
	void EmptyIndex();
	bool HasIndex();
	bool IsInterleaved(bool keepinfo = false);
};

#define TRACKNUM(fcc) (10 * ((fcc & 0xff) - 0x30) + (((fcc >> 8) & 0xff) - 0x30))
#define TRACKTYPE(fcc) ((WORD)((((DWORD)fcc >> 24) & 0xff) | ((fcc >> 8) & 0xff00)))
