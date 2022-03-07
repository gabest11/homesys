#pragma once 

#include <vector>

namespace DXUI
{
	class ResourceLoader
	{
	protected:
		bool FileGetContents(LPCWSTR path, std::vector<BYTE>& buff);

	public:
		virtual bool GetResource(LPCWSTR id, std::vector<BYTE>& buff, bool priority = false, bool refresh = false) = 0;
		virtual void IgnoreResource(LPCWSTR id) = 0;
	};
}