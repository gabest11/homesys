#include "stdafx.h"
#include "service.h"
#include "managed_ptr.h"
#include "..\..\directshow\ddk\devioctl.h"
#include "..\..\directshow\ddk\ntddcdrm.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Diagnostics;
using namespace System::IO;
using namespace System::ServiceModel;
using namespace System::ServiceProcess;
using namespace System::Net;
using namespace System::Xml;
using namespace System::Text;

using namespace msclr;

namespace Homesys
{
	String^ Convert(const std::wstring& str)
	{
		return gcnew String(str.c_str());
	}

	std::wstring Convert(String^ str)
	{
		std::wstring s;

		if(str != nullptr)
		{
			pin_ptr<const wchar_t> ptr = PtrToStringChars(str);
			
			s = std::wstring(ptr);
		}

		return s;
	}

	Guid Convert(const GUID& guid)
	{
		return Guid(
			guid.Data1, guid.Data2, guid.Data3, 
			guid.Data4[0], guid.Data4[1], 
			guid.Data4[2], guid.Data4[3], 
			guid.Data4[4], guid.Data4[5], 
			guid.Data4[6], guid.Data4[7]);
	}

	GUID Convert(Guid% guid)
	{
		array<BYTE>^ guidData = guid.ToByteArray();
		pin_ptr<BYTE> data = &(guidData[0]);
		return *(GUID*)data;
	}

	CTime Convert(DateTime src)
	{
		if(src.Ticks == 0) return CTime(0);

		return CTime(src.Year, src.Month, src.Day, src.Hour, src.Minute, src.Second);
	}

	DateTime Convert(const CTime& src)
	{
		return DateTime(src.GetYear(), src.GetMonth(), src.GetDay(), src.GetHour(), src.GetMinute(), src.GetSecond());
	}

	CTimeSpan Convert(TimeSpan src)
	{
		return CTimeSpan((__time64_t)src.TotalSeconds);
	}

	TimeSpan Convert(const CTimeSpan& src)
	{
		return TimeSpan(10000000i64 * src.GetTotalSeconds());
	}

	void Convert(array<unsigned char>^ src, std::vector<BYTE>& dst)
	{
		dst.clear();

		if(src != nullptr && src->Length > 0)
		{
			pin_ptr<unsigned char> data = &src[0];

			dst.resize(src->Length);

			memcpy(dst.data(), data, src->Length);
		}
	}

	array<Byte>^ Convert(const std::vector<BYTE>& src)
	{
		array<unsigned char>^ dst = gcnew array<unsigned char>(src.size());

		if(!src.empty())
		{
			System::Runtime::InteropServices::Marshal::Copy((IntPtr)(void*)src.data(), dst, 0, src.size());
		}

		return dst;
	}

	static struct {LPCWSTR zone, offset, name;} TimeZones[] = 
	{
	    {L"ACDT", L"+1030", L"Australian Central Daylight"},
	    {L"ACST", L"+0930", L"Australian Central Standard"},
	    {L"ADT", L"-0300", L"(US) Atlantic Daylight"},
	    {L"AEDT", L"+1100", L"Australian East Daylight"},
	    {L"AEST", L"+1000", L"Australian East Standard"},
	    {L"AHDT", L"-0900", L""},
	    {L"AHST", L"-1000", L""},
	    {L"AST", L"-0400", L"(US) Atlantic Standard"},
	    {L"AT", L"-0200", L"Azores"},
	    {L"AWDT", L"+0900", L"Australian West Daylight"},
	    {L"AWST", L"+0800", L"Australian West Standard"},
	    {L"BAT", L"+0300", L"Bhagdad"},
	    {L"BDST", L"+0200", L"British Double Summer"},
	    {L"BET", L"-1100", L"Bering Standard"},
	    {L"BST", L"-0300", L"Brazil Standard"},
	    {L"BT", L"+0300", L"Baghdad"},
	    {L"BZT2", L"-0300", L"Brazil Zone 2"},
	    {L"CADT", L"+1030", L"Central Australian Daylight"},
	    {L"CAST", L"+0930", L"Central Australian Standard"},
	    {L"CAT", L"-1000", L"Central Alaska"},
	    {L"CCT", L"+0800", L"China Coast"},
	    {L"CDT", L"-0500", L"(US) Central Daylight"},
	    {L"CED", L"+0200", L"Central European Daylight"},
	    {L"CET", L"+0100", L"Central European"},
	    {L"CST", L"-0600", L"(US) Central Standard"},
	    {L"CENTRAL", L"-0600", L"(US) Central Standard"},
	    {L"EAST", L"+1000", L"Eastern Australian Standard"},
	    {L"EDT", L"-0400", L"(US) Eastern Daylight"},
	    {L"EED", L"+0300", L"Eastern European Daylight"},
	    {L"EET", L"+0200", L"Eastern Europe"},
	    {L"EEST", L"+0300", L"Eastern Europe Summer"},
	    {L"EST", L"-0500", L"(US) Eastern Standard"},
	    {L"EASTERN", L"-0500", L"(US) Eastern Standard"},
	    {L"FST", L"+0200", L"French Summer"},
	    {L"FWT", L"+0100", L"French Winter"},
	    {L"GMT", L"-0000", L"Greenwich Mean"},
	    {L"GST", L"+1000", L"Guam Standard"},
	    {L"HDT", L"-0900", L"Hawaii Daylight"},
	    {L"HST", L"-1000", L"Hawaii Standard"},
	    {L"IDLE", L"+1200", L"Internation Date Line East"},
	    {L"IDLW", L"-1200", L"Internation Date Line West"},
	    {L"IST", L"+0530", L"Indian Standard"},
	    {L"IT", L"+0330", L"Iran"},
	    {L"JST", L"+0900", L"Japan Standard"},
	    {L"JT", L"+0700", L"Java"},
	    {L"MDT", L"-0600", L"(US) Mountain Daylight"},
	    {L"MED", L"+0200", L"Middle European Daylight"},
	    {L"MET", L"+0100", L"Middle European"},
	    {L"MEST", L"+0200", L"Middle European Summer"},
	    {L"MEWT", L"+0100", L"Middle European Winter"},
	    {L"MST", L"-0700", L"(US) Mountain Standard"},
	    {L"MOUNTAIN", L"-0700", L"(US) Mountain Standard"},
	    {L"MT", L"+0800", L"Moluccas"},
	    {L"NDT", L"-0230", L"Newfoundland Daylight"},
	    {L"NFT", L"-0330", L"Newfoundland"},
	    {L"NT", L"-1100", L"Nome"},
	    {L"NST", L"+0630", L"North Sumatra"},
	    {L"NZ", L"+1100", L"New Zealand "},
	    {L"NZST", L"+1200", L"New Zealand Standard"},
	    {L"NZDT", L"+1300", L"New Zealand Daylight "},
	    {L"NZT", L"+1200", L"New Zealand"},
	    {L"PDT", L"-0700", L"(US) Pacific Daylight"},
	    {L"PST", L"-0800", L"(US) Pacific Standard"},
	    {L"PACIFIC", L"-0800", L"(US) Pacific Standard"},
	    {L"ROK", L"+0900", L"Republic of Korea"},
	    {L"SAD", L"+1000", L"South Australia Daylight"},
	    {L"SAST", L"+0900", L"South Australia Standard"},
	    {L"SAT", L"+0900", L"South Australia Standard"},
	    {L"SDT", L"+1000", L"South Australia Daylight"},
	    {L"SST", L"+0200", L"Swedish Summer"},
	    {L"SWT", L"+0100", L"Swedish Winter"},
	    {L"USZ3", L"+0400", L"USSR Zone 3"},
	    {L"USZ4", L"+0500", L"USSR Zone 4"},
	    {L"USZ5", L"+0600", L"USSR Zone 5"},
	    {L"USZ6", L"+0700", L"USSR Zone 6"},
	    {L"UT", L"-0000", L"Universal Coordinated"},
	    {L"UTC", L"-0000", L"Universal Coordinated"},
	    {L"UZ10", L"+1100", L"USSR Zone 10"},
	    {L"WAT", L"-0100", L"West Africa"},
	    {L"WET", L"-0000", L"West European"},
	    {L"WST", L"+0800", L"West Australian Standard"},
	    {L"YDT", L"-0800", L"Yukon Daylight"},
	    {L"YST", L"-0900", L"Yukon Standard"},
	    {L"ZP4", L"+0400", L"USSR Zone 3"},
	    {L"ZP5", L"+0500", L"USSR Zone 4"},
	    {L"ZP6", L"+0600", L"USSR Zone 5"}
	};
}