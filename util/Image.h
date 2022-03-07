#pragma once

#include <vector>

namespace Util
{
	class ImageCompressor
	{
	public:
		ImageCompressor();
		virtual ~ImageCompressor();

		bool SaveJPEG(LPCWSTR fn, int width, int height, int pitch, void* bits);
	};

	class ImageDecompressor
	{
	public:
		BYTE* m_pixels;
		int m_width, m_height, m_pitch;

	public:
		ImageDecompressor();
		virtual ~ImageDecompressor();

		bool Open(LPCWSTR fn);
		bool Open(const BYTE* data, int len);
		bool Open(const std::vector<BYTE>& data);
		void Close();
	};
}