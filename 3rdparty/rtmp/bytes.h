#pragma once

namespace RTMP
{
	// define missing byte swap macros

	#ifndef __bswap_32
	#define __bswap_32(x) \
	     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |               \
	     (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
	#endif

	#ifndef __bswap_64
	#define __bswap_64(x) \
	     ((((x) & 0xff00000000000000ull) >> 56)                                   \
	      | (((x) & 0x00ff000000000000ull) >> 40)                                 \
	      | (((x) & 0x0000ff0000000000ull) >> 24)                                 \
	      | (((x) & 0x000000ff00000000ull) >> 8)                                  \
	      | (((x) & 0x00000000ff000000ull) << 8)                                  \
	      | (((x) & 0x0000000000ff0000ull) << 24)                                 \
	      | (((x) & 0x000000000000ff00ull) << 40)                                 \
	      | (((x) & 0x00000000000000ffull) << 56))
	#endif

	extern short ReadInt16(const char* data);
	extern int ReadInt24(const char* data);
	extern int ReadInt32(const char* data);
	extern int ReadInt32LE(const char* data);
	extern std::string ReadString(const char* data);
	extern bool ReadBool(const char* data);
	extern double ReadNumber(const char* data);

	extern void WriteNumber(char* data, double dVal);

	extern int EncodeInt16(char* output, short nVal);
	extern int EncodeInt24(char* output, int nVal);
	extern int EncodeInt32(char* output, int nVal);
	extern int EncodeInt32LE(char* output, int nVal);
	extern int EncodeString(char* output, const std::string& strValue);
	extern int EncodeString(char* output, const std::string& strName, const std::string& strValue);
	extern int EncodeBoolean(char* output, bool bVal);
	extern int EncodeBoolean(char* output, const std::string& strName, bool bVal);
	extern int EncodeNumber(char* output, double dVal);
	extern int EncodeNumber(char* output, const std::string& strName, double dVal);
}

