#pragma once

#include "api.h"
#include <string>
#include <list>

#define ID_UPNP_GET_POS 0
#define ID_UPNP_GET_DUR 1
#define ID_UPNP_GET_STATE 2
#define ID_UPNP_OPEN 3
#define ID_UPNP_STOP 4
#define ID_UPNP_PAUSE 5
#define ID_UPNP_PLAY 6
#define ID_UPNP_PREV 7
#define ID_UPNP_NEXT 8
#define ID_UPNP_SEEK 9
#define ID_UPNP_GET_VOLUME 10
#define ID_UPNP_GET_MUTE 11
#define ID_UPNP_SET_VOLUME 12
#define ID_UPNP_SET_MUTE 13

namespace Homesys
{
	class INTEROP_API MediaRendererWrapper
	{
		class MediaRendererPtr;

		MediaRendererPtr* m_mr;

	public:
		MediaRendererWrapper(HWND wnd, UINT message);
		virtual ~MediaRendererWrapper();
	};
}