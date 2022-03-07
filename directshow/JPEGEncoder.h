#pragma once

class CJPEGEncoder
{
	static const int ColorComponents = 3;

	int m_w, m_h;
	BYTE* m_p;

	struct {unsigned int buff, len;} m_bit;

	bool PutBit(int b, int n);
	void Flush();
	int GetBitWidth(short q);

	void WriteSOI();
	void WriteDQT();
	void WriteSOF0();
	void WriteDHT();
	void WriteSOS();
	void WriteEOI();

protected:
	virtual bool PutByte(BYTE b) = 0;
	virtual bool PutBytes(const void* data, int len) = 0;
	virtual bool Encode(const BYTE* dib);

public:
	CJPEGEncoder();
};

class CJPEGEncoderFile : public CJPEGEncoder
{
	std::wstring m_fn;
	FILE* m_file;

protected:
	bool PutByte(BYTE b);
	bool PutBytes(const void* data, int len);

public:
	CJPEGEncoderFile(LPCWSTR fn);

	bool Encode(const BYTE* dib);
};

class CJPEGEncoderMem : public CJPEGEncoder
{
	std::vector<BYTE>* m_data;

protected:
	bool PutByte(BYTE b);
	bool PutBytes(const void* data, int len);

public:
	CJPEGEncoderMem();

	bool Encode(const BYTE* dib, std::vector<BYTE>& data);
};

