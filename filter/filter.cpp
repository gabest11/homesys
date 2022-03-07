// filter.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "../3rdparty/baseclasses/dllsetup.h"
#include "../DirectShow/DSMSplitter.h"
#include <initguid.h>
#include "../DirectShow/moreuuids.h"

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_FLAC_FRAMED},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
//	{&__uuidof(CFLACSplitterFilter), L"FLAC Splitter", MERIT_NORMAL, countof(sudpPins), sudpPins},
//	{&__uuidof(CFLACSourceFilter), L"FLAC Source", MERIT_NORMAL, 0, NULL},
//	{&__uuidof(CDSMMultiSourceFilter), L"DSM Multi-Source", MERIT_NORMAL, 0, NULL},
//	{&__uuidof(CTeletextFilter), L"Teletext Filter", MERIT_NORMAL, 0, NULL},
//	{&__uuidof(CTSDemuxFilter), L"TSDemux", MERIT_NORMAL, 0, NULL},
//	{&__uuidof(CSilverlightSourceFilter), L"Silverlight Source", MERIT_NORMAL, 0, NULL},
	{&__uuidof(CDSMSourceFilter), L"DSM Source", MERIT_NORMAL, 0, NULL},
};

CFactoryTemplate g_Templates[] =
{
//	{sudFilter[0].strName, sudFilter[0].clsID, DirectShow::CreateInstance<CFLACSplitterFilter>, NULL, &sudFilter[0]},
//	{sudFilter[1].strName, sudFilter[1].clsID, DirectShow::CreateInstance<CFLACSourceFilter>, NULL, &sudFilter[1]},
//	{sudFilter[2].strName, sudFilter[2].clsID, DirectShow::CreateInstance<CDSMMultiSourceFilter>, NULL, &sudFilter[2]},
//	{sudFilter[3].strName, sudFilter[3].clsID, DirectShow::CreateInstance<CTeletextFilter>, NULL, &sudFilter[3]},
//	{sudFilter[4].strName, sudFilter[4].clsID, DirectShow::CreateInstance<CTSDemuxFilter>, NULL, &sudFilter[4]},
//	{sudFilter[5].strName, sudFilter[5].clsID, DirectShow::CreateInstance<CSilverlightSourceFilter>, NULL, &sudFilter[5]},	
	{sudFilter[0].strName, sudFilter[0].clsID, DirectShow::CreateInstance<CDSMSourceFilter>, NULL, &sudFilter[0]},	
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
/*
	DirectShow::RegisterSourceFilter(
		CLSID_AsyncReader, MEDIASUBTYPE_FLAC_FRAMED, 
		L"0,4,,664C6143", NULL);
*/
/*
	std::wstring clsid;
	Util::ToString(__uuidof(CDSMMultiSourceFilter), clsid);
	DirectShow::SetRegKeyValue(L"Media Type\\Extensions", L".mdsm", L"Source Filter", clsid.c_str()); 
*/
/*
	std::wstring clsid;
	Util::ToString(__uuidof(CSilverlightSourceFilter), clsid);
	//DirectShow::SetRegKeyValue(L"Media Type\\Extensions", L".ism", L"Source Filter", clsid.c_str()); 
	DirectShow::SetRegKeyValue(L"silverlight", NULL, L"Source Filter", clsid.c_str()); 
*/
	std::wstring clsid;
	Util::ToString(__uuidof(CDSMSourceFilter), clsid);
	DirectShow::SetRegKeyValue(L"Media Type\\Extensions", L".dsm", L"Source Filter", clsid.c_str()); 

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
//	DirectShow::UnRegisterSourceFilter(MEDIASUBTYPE_FLAC_FRAMED);
//	DirectShow::DeleteRegKey(_T("Media Type\\Extensions"), _T(".mdsm"));
//	DirectShow::DeleteRegKey(_T("Media Type\\Extensions"), _T(".ism"));
	DirectShow::DeleteRegKey(_T("Media Type\\Extensions"), _T(".dsm"));

	return AMovieDllRegisterServer2(FALSE);
}