// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <crtdbg.h>
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlwin.h>
#include <atltime.h>
#include <gdiplus.h>
#include <dbt.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <malloc.h>
#include <memory.h>
#include <locale.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "../../3rdparty/baseclasses/baseclasses.h"
#include "../../3rdparty/tinyxml/tinyxml.h"
#include "../../directshow/directshow.h"
#include "../../directshow/threadsafequeue.h"
#include "../../dxui/dxui.h"
#include "../../util/util.h"
#include "../interop/interop.h"

using namespace std::tr1::placeholders;

#define DEFAULT_SKIN L"DARK"
