#pragma once

#include "BaseSplitter.h"
// #include "Ap4AsyncReaderStream.h" // FIXME

class CMP4SplitterFile : public CBaseSplitterFile
{
	void* /* AP4_File* */ m_file;

	HRESULT Init();

public:
	CMP4SplitterFile(IAsyncReader* pReader, HRESULT& hr);
	virtual ~CMP4SplitterFile();

	void* /* AP4_Movie* */ GetMovie();
};
