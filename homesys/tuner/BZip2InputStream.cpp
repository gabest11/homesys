#include "StdAfx.h"
#include "BZip2InputStream.h"

namespace Homesys
{
	BZip2InputStream::BZip2InputStream(Stream^ base)
	{
		m_base = base;

		m_input = gcnew array<unsigned char>(1024);

		m_bz = new bz_stream();
		
		m_bz->bzalloc = NULL;
		m_bz->bzfree = NULL;
		m_bz->opaque = NULL;

		BZ2_bzDecompressInit(m_bz, 0, 0);

		m_buff = new BYTE[1024];
	}

	BZip2InputStream::~BZip2InputStream()
	{
		this->!BZip2InputStream();
	}

	BZip2InputStream::!BZip2InputStream()
	{
		BZ2_bzDecompressEnd(m_bz);

		delete m_bz;

		delete [] m_buff;
	}

	void BZip2InputStream::Flush()
	{
	}

	__int64 BZip2InputStream::Seek(__int64 offset, SeekOrigin origin)
	{
		throw gcnew NotSupportedException();
	}

	void BZip2InputStream::SetLength(__int64 value)
	{
		throw gcnew NotSupportedException();
	}

	void BZip2InputStream::Write(array<unsigned char>^ buffer, int offset, int count)
	{
		throw gcnew NotSupportedException();
	}

	int BZip2InputStream::Read(array<unsigned char>^ buffer, int offset, int count)
	{
		pin_ptr<unsigned char> output = &buffer[offset];

		m_bz->next_out = (char*)output;
		m_bz->avail_out = count;
		m_bz->total_out_lo32 = 0;
		m_bz->total_out_hi32 = 0;

		while(m_bz->total_out_lo32 < count)
		{
			if(m_bz->avail_in == 0)
			{
				int n = m_base->Read(m_input, 0, m_input->Length);

				if(n == 0) break;

				pin_ptr<unsigned char> input = &m_input[0];

				memcpy(m_buff, input, n);

				m_bz->next_in = (char*)m_buff;
				m_bz->avail_in = n;
				m_bz->total_in_lo32 = 0;
				m_bz->total_in_hi32 = 0;
			}

			int res = BZ2_bzDecompress(m_bz);

			if(res != BZ_OK)
			{
				break;
			}
		}

		return m_bz->total_out_lo32;
	}

	void BZip2InputStream::Close()
	{
		if(m_base != nullptr)
		{
			m_base->Close();
		}
	}
}