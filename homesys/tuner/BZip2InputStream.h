#pragma once

#include "../../3rdparty/bzip2/bzlib.h"

using namespace System;
using namespace System::IO;

namespace Homesys
{
	public ref class BZip2InputStream : public Stream
	{
		Stream^ m_base;
		array<unsigned char>^ m_input;
		bz_stream* m_bz;
		unsigned char* m_buff;

	public:
		BZip2InputStream(Stream^ base);
		~BZip2InputStream();
		!BZip2InputStream();

		// Stream

		virtual property bool CanRead
		{
			bool get() override
			{
				return true;
			}
		}

		virtual property bool CanSeek
		{
			bool get() override
			{
				return false;
			}
		}

		virtual property bool CanWrite
		{
			bool get() override
			{
				return false;
			}
		}

		virtual property __int64 Length
		{
			__int64 get() override
			{
				throw gcnew NotSupportedException();
			}
		}

		virtual property __int64 Position
		{
			__int64 get() override
			{
				throw gcnew NotSupportedException();
			}

			void set(__int64 value) override
			{
				throw gcnew NotSupportedException();
			}
		}

		virtual void Flush() override;
		virtual __int64 Seek(__int64 offset, SeekOrigin origin) override;
		virtual void SetLength(__int64 value) override;
		virtual void Write(array<unsigned char>^ buffer, int offset, int count) override;
		virtual int Read(array<unsigned char>^ buffer, int offset, int count) override;
		// TODO: virtual int ReadByte() override;
		virtual void Close() override;
	};
}