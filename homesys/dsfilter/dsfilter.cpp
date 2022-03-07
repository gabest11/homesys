// dsfilter.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "../../3rdparty/baseclasses/dllsetup.h"
#include "../../DirectShow/DSMSplitter.h"
#include "../../DirectShow/DSMMultiSource.h"
#include "../../DirectShow/MP4Splitter.h"
#include "../../DirectShow/MPEG2Decoder.h"
#include "../../DirectShow/MPADecFilter.h"
#include <initguid.h>
#include "../../DirectShow/moreuuids.h"

const AMOVIESETUP_MEDIATYPE sudPinTypesInDSM[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_DirectShowMedia},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPinsDSM[] =
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesInDSM), sudPinTypesInDSM},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilterDSM[] =
{
	{&__uuidof(CDSMSplitterFilter), L"Homesys DSM Splitter", MERIT_NORMAL, countof(sudpPinsDSM), sudpPinsDSM},
	{&__uuidof(CDSMSourceFilter), L"Homesys DSM Source", MERIT_NORMAL, 0, NULL},
	{&__uuidof(CDSMMultiSourceFilter), L"Homesys DSM Multi-Source", MERIT_NORMAL, 0, NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInMP4[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MP4},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPinsMP4[] =
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesInMP4), sudPinTypesInMP4},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilterMP4[] =
{
	{&__uuidof(CMP4SplitterFilter), L"Homesys MP4 Splitter", MERIT_DO_NOT_USE, countof(sudpPinsMP4), sudpPinsMP4},
	{&__uuidof(CMP4SourceFilter), L"Homesys MP4 Source", MERIT_DO_NOT_USE, 0, NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInMPEG[] =
{
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO},
//	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
//	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOutMPEG[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_IYUV},
};

const AMOVIESETUP_PIN sudpPinsMPEG[] =
{
    {L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesInMPEG), sudPinTypesInMPEG},
    {L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesOutMPEG), sudPinTypesOutMPEG}
};

const AMOVIESETUP_FILTER sudFilterMPEG[] =
{
	{&__uuidof(CMPEG2DecoderFilter), L"Homesys Video Decoder", MERIT_UNLIKELY, countof(sudpPinsMPEG), sudpPinsMPEG},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInAudio[] =
{
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE_DOLBY_AC3},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOutAudio[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
};

const AMOVIESETUP_PIN sudpPinsAudio[] =
{
    {L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesInAudio), sudPinTypesInAudio},
    {L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesOutAudio), sudPinTypesOutAudio}
};

const AMOVIESETUP_FILTER sudFilterAudio[] =
{
	{&__uuidof(CMPADecoderFilter), L"Homesys Audio Decoder", MERIT_UNLIKELY, countof(sudpPinsAudio), sudpPinsAudio},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilterDSM[0].strName, sudFilterDSM[0].clsID, DirectShow::CreateInstance<CDSMSplitterFilter>, NULL, &sudFilterDSM[0]},
	{sudFilterDSM[1].strName, sudFilterDSM[1].clsID, DirectShow::CreateInstance<CDSMSourceFilter>, NULL, &sudFilterDSM[1]},	
	{sudFilterDSM[2].strName, sudFilterDSM[2].clsID, DirectShow::CreateInstance<CDSMMultiSourceFilter>, NULL, &sudFilterDSM[2]},	
	{sudFilterMP4[0].strName, sudFilterMP4[0].clsID, DirectShow::CreateInstance<CMP4SplitterFilter>, NULL, &sudFilterMP4[0]},
	{sudFilterMP4[1].strName, sudFilterMP4[1].clsID, DirectShow::CreateInstance<CMP4SourceFilter>, NULL, &sudFilterMP4[1]},	
	{sudFilterMPEG[0].strName, sudFilterMPEG[0].clsID, DirectShow::CreateInstance<CMPEG2DecoderFilter>, NULL, &sudFilterMPEG[0]},
	{sudFilterAudio[0].strName, sudFilterAudio[0].clsID, DirectShow::CreateInstance<CMPADecoderFilter>, NULL, &sudFilterAudio[0]},
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	// dsm

	std::wstring s;

	s = Util::Format(L"0,%d,,%%0%dI64x", DSMSW_SIZE, DSMSW_SIZE * 2);
	s = Util::Format(s.c_str(), DSMSW);

	DirectShow::RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_DirectShowMedia, s.c_str(), NULL);

	Util::ToString(__uuidof(CDSMSourceFilter), s);

	DirectShow::SetRegKeyValue(L"Media Type\\Extensions", L".dsm", L"Source Filter", s.c_str()); 

	//

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	// dsm

	DirectShow::UnRegisterSourceFilter(MEDIASUBTYPE_DirectShowMedia);

	DirectShow::DeleteRegKey(L"Media Type\\Extensions", L".dsm");

	//

	return AMovieDllRegisterServer2(FALSE);
}