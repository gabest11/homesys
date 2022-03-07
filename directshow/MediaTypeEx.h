#pragma once

#include "../util/Vector.h"

class CMediaTypeEx : public CMediaType
{
public:
	CMediaTypeEx();
	CMediaTypeEx(const AM_MEDIA_TYPE& mt);
	CMediaTypeEx(const CMediaType& mt) {CMediaType::operator = (mt);}
	CMediaTypeEx(IPin* pPin);

	std::wstring ToString(IPin* pPin = NULL);

	bool ExtractBIH(BITMAPINFOHEADER* bih);
	bool ExtractDim(Vector4i& dim);

	static bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih);

	void FixMediaType();

	void Dump(std::list<std::wstring>& sl);

	static std::wstring GetVideoCodecName(const GUID& subtype, DWORD biCompression);
	static std::wstring GetAudioCodecName(const GUID& subtype, WORD wFormatTag);
	static std::wstring GetSubtitleCodecName(const GUID& subtype);
};
