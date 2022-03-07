#include "stdafx.h"
#include "String.h"

namespace Util
{
	std::wstring Format(const wchar_t* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		wchar_t* buff = NULL;

		std::wstring str;

		int len = _vscwprintf(fmt, args) + 1;

		if(len > 0)
		{
			buff = new wchar_t[len];

			vswprintf_s(buff, len, fmt, args);

			str = std::wstring(buff, len - 1);
		}

		va_end(args);

		delete [] buff;

		return str;
	}

	std::string Format(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		char* buff = NULL;

		std::string str;

		int len = _vscprintf(fmt, args) + 1;

		if(len > 0)
		{
			buff = new char[len];

			vsprintf_s(buff, len, fmt, args);

			str = std::string(buff, len - 1);
		}

		va_end(args);

		delete [] buff;

		return str;
	}

	std::wstring TrimLeft(const std::wstring& s, LPCWSTR chrs)
	{
		std::wstring::size_type i = s.find_first_not_of(chrs);

		return i != std::wstring::npos ? s.substr(i) : std::wstring();
	}

	std::wstring TrimRight(const std::wstring& s, LPCWSTR chrs)
	{
		std::wstring::size_type i = s.find_last_not_of(chrs);

		return i != std::wstring::npos ? s.substr(0, i + 1) : s;
	}

	std::wstring Trim(const std::wstring& s, LPCWSTR chrs)
	{
		return TrimLeft(TrimRight(s, chrs), chrs);
	}

	std::string TrimLeft(const std::string& s, LPCSTR chrs)
	{
		std::string::size_type i = s.find_first_not_of(chrs);

		return i != std::string::npos ? s.substr(i) : std::string();
	}

	std::string TrimRight(const std::string& s, LPCSTR chrs)
	{
		std::string::size_type i = s.find_last_not_of(chrs);

		return i != std::string::npos ? s.substr(0, i + 1) : s;
	}

	std::string Trim(const std::string& s, LPCSTR chrs)
	{
		return TrimLeft(TrimRight(s, chrs), chrs);
	}

	std::wstring MakeUpper(const std::wstring& s)
	{
		std::wstring str = s;

		if(!str.empty()) _wcsupr(&str[0]);

		return str;
	}

	std::wstring MakeLower(const std::wstring& s)
	{
		std::wstring str = s;

		if(!str.empty()) _wcslwr(&str[0]);

		return str;
	}

	std::string MakeUpper(const std::string& s)
	{
		std::string str = s;

		if(!str.empty()) strupr(&str[0]);

		return str;
	}

	std::string MakeLower(const std::string& s)
	{
		std::string str = s;

		if(!str.empty()) strlwr(&str[0]);

		return str;
	}

	std::wstring UTF8To16(LPCSTR s)
	{
		std::wstring ret;

		int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0) - 1;

		if(n >= 0)
		{
			wchar_t* buff = new wchar_t[n + 1];

			n = MultiByteToWideChar(CP_UTF8, 0, s, -1, buff, n + 1);

			if(n > 0)
			{
				ret = std::wstring(buff);
			}

			delete [] buff;
		}

		return ret;
	}

	std::string UTF16To8(LPCWSTR s)
	{
		std::string ret;

		int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL) - 1;

		if(n >= 0)
		{
			char* buff = new char[n + 1];

			n = WideCharToMultiByte(CP_UTF8, 0, s, -1, buff, n + 1, NULL, NULL);

			if(n > 0)
			{
				ret = std::string(buff);
			}

			delete [] buff;
		}

		return ret;
	}

	DWORD CharSetToCodePage(DWORD charset)
	{
		if(charset == CP_UTF8) return CP_UTF8;
		if(charset == CP_UTF7) return CP_UTF7;

		CHARSETINFO cs = {0};
		
		TranslateCharsetInfo((DWORD*)charset, &cs, TCI_SRCCHARSET);

		return cs.ciACP;
	}

	std::string ConvertMBCS(const std::string& s, DWORD src, DWORD dst)
	{
		wchar_t* utf16 = new wchar_t[s.size() + 1];

		memset(utf16, 0, (s.size() + 1) * sizeof(wchar_t));

		char* mbcs = new char[s.size() * 6 + 1];

		memset(mbcs, 0, s.size() * 6 + 1);

		int len = MultiByteToWideChar(CharSetToCodePage(src), 0, s.c_str(), s.size(), utf16, (s.size() + 1) * sizeof(wchar_t));

		WideCharToMultiByte(CharSetToCodePage(dst), 0, utf16, len, mbcs, s.size() * 6, NULL, NULL);

		std::string res = mbcs;

		delete [] utf16;
		delete [] mbcs;

		return res;
	}

	std::wstring ConvertMBCS(const std::string& s, DWORD src)
	{
		return UTF8To16(ConvertMBCS(s, src, CP_UTF8).c_str());
	}

	#include "iso639.inl"

	std::wstring ISO6391ToLanguage(LPCWSTR code)
	{
		std::wstring s = code;

		for(int i = 0; i < sizeof(s_iso639) / sizeof(s_iso639[0]); i++)
		{
			if(_wcsicmp(s_iso639[i]._1, s.c_str()) == 0)
			{
				s = s_iso639[i].name;

				std::wstring::size_type j = s.find_first_of(';');

				if(j != std::wstring::npos)
				{
					s = s.substr(0, j);
				}

				return s;
			}
		}

		return L"";
	}

	std::wstring ISO6392ToLanguage(LPCWSTR code)
	{
		std::wstring s = code;

		for(int i = 0; i < sizeof(s_iso639) / sizeof(s_iso639[0]); i++)
		{
			if(_wcsicmp(s_iso639[i]._2, s.c_str()) == 0)
			{
				s = s_iso639[i].name;

				std::wstring::size_type j = s.find_first_of(';');

				if(j != std::wstring::npos)
				{
					s = s.substr(0, j);
				}

				return s;
			}
		}

		return L"";
	}

	std::wstring LanguageToISO6391(LPCWSTR lang)
	{
		std::wstring s = MakeLower(lang);

		for(int i = 0; i < sizeof(s_iso639) / sizeof(s_iso639[0]); i++)
		{
			std::list<std::wstring> sl;

			Explode(std::wstring(s_iso639[i].name), sl, ';');

			for(auto j = sl.begin(); j != sl.end(); j++)
			{
				if(_wcsicmp(s.c_str(), j->c_str()) == 0)
				{
					return s_iso639[i]._1;
				}
			}
		}

		return L"";
	}

	std::wstring LanguageToISO6392(LPCWSTR lang)
	{
		std::wstring s = MakeLower(lang);

		for(int i = 0; i < sizeof(s_iso639) / sizeof(s_iso639[0]); i++)
		{
			std::list<std::wstring> sl;

			Explode(std::wstring(s_iso639[i].name), sl, ';');

			for(auto j = sl.begin(); j != sl.end(); j++)
			{
				if(_wcsicmp(s.c_str(), j->c_str()) == 0)
				{
					return s_iso639[i]._2;
				}
			}
		}

		return L"";
	}

	std::wstring ISO6391To6392(LPCWSTR code)
	{
		std::wstring s = MakeLower(code);

		for(int i = 0; i < sizeof(s_iso639) / sizeof(s_iso639[0]); i++)
		{
			if(wcscmp(s_iso639[i]._1, s.c_str()) == 0)
			{
				return s_iso639[i]._2;
			}
		}

		return L"";
	}

	std::wstring ISO6392To6391(LPCWSTR code)
	{
		std::wstring s = MakeLower(code);

		for(int i = 0; i < sizeof(s_iso639) / sizeof(s_iso639[0]); i++)
		{
			if(wcscmp(s_iso639[i]._2, s.c_str()) == 0)
			{
				return s_iso639[i]._1;
			}
		}

		return L"";
	}

	void ToBin(LPCWSTR s, std::vector<BYTE>& data)
	{
		std::wstring str = MakeUpper(Trim(s));

		data.resize(str.size() / 2);

		BYTE b = 0;

		for(int i = 0; i < str.size(); i++)
		{
			wchar_t c = str[i];

			if(c >= '0' && c <= '9') 
			{
				if(!(i & 1)) b = ((char(c - '0') << 4) & 0xf0) | (b & 0x0f);
				else b = (char(c - '0') & 0x0f) | (b & 0xf0);
			}
			else if(c >= 'A' && c <= 'F')
			{
				if(!(i & 1)) b = ((char(c - 'A' + 10) << 4) & 0xf0) | (b & 0x0f);
				else b = (char(c - 'A' + 10) & 0x0f) | (b & 0xf0);
			}
			else 
			{
				break;
			}

			if(i & 1)
			{
				data[i >> 1] = b;

				b = 0;
			}
		}
	}

	std::wstring ToString(const BYTE* buff, int len)
	{
		std::wstring s;

		wchar_t hex[3];

		for(int i = 0; i < len; i++)
		{
			swprintf(hex, L"%02X", buff[i]);

			s += hex;
		}

		return s;
	}

	bool ToCLSID(LPCWSTR s, CLSID& clsid)
	{
		clsid = GUID_NULL;

		return SUCCEEDED(CLSIDFromString(s, &clsid));
	}

	bool ToString(const CLSID& clsid, std::wstring& s)
	{
		wchar_t buff[128];

		if(StringFromGUID2(clsid, buff, 127) > 0)
		{
			s = buff;

			return true;
		}

		return false;
	}

	std::wstring CombinePath(LPCWSTR dir, LPCWSTR fn)
	{
		wchar_t buff[MAX_PATH];

		PathCombine(buff, dir, fn);

		return std::wstring(buff);
	}

	std::wstring RemoveFileSpec(LPCWSTR path)
	{
		WCHAR buff[MAX_PATH];

		wcscpy_s(buff, MAX_PATH, path);

		PathRemoveFileSpec(buff);

		return std::wstring(buff);
	}

	std::wstring RemoveFileExt(LPCWSTR path)
	{
		WCHAR buff[MAX_PATH];

		wcscpy_s(buff, MAX_PATH, path);

		PathRemoveExtension(buff);

		return std::wstring(buff);
	}

	std::wstring ResolveShortcut(LPCWSTR str, bool* islnk)
	{
		if(islnk) *islnk = false;

		std::wstring path = str;

		if(LPCWSTR s = PathFindExtension(str))
		{
			if(wcsicmp(s, L".lnk") == 0)
			{
				CComPtr<IShellLink> pSL;

				pSL.CoCreateInstance(CLSID_ShellLink);
				
				if(CComQIPtr<IPersistFile> pPF = pSL)
				{
					wchar_t buff[MAX_PATH];

					if(SUCCEEDED(pPF->Load(str, STGM_READ))
					&& SUCCEEDED(pSL->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI))
					&& SUCCEEDED(pSL->GetPath(buff, MAX_PATH, NULL, 0)))
					{
						path = buff;

						if(islnk) *islnk = true;
					}
				}
			}
		}

		return path;
	}

	static const std::string base64_chars = 
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	static inline bool is_base64(unsigned char c) 
	{
		return isalnum(c) || c == '+' || c == '/';
	}

	std::string Base64Encode(const unsigned char* bytes_to_encode, unsigned int in_len) 
	{
		std::string ret;

		int i = 0;
		int j = 0;
		unsigned char char_array_3[3];
		unsigned char char_array_4[4];

		while(in_len--) 
		{
			char_array_3[i++] = *(bytes_to_encode++);

			if(i == 3)
			{
				char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
				char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
				char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
				char_array_4[3] = char_array_3[2] & 0x3f;

				for(i = 0; i < 4; i++)
				{
					ret += base64_chars[char_array_4[i]];
				}

				i = 0;
			}
		}

		if(i)
		{
			for(j = i; j < 3; j++)
			{
				char_array_3[j] = '\0';
			}

			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(j = 0; j < i + 1; j++)
			{
				ret += base64_chars[char_array_4[j]];
			}

			while(i++ < 3)
			{
				ret += '=';
			}
		}

		return ret;

	}

	std::string Base64Decode(const std::string& encoded_string)
	{
		std::string ret;

		int in_len = encoded_string.size();
		int i = 0;
		int j = 0;
		int in_ = 0;
		unsigned char char_array_4[4], char_array_3[3];

		while(in_len-- && encoded_string[in_] != '=' && is_base64(encoded_string[in_])) 
		{
			char_array_4[i++] = encoded_string[in_];
			in_++;

			if(i == 4)
			{
				for(i = 0; i < 4; i++)
				{
					char_array_4[i] = base64_chars.find(char_array_4[i]);
				}

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for(i = 0; i < 3; i++)
				{
					ret += char_array_3[i];
				}
				
				i = 0;
			}
		}

		if(i) 
		{
			for(j = i; j < 4; j++)
			{
				char_array_4[j] = 0;
			}

			for(j = 0; j < 4; j++)
			{
				char_array_4[j] = base64_chars.find(char_array_4[j]);
			}

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for(j = 0; j < i - 1; j++) 
			{
				ret += char_array_3[j];
			}
		}

		return ret;
	}

	std::wstring FormatSize(unsigned __int64 size)
	{
		LPCWSTR unit[] = {L"", L"K", L"M", L"G"};

		int i = 0;

		while(size > 1024 && i < sizeof(unit) / sizeof(unit[0]) - 1)
		{
			i++;

			size >>= 10;
		}

		return Format(L"%I64d %sB", size, unit[i]);
	}
}