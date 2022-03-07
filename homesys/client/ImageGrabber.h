#pragma once

#include "../../DirectShow/FGManager.h"
#include <map>
#include <string>

namespace DXUI
{
	class ImageGrabber
	{
		CComPtr<IGraphBuilder2> m_pGB;

	public:
		ImageGrabber();

		struct Bitmap 
		{
			DWORD* bits;
			int width, height;

			Bitmap() {bits = NULL; width = height = 0;}
			virtual ~Bitmap() {delete [] bits;}

			void Resize(int w, int h);
		};

		bool GetInfo(LPCWSTR path, float at, Bitmap* bm, std::map<std::wstring, std::wstring>* tags, REFERENCE_TIME* dur, int timeout = 10000);
	};
}
