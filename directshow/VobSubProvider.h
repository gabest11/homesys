#pragma once

#include "SubProvider.h"

#define VOBSUBIDXSTR L"VobSub index file, v"
#define VOBSUBIDXVER 7

class CVobSubImage : public AlignedClass<16>
{
private:
	Vector2i m_size;
	RGBQUAD* m_buff[2];
	WORD m_offset[2];
	int m_field;
	bool m_custompal;
	char m_aligned;
	int m_tridx;
	struct {RGBQUAD* org /*[16]*/,* cus /*[4]*/;} m_palette;

	void Alloc(int w, int h);
	void Free();

	BYTE GetNibble(BYTE* buff, DWORD* offset);
	void DrawPixels(Vector2i p, int len, int color);
	void TrimSubImage();

public:
	int m_lang;
	UINT_PTR m_cookie;
	bool m_forced;
	REFERENCE_TIME m_start;
	REFERENCE_TIME m_delay;
	Vector4i m_rect;
	typedef struct {BYTE pal:4, tr:4;} SubPal;
	SubPal m_subpal[4];
	RGBQUAD* m_pixels;

	CVobSubImage();
	virtual ~CVobSubImage();

	void Invalidate() {m_lang = 0; m_cookie = 0;}
	void GetPacketInfo(BYTE* buff, int packetsize, int datasize);
	bool Decode(BYTE* buff, int packetsize, int datasize, bool custompal, int tridx, RGBQUAD* org, RGBQUAD* cus, bool trim);
};

[uuid("E7FBFB45-2D13-494F-9B3D-FFC9557D5C45")]
interface IVobSubStream : public ISubStream
{
	STDMETHOD (OpenFile) (LPCWSTR fn) PURE;
	STDMETHOD (OpenIndex) (LPCWSTR str, LPCWSTR name) PURE;
    STDMETHOD (Append) (REFERENCE_TIME start, REFERENCE_TIME stop, BYTE* data, int len) PURE;
	STDMETHOD (RemoveAll) () PURE;
};

[uuid("D7FBFB45-2D13-494F-9B3D-FFC9557D5C45")]
class CVobSubProvider : public AlignedClass<16>, public CSubProvider, public IVobSubStream
{
protected:
	std::wstring m_name;
	Vector2i m_size;
	Vector2i m_pos;
	Vector2i m_org;
	Vector2i m_scale;				// % (don't set it to unsigned because as a side effect it will mess up negative coordinates in GetDestrect())
	int m_alpha;					// %
	int m_smooth;					// 0: OFF, 1: ON, 2: OLD (means no filtering at all)
	struct {int in, out;} m_fade;	// ms
	struct {int hor, ver; bool enabled;} m_align; // 0: left/top, 1: center, 2: right/bottom
	unsigned int m_offset;			// ms
	bool m_forcedonly;
	bool m_custompal;
	int m_tridx;
	struct {RGBQUAD org[16], cus[4];} m_palette;
	CVobSubImage m_img;

	struct SubPic {REFERENCE_TIME start, stop; bool forced; std::vector<BYTE> buff;};
	struct SubStream {int id; std::wstring name; std::vector<SubPic*> sps; struct SubStream() : id(0) {}};
	SubStream m_sss[32];
	SubStream* m_ss;

	bool Open(const std::list<std::wstring>& rows, FILE* sub);

	bool GetCustomPal(RGBQUAD* pal, int& tridx);
    void SetCustomPal(RGBQUAD* pal, int tridx);
	void GetDestrect(Vector4i& r); // destrect of m_img, considering the current alignment mode
	void GetDestrect(const Vector2i& s, Vector4i& r); // this will scale it to the frame size of s
	void SetAlignment(bool align, int x, int y, int hor, int ver);

public:
	CVobSubProvider();
	virtual ~CVobSubProvider();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubProvider

	STDMETHODIMP_(void*) GetStartPosition(REFERENCE_TIME rt, float fps);
	STDMETHODIMP_(void*) GetNext(void* pos);
	STDMETHODIMP GetTime(void* pos, float fps, REFERENCE_TIME& start, REFERENCE_TIME& stop);
	STDMETHODIMP_(bool) IsAnimated(void* pos);
	STDMETHODIMP Render(SubPicDesc& desc, float fps);

	// ISubStream

	STDMETHODIMP_(int) GetStreamCount();
	STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
	STDMETHODIMP_(int) GetStream();
	STDMETHODIMP SetStream(int i);

	// IVobSubStream

	STDMETHODIMP OpenFile(LPCWSTR fn);
	STDMETHODIMP OpenIndex(LPCWSTR str, LPCWSTR name);
    STDMETHODIMP Append(REFERENCE_TIME start, REFERENCE_TIME stop, BYTE* data, int len);
	STDMETHODIMP RemoveAll();
};
