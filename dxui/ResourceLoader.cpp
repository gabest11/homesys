#include "stdafx.h"
#include "ResourceLoader.h"

namespace DXUI
{
	bool ResourceLoader::FileGetContents(LPCWSTR path, std::vector<BYTE>& buff)
	{
		if(FILE* fp = _wfopen(path, L"rb"))
		{
			fseek(fp, 0, SEEK_END);
			buff.resize((size_t)_ftelli64(fp));
			fseek(fp, 0, SEEK_SET);
			fread(buff.data(), buff.size(), 1, fp);
			fclose(fp);

			return true;
		}

		return false;
	}
}