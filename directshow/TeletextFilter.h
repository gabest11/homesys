#pragma once

#include "../3rdparty/baseclasses/baseclasses.h"

[uuid("612297A6-E1BF-4ACB-A19E-6BF1E62F1617")]
class CTeletextFilter
	: public CBaseFilter
	, public CCritSec
{
	class CInputPin : public CBaseInputPin
	{
		struct row_t {BYTE row[43]; REFERENCE_TIME start, stop;};

		std::list<row_t*> m_rows;

		CCritSec m_csReceive;

	public:
		CInputPin(CTeletextFilter* pFilter, HRESULT* phr);
		virtual ~CInputPin();
		
		HRESULT CheckMediaType(const CMediaType* pmt);
	
		STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
		STDMETHODIMP Receive(IMediaSample* pSample);
	    STDMETHODIMP EndOfStream();
	    STDMETHODIMP BeginFlush();
	    STDMETHODIMP EndFlush();
	};

	class COutputPin : public CBaseOutputPin
	{
	public:
		COutputPin(CTeletextFilter* pFilter, HRESULT* phr);

	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* props);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}
	};

	CInputPin* m_input;
	COutputPin* m_output;

public:
	CTeletextFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CTeletextFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);
};

[uuid("92FEBFC4-664E-4465-B145-A08FB767DBD2")]
interface ITeletextRenderer : public IUnknown
{
	STDMETHOD(HasPage)() PURE;
	STDMETHOD(GetPage)(DWORD& page) PURE;
	STDMETHOD(SetPage)(DWORD page) PURE;
	STDMETHOD(EnterPage)(BYTE digit) PURE;
	STDMETHOD(JumpToLink)(BYTE index) PURE;
	STDMETHOD(GetImage)(BYTE* buff, int pitch) PURE; // image must be 520x400 32bpp
};

[uuid("4EF4A2FA-7A99-4D7B-86C3-09F8EFA1070D")]
class CTeletextRenderer
	: public CBaseRenderer
	, public CCritSec
	, public ITeletextRenderer
{
	struct row_t
	{
		BYTE color[2];
		BYTE dblheight;
		BYTE text[40];

		void Reset()
		{
			color[0] = 0;
			color[1] = 7;
			dblheight = 0;
			memset(text, 0x20, sizeof(text));
		}
	};

	struct link_t
	{
		BYTE magazine;
		BYTE page;
        WORD subpage;
	};

	struct magazine_t
	{
		BYTE page;
        WORD subpage;
        BYTE language;
		BYTE packet;

		struct
		{
			DWORD erase_page:1; 
			DWORD newsflash:1;
			DWORD subtitle:1;
			DWORD supresss_hader:1;
			DWORD update_indicator:1;
			DWORD interrupted_sequence:1;
			DWORD inhibit_display:1;
			DWORD serial:1;
		};

		row_t row[24];
		link_t link[6];
	};

	struct box_t
	{
		BYTE start;
		BYTE end;
	};
	
	bool m_has_page;
	bool m_erase_page;
	DWORD m_page;
	DWORD m_page_new;
	magazine_t* m_magazine;
	row_t m_row0;
	BYTE* m_image;
	DWORD m_clut[8];
	box_t m_box[25];
	std::list<BYTE*> m_cache;

	void AddLine(BYTE magazine, BYTE packet, BYTE* buff);
	void UpdateImageRow(BYTE packet, row_t& r, magazine_t& m);
	void SaveImage(const wchar_t* fn);

public:
	CTeletextRenderer(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CTeletextRenderer();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DoRenderSample(IMediaSample* pSample);
    HRESULT CheckMediaType(const CMediaType* pmt);
	
	// ITeletextRenderer

	STDMETHODIMP HasPage();
	STDMETHODIMP GetPage(DWORD& page);
	STDMETHODIMP SetPage(DWORD page);
	STDMETHODIMP EnterPage(BYTE digit);
	STDMETHODIMP JumpToLink(BYTE index);
	STDMETHODIMP GetImage(BYTE* buff, int pitch);
};