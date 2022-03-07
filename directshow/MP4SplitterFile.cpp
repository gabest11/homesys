#include "StdAfx.h"
#include "MP4SplitterFile.h"
#include "Ap4AsyncReaderStream.h"

CMP4SplitterFile::CMP4SplitterFile(IAsyncReader* pReader, HRESULT& hr) 
	: CBaseSplitterFile(pReader, hr)
	, m_file(NULL)
{
	if(FAILED(hr)) return;

	hr = Init();
}

CMP4SplitterFile::~CMP4SplitterFile()
{
	delete (AP4_File*)m_file;
}

void* /* AP4_Movie* */ CMP4SplitterFile::GetMovie()
{
	ASSERT(m_file);

	return m_file ? ((AP4_File*)m_file)->GetMovie() : NULL;
}

HRESULT CMP4SplitterFile::Init()
{
	Seek(0);

	delete (AP4_File*)m_file;

	AP4_ByteStream* stream = new AP4_AsyncReaderStream(this);

	m_file = new AP4_File(*stream);
	
	AP4_Movie* movie = ((AP4_File*)m_file)->GetMovie();

	stream->Release();

	return movie ? S_OK : E_FAIL;
}
