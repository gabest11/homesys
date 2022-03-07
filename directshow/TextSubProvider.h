#pragma once

#include "SubProvider.h"
#include "../subtitle/SubtitleFile.h"
#include "../subtitle/Renderer.h"

#pragma once

[uuid("D0593632-0AB7-47CA-8BE1-E9D2A6A4825E")]
interface ITextSubStream : public ISubStream
{
	STDMETHOD (OpenFile) (LPCWSTR fn, LPCWSTR name = L"") PURE;
	STDMETHOD (OpenMemory) (BYTE* buff, int len, LPCWSTR name = L"") PURE;
	STDMETHOD (OpenText) (LPCWSTR name = L"") PURE;
	STDMETHOD (OpenSSF) (LPCWSTR header, LPCWSTR name = L"") PURE;
	STDMETHOD (OpenSSA) (LPCWSTR header, LPCWSTR name = L"") PURE;
	STDMETHOD (AppendText) (REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str) PURE;
	STDMETHOD (AppendSSF) (REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str) PURE;
	STDMETHOD (AppendSSA) (REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str) PURE;
	STDMETHOD (SetFont) (LPCWSTR name, float size, bool bold, bool italic, bool underline, bool strikethrough) PURE;
	STDMETHOD (SetColor) (DWORD font, DWORD fill, DWORD background, DWORD shadow) PURE;
	STDMETHOD (SetBackgroundSize) (float size) PURE;
	STDMETHOD (SetShadowDepth) (float depth) PURE;
	STDMETHOD (SetMargin) (int alignment, int left, int top, int right, int bottom) PURE;
	STDMETHOD (SetDefaultStyle) () PURE; // applies changes
	// TODO: other default style properties (get+set)
};

[uuid("E0593632-0AB7-47CA-8BE1-E9D2A6A4825E")]
class CTextSubProvider : public CSubProvider, public ITextSubStream
{
	std::wstring m_name;
	SSF::SubtitleFile* m_file;
	SSF::Renderer* m_renderer;
	bool m_frame_based;

	static LPCWSTR m_ssf;
	static LPCWSTR m_alignment[];

	bool OpenSSF(SSF::InputStream& s, LPCWSTR name);
	bool OpenSSA(SSF::InputStream& s, LPCWSTR name);
	bool OpenSUB(SSF::InputStream& s, LPCWSTR name);
	bool OpenSRT(SSF::InputStream& s, LPCWSTR name);
	bool OpenSubViewer(SSF::InputStream& s, LPCWSTR name);
	void Close();

	// SSA helpers

	struct Style
	{
		std::wstring name;
		std::wstring safename;
		std::wstring face;
		std::wstring safeface;
		float size;
		DWORD color[4];
		bool bold, italic, underline, strikethrough;
		float scalex, scaley;
		float spacing;
		float anglex, angley, anglez;
		int borderstyle;
		float outline, shadow;
		int alignment;
		int left, top, right, bottom;
		int alpha;
		int charset;
		int relativeto;
		int wrapstyle;

		int r(int i) const {return (int)((color[i] >> 0) & 0xff);}
		int g(int i) const {return (int)((color[i] >> 8) & 0xff);}
		int b(int i) const {return (int)((color[i] >> 16) & 0xff);}
		int a(int i) const {return (int)(0xff - (color[i] >> 24));}
	};

	struct Modifier 
	{
		std::wstring name; 
		std::vector<std::wstring> params;
	};

	class ModifierConverter
	{
		std::set<std::wstring> m_modifiers;

		void Parse(LPCWSTR str, std::list<Modifier>& m);

	public:
		ModifierConverter(int charset, int wrapstyle);

		void Process(LPCWSTR str, const Style& style, float start, float stop);

		int m_charset;
		int m_wrapstyle;
		bool m_karaoke;
		std::list<std::wstring> m_style;
	};

	Vector2i m_playres;
	int m_wrapstyle;
	std::map<std::wstring, Style> m_styles;
	Style m_defstyle;

	bool ConvertSSA(SSF::InputStream& s, int ver, int index, std::wstring& ssf);

	std::wstring ConvertSSAText(const std::wstring& str, int wrapstyle);

	static std::wstring EscapeStyleName(LPCWSTR style);
	static std::wstring EscapeFaceName(LPCWSTR face);
	static DWORD HexToDWORD(LPCWSTR str);

public:
	CTextSubProvider();
	virtual ~CTextSubProvider();

//	LPCWSTR GetName() const {return m_name.c_str();}

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
	STDMETHODIMP SetStream(int iStream);

	// ITextSubStream

	STDMETHODIMP OpenFile(LPCWSTR fn, LPCWSTR name = L"");
	STDMETHODIMP OpenMemory(BYTE* buff, int len, LPCWSTR name = L"");
	STDMETHODIMP OpenText(LPCWSTR name = L"");
	STDMETHODIMP OpenSSF(LPCWSTR header, LPCWSTR name = L"");
	STDMETHODIMP OpenSSA(LPCWSTR header, LPCWSTR name = L"");
	STDMETHODIMP AppendText(REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str);
	STDMETHODIMP AppendSSF(REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str);
	STDMETHODIMP AppendSSA(REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str);
	STDMETHODIMP SetFont(LPCWSTR name, float size, bool bold, bool italic, bool underline, bool strikethrough);
	STDMETHODIMP SetColor(DWORD font, DWORD fill, DWORD background, DWORD shadow);
	STDMETHODIMP SetBackgroundSize(float size);
	STDMETHODIMP SetShadowDepth(float depth);
	STDMETHODIMP SetMargin(int alignment, int left, int top, int right, int bottom);
	STDMETHODIMP SetDefaultStyle();
};
