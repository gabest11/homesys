// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#pragma warning(disable: 4127 4189 4201 4511 4512)

#include <tchar.h>
#include <olectl.h>
//#include <ddraw.h>
#include <strsafe.h>
#include <strmif.h>
#include <mmsystem.h>
#include <uuids.h>
#include <dvdmedia.h>
#include <vfwmsgs.h>
#include <evcode.h>
#include <vector>
#include <list>
#include <string>

#ifndef NUMELMS
   #define NUMELMS(aa) (sizeof(aa) / sizeof((aa)[0]))
#endif