/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *      Copyright (C) 2008-2009 Andrej Stepanchuk
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
#include "amf.h"
#include "bytes.h"

AMFObjectProperty AMFObject::m_invalidProp;

AMFObjectProperty::AMFObjectProperty()
{
	Reset();
}

AMFObjectProperty::AMFObjectProperty(const std::string& strName, double dValue)
{
	Reset();
}

AMFObjectProperty::AMFObjectProperty(const std::string& strName, bool bValue)
{
	Reset();
}

AMFObjectProperty::AMFObjectProperty(const std::string& strName, const std::string& strValue)
{
	Reset();
}

AMFObjectProperty::AMFObjectProperty(const std::string& strName, const AMFObject& objValue)
{
	Reset();
}

AMFObjectProperty::~ AMFObjectProperty()
{
}

const std::string& AMFObjectProperty::GetPropName() const
{
	return m_strName;
}

void AMFObjectProperty::SetPropName(const std::string strName)
{
	m_strName = strName;
}

AMFDataType AMFObjectProperty::GetType() const
{
	return m_type;
}

double AMFObjectProperty::GetNumber() const
{
	return m_dNumVal;
}

bool AMFObjectProperty::GetBoolean() const
{
	return m_dNumVal != 0;
}

const std::string& AMFObjectProperty::GetString() const
{
	return m_strVal;
}

const AMFObject& AMFObjectProperty::GetObject() const
{
	return m_objVal;
}

bool AMFObjectProperty::IsValid() const
{
	return m_type != AMF_INVALID;
}

int AMFObjectProperty::Encode(char* pBuffer, int nSize) const
{
	int nBytes = 0;

	if(m_type == AMF_INVALID)
	{
		return -1;
	}

	if(m_type != AMF_NULL && nSize < (int)m_strName.size() + (int)sizeof(short) + 1)
	{
		return -1;
	}

	if(m_type != AMF_NULL && !m_strName.empty())
	{
		nBytes += EncodeName(pBuffer);
		pBuffer += nBytes;
		nSize -= nBytes;
	}

	int nRes;

	switch(m_type)
	{
	case AMF_NUMBER:
		if(nSize < 9) return -1;
		nBytes += RTMP::EncodeNumber(pBuffer, GetNumber());
		break;

	case AMF_BOOLEAN:
		if(nSize < 2) return -1;
		nBytes += RTMP::EncodeBoolean(pBuffer, GetBoolean());
		break;

	case AMF_STRING:
		if(nSize < (int)m_strVal.size() + (int)sizeof(short)) return -1;
		nBytes += RTMP::EncodeString(pBuffer, GetString());
		break;

	case AMF_NULL:
		if(nSize < 1) return -1;
		*pBuffer = 0x05;
		nBytes += 1;
		break;

	case AMF_OBJECT:
		nRes = m_objVal.Encode(pBuffer, nSize);
		if(nRes == -1) return -1;
		nBytes += nRes;
		break;

	default:
		return -1;
	};  

	return nBytes;
}

// TODO AMF3

#define AMF3_INTEGER_MAX 268435455
#define AMF3_INTEGER_MIN -268435456

int AMF3ReadInteger(const char* data, int* val)
{
	int i = 0;

	while(i <= 2)
	{
		// handle first 3 bytes

		if((data[i] & 0x80) == 0)
		{
			break;
		}

		// byte used

		*val <<= 7; // shift up
		*val |= data[i] & 0x7f; // add bits

		i++;
	}

	if(i > 2)
	{
		// use 4th byte, all 8bits
		
		*val <<= 8;
		*val |= data[3];

		// range check

		if(*val > AMF3_INTEGER_MAX)
		{
			*val -= 1 << 29;
		}
	}
	else
	{
		// use 7 bits of last unparsed byte (0xxxxxxx)
		
		*val <<= 7;
		*val |= data[i];
	}

	return i > 2 ? 4 : i + 1;
}

int AMF3ReadString(const char* data, char** pStr)
{
	assert(pStr != 0);

	int ref = 0;
	int len = AMF3ReadInteger(data, &ref);

	data += len;

	if((ref & 0x1) == 0)
	{
		// reference: 0xxx
		// unsigned int refIndex = ref >> 1;

		return len;
	}
	else
	{
		unsigned int nSize = ref >> 1;

		(*pStr) = new char[nSize + 1];
		memcpy(*pStr, data, nSize);
		(*pStr)[nSize] = 0;

		return len + nSize;
	}

	return len;
}

int AMFObjectProperty::AMF3Decode(const char* pBuffer, int nSize, bool bDecodeName)
{
	int nOriginalSize = nSize;

	if(nSize == 0 || !pBuffer)
	{
		return -1;
  	}

	// decode name

	if(bDecodeName)
	{
		char* name;

		int nRes = AMF3ReadString(pBuffer, &name);

		if(strlen(name) <= 0)
		{
			return nRes;
		}

		m_strName = name;
		pBuffer += nRes;
		nSize -= nRes;
	}

	// decode

	unsigned char type = *pBuffer;

	nSize--;
	pBuffer++;

	int res = 0;
	int len = 0;
	char* str = NULL;

	switch(type)
	{
		case 0x00: // AMF3_UNDEFINED
		case 0x01: // AMF3_NULL
			m_type = AMF_NULL;
			break;

		case 0x02: // AMF3_FALSE
			m_type = AMF_BOOLEAN;
			m_dNumVal = 0.0;
			break;

		case 0x03: // AMF3_TRUE
			m_type = AMF_BOOLEAN;
			m_dNumVal = 1.0;
			break;

		case 0x04: // AMF3_INTEGER
			m_type = AMF_NUMBER;
			nSize -= AMF3ReadInteger(pBuffer, &res);
			m_dNumVal = (double)res;
			break;

		case 0x0A: // AMF3_OBJECT
			m_type = AMF_OBJECT;
			res = m_objVal.AMF3Decode(pBuffer, nSize, true);
			if(res == -1) return -1;
			nSize -= res;	
			break;

		case 0x06: // AMF3_STRING
		case 0x07: // AMF3_XML_DOC
		case 0x0B: // AMF3_XML_STRING, not tested
			m_type = AMF_STRING;
			nSize -= AMF3ReadString(pBuffer, &str);
			m_strVal = str;
			delete [] str;
			break;

		case 0x05: // AMF3_NUMBER
			m_type = AMF_NUMBER;
			if(nSize < 8) return -1;
			m_dNumVal = RTMP::ReadNumber(pBuffer);
			nSize -= 8;
			break;

		case 0x08: // AMF3_DATE, not tested

			len = AMF3ReadInteger(pBuffer, &res);
			nSize -= len;
			pBuffer += len;

			if((res & 0x1) == 0)
			{
				// reference
				// unsigned int nIndex = res >> 1;
			}
			else
			{
				if(nSize < 8) return -1;
				m_type = AMF_NUMBER;	
				m_dNumVal = RTMP::ReadNumber(pBuffer);
				nSize -= 8;
			}	

			break;

		case 0x09: // AMF3_ARRAY
		case 0x0C: // AMF3_BYTE_ARRAY
		default:
			return -1;
	}

	return nOriginalSize - nSize;
}

int AMFObjectProperty::Decode(const char* pBuffer, int nSize, bool bDecodeName) 
{
	int nOriginalSize = nSize;

	if(nSize == 0 || !pBuffer)
	{
		return -1;
	}

	if(bDecodeName && nSize < 4)
	{
		// at least name (length + at least 1 byte) and 1 byte of data
		
		return -1;
	}

	if(bDecodeName)
	{
		unsigned short nNameSize = RTMP::ReadInt16(pBuffer);

		if(nNameSize > nSize - 2)
		{
			return -1;
		}

		m_strName = RTMP::ReadString(pBuffer);
		nSize -= 2 + m_strName.size();
		pBuffer += 2 + m_strName.size();
	}

	if(nSize == 0)
	{
		return -1;
	}

	nSize--;

	int len = 0;

	switch(*pBuffer)
	{
	case 0x00: // AMF_NUMBER:
		if(nSize < 8) return -1;
		m_type = AMF_NUMBER;
		m_dNumVal = RTMP::ReadNumber(pBuffer + 1);
		nSize -= 8;
		break;

	case 0x01: // AMF_BOOLEAN:
		if(nSize < 1) return -1;
		m_type = AMF_BOOLEAN;
		m_dNumVal = (double)RTMP::ReadBool(pBuffer + 1);
		nSize--;
		break;

	case 0x02: // AMF_STRING:
		len = RTMP::ReadInt16(pBuffer+1);
		if(nSize < len + 2) return -1;
		m_type = AMF_STRING;
		m_strVal = RTMP::ReadString(pBuffer + 1);
		nSize -= 2 + len;
		break;

	case 0x03: // AMF_OBJECT:
		len = m_objVal.Decode(pBuffer + 1, nSize, true);
		if(len == -1) return -1;
		m_type = AMF_OBJECT;
		nSize -= len;
		break;

	case 0x04: // AMF_MOVIE_CLIP
	case 0x07: // AMF_REFERENCE
		return -1;

	case 0x0A: // AMF_ARRAY
		len = m_objVal.DecodeArray(pBuffer + 5, nSize, RTMP::ReadInt32(pBuffer + 1), false);
		if(len == -1) return -1;
		m_type = AMF_OBJECT; 
		nSize -= 4 + len;
		break;

	case 0x08: // AMF_MIXEDARRAY
		//int nMaxIndex = RTMP::ReadInt32(pBuffer+1); // can be zero for unlimited
		// next comes the rest, mixed array has a final 0x000009 mark and names, so its an object
		len = m_objVal.Decode(pBuffer + 5, nSize, true);
		if(len == -1) return -1;
		m_type = AMF_OBJECT; 
		nSize -= 4 + len;
		break;

	case 0x05: /* AMF_NULL */
	case 0x06: /* AMF_UNDEFINED */
	case 0x0D: /* AMF_UNSUPPORTED */
		m_type = AMF_NULL;
		break;

	case 0x0B: // AMF_DATE
		if(nSize < 10) return -1;
		m_type = AMF_DATE;
		m_dNumVal = RTMP::ReadNumber(pBuffer + 1);
		m_nUTCOffset = RTMP::ReadInt16(pBuffer + 9);
		nSize -= 10;
		break;

	case 0x0C: // AMF_LONG_STRING
		len = RTMP::ReadInt32(pBuffer + 1);
		if(nSize < 4 + len) return -1;
		m_type = AMF_STRING;
		m_strVal = RTMP::ReadString(pBuffer + 1);
		nSize -= 4 + len;
		break;

	case 0x0E: // AMF_RECORDSET
	case 0x0F: // AMF_XML
	case 0x10: // AMF_CLASS_OBJECT
		return -1;

	case 0x11: // AMF_AMF3_OBJECT
		len = m_objVal.AMF3Decode(pBuffer + 1, nSize, true);
		if(len == -1) return -1;
		m_type = AMF_OBJECT;
		nSize -= len;
		break;	

	default:
		return -1;
	}

	return nOriginalSize - nSize;
}

void AMFObjectProperty::Dump() const
{
	if(m_type == AMF_INVALID)
	{
		return;
	}

	if(m_type == AMF_NULL)
	{
		return;
	}

	if(m_type == AMF_OBJECT)
	{
		m_objVal.Dump();

		return;
	}

	printf("Name: %25s, ", m_strName.empty() ? "no-name." : m_strName.c_str());

	switch(m_type)
	{
	case AMF_NUMBER:
		printf("NUMBER:\t%.2f\n", m_dNumVal);
		break;

	case AMF_BOOLEAN:
		printf("BOOLEAN:\t%s\n", m_dNumVal == 1.0 ? "TRUE" : "FALSE");
		break;

	case AMF_STRING:
		printf("STRING:\t%s\n", m_strVal.c_str());
		break;

	case AMF_DATE:
		printf("DATE:\ttimestamp: %.2f, UTC offset: %d\n", m_dNumVal, m_nUTCOffset);
		break;

	default:
		printf("INVALID TYPE 0x%02x\n", (unsigned char)m_type);
		break;
	}
}

void AMFObjectProperty::Reset()
{
	m_dNumVal = 0.0;
	m_strVal.clear();
	m_objVal.Reset();
	m_type = AMF_INVALID;
}

int AMFObjectProperty::EncodeName(char* pBuffer) const
{
	*(short*)pBuffer = htons(m_strName.size());
	memcpy(&pBuffer[2], m_strName.c_str(), m_strName.size());
	return sizeof(short) + m_strName.size();
}

// AMFObject

AMFObject::AMFObject()
{
	Reset();
}

AMFObject::~AMFObject()
{
	Reset();
}

int AMFObject::Encode(char* pBuffer, int nSize) const
{
	if(nSize < 4) return -1;

	*pBuffer = 0x03; // object

	int nOriginalSize = nSize;

	for(size_t i = 0; i < m_properties.size(); i++)
	{
		int nRes = m_properties[i].Encode(pBuffer, nSize);

		if(nRes != -1)
		{
			nSize -= nRes;
			pBuffer += nRes;
		}
	}

	if(nSize < 3) return -1; // no room for the end marker

	RTMP::EncodeInt24(pBuffer, 0x000009);

	nSize -= 3;

	return nOriginalSize - nSize;
}

int AMFObject::DecodeArray(const char* pBuffer, int nSize, int nArrayLen, bool bDecodeName)
{
	int nOriginalSize = nSize;

	bool bError = false;

	while(nArrayLen-- > 0)
	{
		AMFObjectProperty prop;

		int len = prop.Decode(pBuffer, nSize, bDecodeName);

		if(len == -1)
		{
			bError = true;
		}
		else
		{
			nSize -= len;
			pBuffer += len;

			m_properties.push_back(prop);
		}
	}

	return !bError ? nOriginalSize - nSize : -1;
}

int AMFObject::AMF3Decode(const char* pBuffer, int nSize, bool bAMFData)
{
	int nOriginalSize = nSize;

	if(bAMFData)
	{
		// ASSERT(*pBuffer == 0x0A);

		pBuffer++;
		nSize--;
	}

	int ref = 0;
    int len = AMF3ReadInteger(pBuffer, &ref);

    pBuffer += len;
    nSize -= len;

	if((ref & 1) == 0)
	{
		// object reference, 0xxx
		// unsigned int objectIndex = ref >> 1;
	}
	else // object instance
	{
		int classRef = ref >> 1;

		AMF3ClassDefinition* classDef = 0;

		if((classRef & 0x1) == 0)
		{
			// class reference
			// unsigned int classIndex = classRef >> 1;
		}
		else
		{
			int classExtRef = classRef >> 1;

			bool bExternalizable = (classExtRef & 1) != 0;
			bool bDynamic = (classExtRef & 2) != 0;

			unsigned int numMembers = classExtRef >> 2;

			// class name

			char* className = NULL;

			len = AMF3ReadString(pBuffer, &className);

			nSize -= len;
			pBuffer += len;

			//std::string str = className;

			classDef = new AMF3ClassDefinition(className, bExternalizable, bDynamic);

			delete [] className;

			for(unsigned int i = 0; i < numMembers; i++)
			{
				char* memberName = NULL;

				len = AMF3ReadString(pBuffer, &memberName);
				
				classDef->AddProperty(memberName);
				
				delete [] memberName;

				nSize -= len;
				pBuffer += len;
			}
		}

		// add as referencable object
		// ...

		if(classDef->isExternalizable())
		{
			AMFObjectProperty prop;

			int len = prop.AMF3Decode(pBuffer, nSize, false);

			if(len != -1)
			{
				nSize -= len;
				pBuffer += len;
			}

			prop.SetPropName("DEFAULT_ATTRIBUTE");

			m_properties.push_back(prop);

		}
		else
		{
			for(int i = 0; i < classDef->GetMemberCount(); i++) // non-dynamic
			{
				AMFObjectProperty prop;

				int len = prop.AMF3Decode(pBuffer, nSize, false);

				// ASSERT(nRes != -1);

				prop.SetPropName(classDef->GetProperty(i));

				//prop.Dump();

				m_properties.push_back(prop);

				pBuffer += len;
				nSize -= len;
			}

			if(classDef->isDynamic())
			{
				while(true)
				{
					AMFObjectProperty prop;

					int len = prop.AMF3Decode(pBuffer, nSize, true);

					m_properties.push_back(prop);

					pBuffer += len;
					nSize -= len;

					if(prop.GetPropName().empty())
					{
						break;
					}
				}

				// property name
				/*
				AMFObjectProperty prop;
				int res = prop.AMF3Decode(pBuffer, nSize);
				if(res != -1) m_properties.push_back(prop);
				*/
			}			
		}
	}

	/*while (nSize > 0) {
	AMFObjectProperty prop;
	int nRes = prop.AMF3Decode(pBuffer, nSize, bDecodeName);
	if(nRes == -1)
	return -1;

	nSize -= nRes;
	pBuffer += nRes;
	//if(prop.GetType() != AMF_NULL)
	m_properties.push_back(prop);
	}*/

	return nOriginalSize - nSize;
}

int AMFObject::Decode(const char*  pBuffer, int nSize, bool bDecodeName)
{
	int nOriginalSize = nSize;

	bool bError = false; // if there is an error while decoding - try to at least find the end mark 0x000009

	while(nSize >= 3)
	{
		if(RTMP::ReadInt24(pBuffer) == 0x000009)
		{
			nSize -= 3;
			bError = false;
			break;
		}

		if(bError)
		{
			nSize--;
			pBuffer++;
			continue;
		}

		AMFObjectProperty prop;

		int len = prop.Decode(pBuffer, nSize, bDecodeName);

		if(len == -1)
		{
			bError = true;
		}
		else
		{
			nSize -= len;
			pBuffer += len;

			m_properties.push_back(prop);
		}
	}

	if(!bError)
	{
		Dump();
	}

	return !bError ? nOriginalSize - nSize : -1;
}

void AMFObject::AddProperty(const AMFObjectProperty& prop)
{
	m_properties.push_back(prop);
}

int AMFObject::GetPropertyCount() const
{
	return m_properties.size();
}

const AMFObjectProperty& AMFObject::GetProperty(const std::string& strName) const
{
	for(size_t n = 0; n < m_properties.size(); n++)
	{
		if(m_properties[n].GetPropName() == strName)
		{
			return m_properties[n];
		}
	}

	return m_invalidProp;
}

const AMFObjectProperty & AMFObject::GetProperty(size_t nIndex) const
{
	return nIndex < m_properties.size() ? m_properties[nIndex] : m_invalidProp;
}

void AMFObject::Dump() const
{
	for(size_t n = 0; n < m_properties.size(); n++)
	{
		m_properties[n].Dump();
	}
}

void AMFObject::Reset()
{
	m_properties.clear();
}

bool AMFObject::FindFirstMatchingProperty(std::string name, AMFObjectProperty& p) const
{
	for(int n = 0; n < GetPropertyCount(); n++)
	{
		AMFObjectProperty prop = GetProperty(n);

		if(prop.GetPropName() == name)
		{
			p = GetProperty(n);

			return true;
		}

		if(prop.GetType() == AMF_OBJECT)
		{
			if(prop.GetObject().FindFirstMatchingProperty(name, p))
			{
				return true;
			}
		}
	}

	return false;
}


AMF3ClassDefinition::AMF3ClassDefinition(const std::string& strClassName, bool bExternalizable, bool bDynamic)
{
	m_bExternalizable = bExternalizable;
	m_bDynamic = bDynamic;
	m_strClassName = strClassName;
}

AMF3ClassDefinition::~AMF3ClassDefinition()
{
}

void AMF3ClassDefinition::AddProperty(const std::string& strPropertyName)
{
	m_properties.push_back(strPropertyName);
}

const std::string& AMF3ClassDefinition::GetProperty(size_t nIndex) const
{
	static std::string s;

	return nIndex < m_properties.size() ? m_properties[nIndex] : s;
}

int AMF3ClassDefinition::GetMemberCount() const
{
	return m_properties.size();
}
