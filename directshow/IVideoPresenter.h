#pragma once

#include <d3d9.h>
#include "../Util/Vector.h"

struct VideoPresenterParam
{
	CComPtr<IDirect3DDevice9> dev;
	HWND wnd;
	HANDLE flip;
	UINT msg;
};

[uuid("C9B916A6-638F-4717-AF36-8FD394C99356")]
interface IVideoPresenter : public IUnknown
{
	STDMETHOD(GetVideoTexture)(IDirect3DTexture9** ppt, Vector2i* ar, Vector4i* src) PURE;
	STDMETHOD(SetFlipEvent)(HANDLE hFlipEvent) PURE;
	STDMETHOD(GetFPS)(double& fps) PURE;
};
