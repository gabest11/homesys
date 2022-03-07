/*  RTMPDump
 *  Copyright (C) 2008-2009 Andrej Stepanchuk
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTMPDump; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "bytes.h"

namespace RTMP
{
	short ReadInt16(const char* data)
	{
		return ntohs(*(short*)data);
	}

	int ReadInt24(const char* data)
	{
		char tmp[4] = {0};
		
		memcpy(tmp + 1, data, 3);

		return ntohl(*(int*)tmp);
	}

	int ReadInt32(const char* data)
	{
		return ntohl(*(int*)data);
	}

	int ReadInt32LE(const char* data)
	{
		return *(int*)data;
	}

	std::string ReadString(const char* data)
	{
		std::string s;

		short len = ReadInt16(data);

		if(len > 0)
		{
			s = std::string(data + sizeof(short), len);
		}

		return s;
	}

	bool ReadBool(const char* data)
	{
		return *data == 0x01;
	}

	double ReadNumber(const char* data)
	{
		unsigned __int64 in  = *(unsigned __int64*)data;
		unsigned __int64 res = __bswap_64(in);
		return *(double*)&res;
	}

	void WriteNumber(char* data, double dVal)
	{
		unsigned __int64 in  = *(unsigned __int64*)&dVal;
		unsigned __int64 res = __bswap_64(in);
		memcpy(data, &res, 8);
	}

	int EncodeInt16(char* output, short nVal)
	{
		*(short*)output = htons(nVal);

		return sizeof(short);
	}

	int EncodeInt24(char* output, int nVal)
	{
		nVal = htonl(nVal);

		memcpy(output, (char*)&nVal + 1, 3);

		return 3;
	}

	int EncodeInt32(char* output, int nVal)
	{
		*(int*)output = htonl(nVal);

		return sizeof(int);
	}

	int EncodeInt32LE(char* output, int nVal)
	{
		*(int*)output = nVal;

		return sizeof(int);
	}

	int EncodeString(char* output, const std::string& strValue)
	{
		char* buf = output;
		*buf++ = 0x02; // Datatype: String
		*(short*)buf = htons(strValue.size());
		buf += 2;
		memcpy(buf, strValue.c_str(), strValue.size());
		buf += strValue.size();
		return buf - output;
	}

	int EncodeString(char* output, const std::string& strName, const std::string &strValue)
	{
		char* buf = output;
		*(short*)buf = htons(strName.size());
		buf += 2;
		memcpy(buf, strName.c_str(), strName.size());
		buf += strName.size();
		buf += EncodeString(buf, strValue);
		return buf - output;
	}

	int EncodeBoolean(char* output, bool bVal)
	{
		char* buf = output;  
		*buf++ = 0x01; // type: Boolean
		*buf++ = bVal ? 1 : 0;
		return buf - output;
	}

	int EncodeBoolean(char* output, const std::string& strName, bool bVal)
	{
		char* buf = output;
		*(short*)buf = htons(strName.size());
		buf += 2;
		memcpy(buf, strName.c_str(), strName.size());
		buf += strName.size();
		buf += EncodeBoolean(buf, bVal);
		return buf - output;
	}

	int EncodeNumber(char* output, double dVal)
	{
		char* buf = output;  
		*buf++ = 0x00; // type: Number
		WriteNumber(buf, dVal);
		buf += 8;
		return buf - output;
	}

	int EncodeNumber(char* output, const std::string& strName, double dVal)
	{
		char* buf = output;
		*(short*)buf = htons(strName.size());
		buf += 2;
		memcpy(buf, strName.c_str(), strName.size());
		buf += strName.size();
		buf += EncodeNumber(buf, dVal);
		return buf - output;
	}
}