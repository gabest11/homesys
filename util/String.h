#pragma once

#include <vector>
#include <list>
#include <string>

namespace Util
{
	extern std::wstring Format(const wchar_t* fmt, ...);
	extern std::string Format(const char* fmt, ...);
	extern std::wstring TrimLeft(const std::wstring& s, LPCWSTR chrs = L" \t\r\n");
	extern std::wstring TrimRight(const std::wstring& s, LPCWSTR chrs = L" \t\r\n");
	extern std::wstring Trim(const std::wstring& s, LPCWSTR chrs = L" \t\r\n");
	extern std::string TrimLeft(const std::string& s, LPCSTR chrs = " \t\r\n");
	extern std::string TrimRight(const std::string& s, LPCSTR chrs = " \t\r\n");
	extern std::string Trim(const std::string& s, LPCSTR chrs = " \t\r\n");
	extern std::wstring MakeUpper(const std::wstring& s);
	extern std::wstring MakeLower(const std::wstring& s);
	extern std::string MakeUpper(const std::string& s);
	extern std::string MakeLower(const std::string& s);
	extern std::wstring UTF8To16(LPCSTR s);
	extern std::string UTF16To8(LPCWSTR s);
	extern DWORD CharSetToCodePage(DWORD charset);
	extern std::string ConvertMBCS(const std::string& s, DWORD src, DWORD dst);
	extern std::wstring ConvertMBCS(const std::string& s, DWORD src);
	extern std::wstring ISO6391ToLanguage(LPCWSTR code);
	extern std::wstring ISO6392ToLanguage(LPCWSTR code);
	extern std::wstring LanguageToISO6391(LPCWSTR lang);
	extern std::wstring LanguageToISO6392(LPCWSTR lang);
	extern std::wstring ISO6391To6392(LPCWSTR code);
	extern std::wstring ISO6392To6391(LPCWSTR code);
	extern void ToBin(LPCWSTR s, std::vector<BYTE>& data);
	extern std::wstring ToString(const BYTE* buff, int len);
	extern bool ToCLSID(LPCWSTR s, CLSID& clsid);
	extern bool ToString(const CLSID& clsid, std::wstring& s);
	extern std::wstring CombinePath(LPCWSTR dir, LPCWSTR fn);
	extern std::wstring RemoveFileSpec(LPCWSTR path);
	extern std::wstring RemoveFileExt(LPCWSTR path);
	extern std::wstring ResolveShortcut(LPCWSTR str, bool* islnk = NULL);
	extern std::string Base64Encode(const unsigned char* bytes_to_encode, unsigned int in_len);
	extern std::string Base64Decode(const std::string& encoded_string);
	extern std::wstring FormatSize(unsigned __int64 size);

	template<class T> void Replace(T& s, typename T::const_pointer src, typename T::const_pointer dst)
	{
		int i = 0;

		int src_size = T(src).size();
		int dst_size = T(dst).size();

		while((i = s.find(src, i)) != std::string::npos) 
		{
			s.replace(i, src_size, dst); 

			i += dst_size;
		}
	}

	template<class T, class U, typename SEP> T Explode(const T& str, U& tokens, SEP sep, int limit = 0)
	{
		tokens.clear();

		for(int i = 0, j = 0; ; i = j + 1)
		{
			j = str.find_first_of(sep, i);

			if(j == T::npos || tokens.size() == limit - 1)
			{
				tokens.push_back(Trim(str.substr(i)));

				break;
			}
			else
			{
				tokens.push_back(Trim(str.substr(i, j - i)));
			}		
		}

		return tokens.front();
	}

	template<class T, typename SEP> T Implode(const std::list<T>& src, SEP sep)
	{
		T s;

		if(!src.empty())
		{
			auto i = src.begin();

			for(;;)
			{
				s += *i++;

				if(i == src.end())
				{
					break;
				}

				s += sep;
			}
		}

		return s;
	}
}