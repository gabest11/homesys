#include "StdAfx.h"
#include "Image.h"

namespace Util
{
	static bool GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
	{
	   UINT num, size;

	   Gdiplus::GetImageEncodersSize(&num, &size);
	   Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)malloc(size);
	   Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	   bool found = false;

	   for(UINT i = 0; !found && i < num; i++)
	   {
		   if(_wcsicmp(pImageCodecInfo[i].MimeType, format) == 0)
		   {
			   *pClsid = pImageCodecInfo[i].Clsid;

			   found = true;
		   }
	   }

	   free(pImageCodecInfo);

	   return found;
	}

	ImageCompressor::ImageCompressor()
	{
	}

	ImageCompressor::~ImageCompressor()
	{
	}

	bool ImageCompressor::SaveJPEG(LPCWSTR fn, int width, int height, int pitch, void* bits)
	{
		bool ret = false;

		Gdiplus::Image* img = new Gdiplus::Bitmap(width, height, pitch, PixelFormat32bppRGB, (BYTE*)bits);

		if(img != NULL && img->GetLastStatus() == Gdiplus::Ok)
		{
			CLSID clsid = GUID_NULL;

			GetEncoderClsid(L"image/jpeg", &clsid);

			if(img->Save(fn, &clsid, NULL) == Gdiplus::Ok)
			{
				ret = true;
			}
		}

		delete img;

		return ret;
	}

	//

	ImageDecompressor::ImageDecompressor()
		: m_pixels(NULL)
		, m_width(0)
		, m_height(0)
		, m_pitch(0)
	{
	}

	ImageDecompressor::~ImageDecompressor()
	{
		Close();
	}

	bool ImageDecompressor::Open(LPCWSTR fn)
	{
		Close();

		bool ret = false;

		Gdiplus::Bitmap* bm = new Gdiplus::Bitmap(fn);

		if(bm != NULL && bm->GetLastStatus() == Gdiplus::Ok)
		{
			Gdiplus::Rect r(0, 0, bm->GetWidth(), bm->GetHeight());

			Gdiplus::BitmapData data;

			if(bm->LockBits(&r, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) == Gdiplus::Ok)
			{
				int size = data.Stride * data.Height;

				m_width = data.Width;
				m_height = data.Height;
				m_pitch = data.Stride;
				m_pixels = new BYTE[size];

				memcpy(m_pixels, data.Scan0, size);

				bm->UnlockBits(&data);

				ret = true;
			}
		}

		delete bm;

		return ret;
	}

	bool ImageDecompressor::Open(const BYTE* data, int len)
	{
		Close();

		if(data == NULL || len == 0)
		{
			return false;
		}

		bool ret = false;

		CComPtr<IStream> stream;

		HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, len);
		
		if(h != NULL)
		{
			void* p = ::GlobalLock(h);
			
			if(p != NULL)
			{
				memcpy(p, data, len);

				if(CreateStreamOnHGlobal(h, FALSE, &stream) == S_OK)
				{
					Gdiplus::Bitmap* bm = new Gdiplus::Bitmap(stream);

					if(bm != NULL && bm->GetLastStatus() == Gdiplus::Ok)
					{
						Gdiplus::Rect r(0, 0, bm->GetWidth(), bm->GetHeight());

						Gdiplus::BitmapData data;

						if(bm->LockBits(&r, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) == Gdiplus::Ok)
						{
							int size = data.Stride * data.Height;

							m_width = data.Width;
							m_height = data.Height;
							m_pitch = data.Stride;
							m_pixels = new BYTE[size];

							memcpy(m_pixels, data.Scan0, size);

							bm->UnlockBits(&data);

							ret = true;
						}
					}

					delete bm;
				}

				::GlobalUnlock(h);
			}

			::GlobalFree(h);
		}

		return ret;
	}

	bool ImageDecompressor::Open(const std::vector<BYTE>& data)
	{
		return Open(data.data(), data.size());
	}

	void ImageDecompressor::Close()
	{
		m_width = m_height = m_pitch = 0;

		if(m_pixels != NULL)
		{
			delete [] m_pixels;

			m_pixels = NULL;
		}
	}
}