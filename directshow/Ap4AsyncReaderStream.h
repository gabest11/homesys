#pragma once

#include "BaseSplitterFile.h"

#include "../3rdparty/bento4/Ap4.h"
#include "../3rdparty/bento4/Ap4File.h"
#include "../3rdparty/bento4/Ap4ByteStream.h"

class AP4_AsyncReaderStream : public AP4_ByteStream
{
	int m_refs;
	CBaseSplitterFile* m_file;

public:
	AP4_AsyncReaderStream(CBaseSplitterFile* file);
	virtual ~AP4_AsyncReaderStream();

	void AddReference();
	void Release();

	AP4_Result Read(void* buffer, AP4_Size bytesToRead, AP4_Size* bytesRead);
	AP4_Result Write(const void* buffer, AP4_Size bytesToWrite, AP4_Size* bytesWritten);
	AP4_Result Seek(AP4_Offset offset);
	AP4_Result Tell(AP4_Offset& offset);
	AP4_Result GetSize(AP4_Size& size);
};