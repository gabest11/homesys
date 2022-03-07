#pragma once

#ifdef INTEROP_EXPORTS
#define INTEROP_API __declspec(dllexport)
#else
#define INTEROP_API __declspec(dllimport)
#endif

