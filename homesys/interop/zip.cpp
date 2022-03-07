#include "stdafx.h"
#include "zip.h"

using namespace System;
using namespace System::IO;
using namespace System::Diagnostics;
using namespace ICSharpCode::SharpZipLib::Zip;
using namespace ICSharpCode::SharpZipLib::Zip::Compression;
using namespace ICSharpCode::SharpZipLib::Zip::Compression::Streams;

namespace Homesys
{
	ZipDecompressor::ZipDecompressor()
	{
	}

	bool ZipDecompressor::Decompress(LPCWSTR src, LPCWSTR dst)
	{
		try
		{
			array<Byte>^ buff = gcnew array<Byte>(2048);

			ZipInputStream^ s = gcnew ZipInputStream(File::OpenRead(gcnew String(src)));

			while(ZipEntry^ e = s->GetNextEntry())
			{
				String^ dir = Path::Combine(gcnew String(dst), Path::GetDirectoryName(e->Name));
				String^ path = Path::Combine(dir, Path::GetFileName(e->Name));

				Directory::CreateDirectory(dir);

				if(e->IsFile)
				{
					FileStream^ fs = File::Create(path);

					while(true)
					{
						int size = s->Read(buff, 0, buff->Length);

						if(size <= 0) break;
						
						fs->Write(buff, 0, size);
					}
					
					fs->Close();
				}
			}

			s->Close();

			return true;
		}
		catch(Exception^ e)
		{
			Console::WriteLine(e);
		}

		return false;
	}

	bool ZipDecompressor::Decompress(LPCWSTR src, LPCWSTR file, std::vector<BYTE>& data)
	{
		data.clear();

		try
		{
			array<Byte>^ buff = gcnew array<Byte>(2048);

			ZipInputStream^ s = gcnew ZipInputStream(File::OpenRead(gcnew String(src)));

			while(ZipEntry^ e = s->GetNextEntry())
			{
				if(String::Compare(e->Name, gcnew String(file), true) != 0)
				{
					continue;
				}

				while(true)
				{
					int size = s->Read(buff, 0, buff->Length);
					
					if(size <= 0)
					{
						break;
					}

					pin_ptr<unsigned char> ptr = &buff[0];

					int oldsize = data.size();
					
					data.resize(oldsize + size);

					memcpy(&data[oldsize], ptr, size);
				}

				break;
			}

			s->Close();
		}
		catch(Exception^ e)
		{
			Console::WriteLine(e);

			data.clear();
		}

		return !data.empty();
	}
}