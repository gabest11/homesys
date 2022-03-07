#include "StdAfx.h"
#include "TextSubProvider.h"
#include "../util/String.h"

CTextSubProvider::CTextSubProvider()
	: m_file(NULL)
	, m_renderer(NULL)
	, m_frame_based(false)
{
	m_defstyle.face = L"Arial";
	m_defstyle.size = 26;
	m_defstyle.bold = true;
	m_defstyle.italic = false;
	m_defstyle.underline = false;
	m_defstyle.strikethrough = false;
	m_defstyle.color[0] = 0x00ffffff;
	m_defstyle.color[1] = 0x0000ffff;
	m_defstyle.color[2] = 0x00000000;
	m_defstyle.color[3] = 0x80000000;
	m_defstyle.alignment = 2;
	m_defstyle.left = 40;
	m_defstyle.top = 40;
	m_defstyle.right = 40;
	m_defstyle.bottom = 40;
	m_defstyle.outline = 1.5f;
	m_defstyle.shadow = 1.5f;
}

CTextSubProvider::~CTextSubProvider()
{
	Close();
}

STDMETHODIMP CTextSubProvider::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		QI(ISubStream)
		QI(ITextSubStream)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

bool CTextSubProvider::OpenSSF(SSF::InputStream& s, LPCWSTR name)
{
	Close();

	try
	{
		m_name = name;

		m_file = new SSF::SubtitleFile();

		m_file->Parse(s);

		SetDefaultStyle();

		m_renderer = new SSF::Renderer();

		return true;
	}
	catch(SSF::Exception& e)
	{
		wprintf(L"%s\n", (LPCWSTR)e);
	}

	Close();

	return false;
}

bool CTextSubProvider::OpenSSA(SSF::InputStream& s, LPCWSTR name)
{
	bool ret = false;

	m_playres = Vector2i(0, 0);
	m_wrapstyle = 1;
	m_styles.clear();

	// [script info]

	int ver = 3;
	int sver = 3;
	int collisions = 0; // not used (automatic)
	bool scaledbas = false;
	
	// [events]

	std::list<std::wstring> dialogs;

	//

	std::wstring row;

	int c;

	while((c = s.ReadString(row, ':')) != 0)
	{
		std::wstring key = Util::MakeLower(Util::Trim(row));

		if(!key.empty() && key[0] == ';')
		{
			if(c == ':') 
			{
				s.ReadString(row); // skip the rest
			}
		}
		else if(c == ':')
		{
			if(key == L"dialogue")
			{
				if(ConvertSSA(s, ver, dialogs.size(), row))
				{
					dialogs.push_back(row);
				}
			}
			else
			{
				if(s.ReadString(row))
				{
					row = Util::Trim(row);

					if(key == L"scripttype")
					{
						if(wcsicmp(row.c_str(), L"v4.00") == 0) ver = sver = 4;
						else if(wcsicmp(row.c_str(), L"v4.00+") == 0) ver = sver = 5;
						else if(wcsicmp(row.c_str(), L"v4.00++") == 0) ver = sver = 6;
					}
					else if(key == L"playresx")
					{
						m_playres.x = _wtoi(row.c_str());
					}
					else if(key == L"playresy")
					{
						m_playres.y = _wtoi(row.c_str());
					}
					else if(key == L"wrapstyle")
					{
						m_wrapstyle = _wtoi(row.c_str());
					}
					else if(key == L"collisions")
					{
						collisions = Util::MakeLower(row).find(L"reverese") != std::wstring::npos ? 1 : 0;
					}
					else if(key == L"scaledborderandshadow")
					{
						scaledbas =  Util::MakeLower(row).find(L"yes") != std::wstring::npos;
					}
					else if(key == L"style")
					{
						std::list<std::wstring> sl;

						Util::Explode(row, sl, ',');

						if(sver == 3 && sl.size() == 17
						|| sver == 4 && sl.size() == 18
						|| sver == 5 && sl.size() == 23
						|| sver == 6 && sl.size() == 25)
						{
							Style style;

							style.name = Util::TrimLeft(sl.front(), L"*"); sl.pop_front();
							style.safename = EscapeStyleName(style.name.c_str());
							style.face = sl.front(); sl.pop_front();
							style.safeface = EscapeFaceName(style.face.c_str());
							style.size = (float)_wtof(sl.front().c_str()); sl.pop_front();
							
							for(int i = 0; i < 4; i++)
							{
								std::wstring c = Util::MakeLower(sl.front());
								style.color[i] = c.find(L"&h") == 0 || c.find(L"0x") == 0 ? wcstoul(c.c_str() + 2, NULL, 16) : _wtoi(c.c_str());
								sl.pop_front();
							}

							style.bold = _wtoi(sl.front().c_str()) != 0; sl.pop_front();
							style.italic = _wtoi(sl.front().c_str()) != 0; sl.pop_front();

							if(sver >= 5)
							{
								style.underline = _wtoi(sl.front().c_str()) != 0; sl.pop_front();
								style.strikethrough = _wtoi(sl.front().c_str()) != 0; sl.pop_front();
								style.scalex = (float)_wtof(sl.front().c_str()) / 100; sl.pop_front();
								style.scaley = (float)_wtof(sl.front().c_str()) / 100; sl.pop_front();
								style.spacing = (float)_wtof(sl.front().c_str()); sl.pop_front();
								style.anglex = style.angley = 0;
								style.anglez = (float)_wtof(sl.front().c_str()); sl.pop_front();
							}
							else
							{
								style.underline = style.strikethrough = false;
								style.scalex = style.scaley = 1.0f;
								style.spacing = 0;
								style.anglex = style.angley = style.anglez = 0;
							}

							if(sver >= 4)
							{
								style.borderstyle = _wtoi(sl.front().c_str()); sl.pop_front();
							}
							else
							{
								style.borderstyle = 0;
							}

							style.outline = (float)_wtof(sl.front().c_str()); sl.pop_front();
							style.shadow = (float)_wtof(sl.front().c_str()); sl.pop_front();
							style.alignment = _wtoi(sl.front().c_str()); sl.pop_front();
							style.left = _wtoi(sl.front().c_str()); sl.pop_front();
							style.right = _wtoi(sl.front().c_str()); sl.pop_front();
							style.top = _wtoi(sl.front().c_str()); sl.pop_front();
							style.bottom = style.top;
							
							if(sver >= 6)
							{
								style.bottom = _wtoi(sl.front().c_str()); sl.pop_front();
							}

							if(sver <= 4)
							{
								style.alpha = _wtoi(sl.front().c_str()); sl.pop_front();
							}

							style.charset = _wtoi(sl.front().c_str()); sl.pop_front();

							if(sver >= 6)
							{
								style.relativeto = _wtoi(sl.front().c_str()); sl.pop_front();
							}
							else 
							{
								style.relativeto = 0;
							}

							if(sver <= 4)
							{
								style.color[2] = style.color[3];

								style.alpha = std::min<int>(std::max<int>(style.alpha, 0), 255);

								for(int i = 0; i < 3; i++) 
								{
									style.color[i] = (style.color[i] & 0xffffff) | (style.alpha << 24);
								}

								style.color[3] = (style.color[3] & 0xffffff) | 0x80000000;
							}
							
							if(sver >= 5)
							{
								style.scalex = std::max<float>(style.scalex, 0);
								style.scaley = std::max<float>(style.scaley, 0);
								style.spacing = std::max<float>(style.spacing, 0);
							}

							style.borderstyle = style.borderstyle == 3 ? 1 : 0;

							style.outline = std::max<float>(style.outline, 0);
							style.shadow = std::max<float>(style.shadow, 0);

							if(sver <= 4)
							{
								int n = style.alignment & 3;

								if(style.alignment & 4) n += 6; // top
								else if(style.alignment & 8) n += 3; // middle

								style.alignment = n;
							}

							if(style.alignment < 1 || style.alignment > 9)
							{
								style.alignment = 2;
							}

							style.wrapstyle = m_wrapstyle;

							m_styles[style.name] = style;
						}
					}
					else if(key == L"fontname")
					{
						// TODO
					}
				}
			}
		}
		else
		{
			if(key == L"[script info]")
			{
				ret = true;
			}
			else if(key == L"[events]")
			{
				ret = true;
			}
			else if(key == L"[v4 styles]")
			{
				ret = true;
				sver = 4;
			}
			else if(key == L"[v4+ styles]")
			{
				ret = true;
				sver = 5;
			}
			else if(key == L"[v4++ styles]")
			{
				ret = true;
				sver = 6;
			}
		}
	}

	if(!ret)
	{
		return false;
	}

	if(m_playres.x <= 0 && m_playres.y <= 0) m_playres = Vector2i(384, 288);
	else if(m_playres.x <= 0) m_playres.x = m_playres.y == 1024 ? 1280 : m_playres.y * 4 / 3;
	else if(m_playres.y <= 0) m_playres.y = m_playres.x == 1280 ? 1024 : m_playres.y * 3 / 4;

	std::wstring ssf = Util::Format(
		L"file#file {format: 'ssf'; version: 1;};\n"
		L"#k {time {id: 10000; scale: 0.01; start: +0s;}; transition: 'start';};\n" // transition: 'stop'?
		L"#K {time {id: 10000; scale: 0.01; start: +0s;}; fill.width: 1;};\n"
		L"#opaque {color.a: 255;};\n"
		L"#transp {color.a: 0;};\n"
		L"subtitle#subtitle {\n"
		L" frame {reference: 'window'; resolution: {cx: %d; cy: %d;};};\n"
		L" wrap: '%s';\n"
		L" style {\n"
		L"  background.scaled: '%s';\n"
		L"  shadow.scaled: '%s';\n"
		L" };\n"
		L"};\n",
		m_playres.x, m_playres.y,
		m_wrapstyle == 1 ? L"normal" : m_wrapstyle == 2 ? L"manual" : L"even", 
		scaledbas ? L"true" : L"false",
		scaledbas ? L"true" : L"false");

	for(auto i = m_styles.begin(); i != m_styles.end(); i++)
	{
		Style& s = i->second;

		ssf += Util::Format(
			L"style#%s {\n"
			L" font {\n"
			L"  face: '%s';\n"
			L"  size: %f;\n"
			L"  color {r: %d; g: %d; b: %d; a: %d;};\n"
			L"  weight: '%s';\n"
			L"  italic: '%s';\n"
			L"  underline: '%s';\n"
			L"  strikethrough: '%s';\n"
			L"  scale {cx: %f; cy: %f;};\n"
			L"  spacing: %f;\n"
			L" };\n"
			L" placement {\n"
			L"  margin: {l: %d; t: %d; r: %d; b: %d;};\n"
			L"  align: %s;\n"
			L"  angle {x: %f; y: %f; z: %f;};\n"
			L" };\n"
			L" background {\n"
			L"  type: '%s';\n"
			L"  color {r: %d; g: %d; b: %d; a: %d;};\n"
			L"  size: %f;\n"
			L" };\n"
			L" shadow {\n"
			L"  color {r: %d; g: %d; b: %d; a: %d;};\n"
			L"  depth: %f;\n"
			L" };\n"
			L" fill {\n"
			L"  color {r: %d; g: %d; b: %d; a: %d;};\n"
			L" };\n"
			L"};\n",
			s.safename.c_str(),
			s.safeface.c_str(),
			s.size,
			s.r(0), s.g(0), s.b(0), s.a(0),
			s.bold ? L"bold" : L"normal",
			s.italic ? L"true" : L"false",
			s.underline ? L"true" : L"false",
			s.strikethrough ? L"true" : L"false",
			s.scalex, s.scaley, 
			s.spacing,
			s.left, s.top, s.right, s.bottom,
			m_alignment[s.alignment - 1],
			s.anglex, s.angley, s.anglez, 
			s.borderstyle ? L"box" : L"outline",
			s.r(2), s.g(2), s.b(2), s.a(2),
			s.outline,
			s.r(3), s.g(3), s.b(3), s.a(3),
			s.shadow,
			s.r(1), s.g(1), s.b(1), s.a(1));
	}

	for(auto i = dialogs.begin(); i != dialogs.end(); i++)
	{
		ssf += *i;
	}

	return OpenSSF(SSF::WCharInputStream(ssf.c_str()), name);
}

bool CTextSubProvider::OpenSUB(SSF::InputStream& s, LPCWSTR name)
{
	// TODO: style and style modifiers

	std::wstring ssf = m_ssf;

	struct SubEntry {int start, stop; std::wstring text;};

	std::list<SubEntry> subs;

	std::wregex rx(L"\\{([0-9]+)\\} *\\{([0-9]*)\\}(.+)");

	std::wstring row;

	while(s.ReadString(row))
	{
		row = Util::Trim(row);

		std::wsmatch res;
		
		if(std::regex_match(row, res, rx))
		{
			SubEntry s;

			s.start = _wtoi(res[1].str().c_str());
			s.stop = _wtoi(res[2].str().c_str());
			s.text = res[3];

			subs.push_back(s);
		}
	}

	if(subs.empty())
	{
		return false;
	}

	m_frame_based = true;

	float fps = 0;

	for(auto i = subs.begin(); i != subs.end(); )
	{
		SubEntry& s = *i++;

		if(s.start == 1 && s.stop == 1)
		{
			fps = (float)_wtof(s.text.c_str());

			if(fps != 0)
			{
				m_frame_based = false;

				continue;
			}
		}
		
		if(s.start > 0 && s.stop == 0 && i != subs.end())
		{
			s.stop = i->start;
		}

		s.text = SSF::SubtitleFile::Escape(s.text.c_str());

		Util::Replace(s.text, L"|", L"\n");

		if(m_frame_based)
		{
			ssf += Util::Format(
				L"subtitle { time {start: %d; stop: %d;}; @ {%s}; };\n",
				s.start, s.stop, s.text.c_str());
		}
		else
		{
			int t = (int)(1000.0f * s.start / fps);

			int ms1 = t % 1000; t /= 1000;
			int ss1 = t % 60; t /= 60;
			int mm1 = t % 60; t /= 60;
			int hh1 = t; 

			t = (int)(1000.0f * s.stop / fps);

			int ms2 = t % 1000; t /= 1000;
			int ss2 = t % 60; t /= 60;
			int mm2 = t % 60; t /= 60;
			int hh2 = t; 

			ssf += Util::Format(
				L"subtitle {\n"
				L"time {start: %02d:%02d:%02d.%03d; stop: %02d:%02d:%02d.%03d;};\n"
				L"@ {%s};\n"
				L"};\n",
				hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2,
				s.text.c_str());
		}
	}

	return OpenSSF(SSF::WCharInputStream(ssf.c_str()), name);
}

bool CTextSubProvider::OpenSRT(SSF::InputStream& s, LPCWSTR name)
{
	std::wstring ssf = m_ssf;

	std::wregex rx1(L"<(b|i|u|s)\\b[^>]*>", std::regex::ECMAScript | std::regex::icase);
	std::wregex rx2(L"<\\\\?/[bius]>", std::regex::ECMAScript | std::regex::icase);

	bool valid = false;

	std::wstring row;

	for(int line = 0; s.ReadString(row); line++)
	{
		row = Util::Trim(row);

		wchar_t c;

		int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;

		int n = swscanf(row.c_str(), L"%d%c%d%c%d%c%d --> %d%c%d%c%d%c%d", 
			&hh1, &c, &mm1, &c, &ss1, &c, &ms1,
			&hh2, &c, &mm2, &c, &ss2, &c, &ms2);

		if(n == 1)
		{
			// num = hh1;
		}
		else if(n == 14)
		{
			std::wstring text;

			bool emptyrow = false;

			while(s.ReadString(row))
			{
				row = Util::Trim(row);

				if(row.empty()) 
				{
					emptyrow = true;
				}

				int num;

				if(emptyrow && swscanf(row.c_str(), L"%d%c", &num, &c) == 1)
				{
					break;
				}

				text += row + L"\n";
			}

			text = SSF::SubtitleFile::Escape(text.c_str());

			text = std::regex_replace(text, rx1, std::wstring(L"[$1] {"));
			text = std::regex_replace(text, rx2, std::wstring(L"}"));

			ssf += Util::Format(
				L"subtitle {\n"
				L"time {start: %02d:%02d:%02d.%03d; stop: %02d:%02d:%02d.%03d;};\n"
				L"@ {%s};\n"
				L"};\n",
				hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2,
				text.c_str());

			if(!text.empty()) 
			{
				valid = true;
			}
		}
		else
		{
			if(!valid && line > 100)
			{
				break;
			}
		}
	}

	if(!valid)
	{
		return false;
	}

	return OpenSSF(SSF::WCharInputStream(ssf.c_str()), name);
}

bool CTextSubProvider::OpenSubViewer(SSF::InputStream& s, LPCWSTR name)
{
	std::wstring ssf = m_ssf;

	bool valid = false;

	std::wstring row;

	for(int line = 0; s.ReadString(row); line++)
	{
		row = Util::Trim(row);

		if(row[0] == '[') continue;

		wchar_t c;

		int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;

		int n = swscanf(row.c_str(), L"%d%c%d%c%d%c%d,%d%c%d%c%d%c%d", 
			&hh1, &c, &mm1, &c, &ss1, &c, &ms1,
			&hh2, &c, &mm2, &c, &ss2, &c, &ms2);

		if(n == 14)
		{
			std::wstring text;

			if(s.ReadString(text))
			{
				text = Util::Trim(text);
				
				Util::Replace(text, L"[br]", L"\n");
			}

			text = SSF::SubtitleFile::Escape(text.c_str());

			ssf += Util::Format(
				L"subtitle {\n"
				L"time {start: %02d:%02d:%02d.%03d; stop: %02d:%02d:%02d.%03d;};\n"
				L"@ {%s};\n"
				L"};\n",
				hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2,
				text.c_str());

			if(!text.empty()) 
			{
				valid = true;
			}
		}
		else
		{
			if(!valid && line > 100)
			{
				break;
			}
		}
	}

	if(!valid)
	{
		return false;
	}

	return OpenSSF(SSF::WCharInputStream(ssf.c_str()), name);
}

void CTextSubProvider::Close()
{
	m_name.clear();

	delete m_file;
	delete m_renderer;

	m_file = NULL;
	m_renderer = NULL;
}

// ISubProvider

STDMETHODIMP_(void*) CTextSubProvider::GetStartPosition(REFERENCE_TIME rt, float fps)
{
	CAutoLock cAutoLock(this);

	int i;

	float t = (float)rt / 10000000;

	if(m_frame_based)
	{
		t *= fps;
	}

	return m_file->m_segments.Lookup(t, i) ? (void*)(++i) : NULL;
}

STDMETHODIMP_(void*) CTextSubProvider::GetNext(void* pos)
{
	CAutoLock cAutoLock(this);

	int i = (int)pos;

	return m_file->m_segments.GetSegment(i) ? (void*)(++i) : NULL;
}

STDMETHODIMP CTextSubProvider::GetTime(void* pos, float fps, REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	CAutoLock cAutoLock(this);

	const SSF::SubtitleFile::Segment* s = m_file->m_segments.GetSegment((int)pos - 1);

	if(s != NULL)
	{
		float scale = 10000000;

		if(m_frame_based)
		{
			scale /= fps;
		}

		start = (REFERENCE_TIME)(s->m_start * scale);
		stop = (REFERENCE_TIME)(s->m_stop * scale);

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP_(bool) CTextSubProvider::IsAnimated(void* pos)
{
	CAutoLock cAutoLock(this);

	bool animated = false;

	const SSF::SubtitleFile::Segment* s = m_file->m_segments.GetSegment((int)pos - 1);

	if(s != NULL)
	{
		std::list<SSF::Subtitle*> subs;

		m_file->Lookup(s->m_start, subs);

		for(auto i = subs.begin(); i != subs.end(); i++)
		{
			if((*i)->m_animated)
			{
				animated = true;
			}

			delete *i;
		}
	}

	return animated;
}

STDMETHODIMP CTextSubProvider::Render(SubPicDesc& desc, float fps)
{
	CheckPointer(m_renderer, E_UNEXPECTED);	

	float t = (float)(desc.start + desc.stop) / 20000000;

	if(m_frame_based)
	{
		t *= fps;
	}

	SSF::Bitmap bm;

	bm.w = desc.size.x;
	bm.h = desc.size.y;
	bm.pitch = desc.pitch;
	bm.bits = desc.bits;

	CAutoLock cAutoLock(this);

	desc.bbox = Vector4i::zero();

	std::list<SSF::Subtitle*> subs;

	m_file->Lookup(t, subs);

	m_renderer->NextSegment(subs);

	for(auto i = subs.begin(); i != subs.end(); i++)
	{
		const SSF::RenderedSubtitle* rs = m_renderer->Lookup(*i, Vector2i(bm.w, bm.h), desc.rect);

		if(rs != NULL)
		{
			desc.bbox = desc.bbox.runion(rs->Draw(bm));
		}

		delete *i;
	}

	desc.bbox = desc.bbox.rintersect(Vector4i(0, 0, bm.w, bm.h));

	return S_OK;
}

// ISubStream

STDMETHODIMP_(int) CTextSubProvider::GetStreamCount()
{
	return 1;
}

STDMETHODIMP CTextSubProvider::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
	CAutoLock cAutoLock(this);

	if(iStream != 0)
	{
		return E_INVALIDARG;
	}

	if(ppName)
	{
		*ppName = (WCHAR*)CoTaskMemAlloc((m_name.size() + 1) * sizeof(WCHAR));

		if(*ppName == NULL) return E_OUTOFMEMORY;

		wcscpy(*ppName, m_name.c_str());
	}

	if(pLCID)
	{
		*pLCID = 0; // TODO
	}

	return S_OK;
}

STDMETHODIMP_(int) CTextSubProvider::GetStream()
{
	return 0;
}

STDMETHODIMP CTextSubProvider::SetStream(int iStream)
{
	return iStream == 0 ? S_OK : E_FAIL;
}

// ITextSubStream

STDMETHODIMP CTextSubProvider::OpenFile(LPCWSTR fn, LPCWSTR name)
{
	CAutoLock cAutoLock(this);

	std::wstring n = name;

	if(n.empty())
	{
		n = fn;

		Util::Replace(n, L"\\", L"/");

		n = n.substr(0, n.find_last_of('.'));
		n = n.substr(n.find_last_of('/') + 1);
		n = n.substr(n.find_last_of('.') + 1);
	}

	try
	{
		m_frame_based = false;

		if(OpenSSF(SSF::FileInputStream(fn), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSSA(SSF::FileInputStream(fn, false), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSUB(SSF::FileInputStream(fn, false), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSRT(SSF::FileInputStream(fn, false), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSubViewer(SSF::FileInputStream(fn, false), n.c_str())) return S_OK;
	}
	catch(SSF::Exception& e)
	{
		wprintf(L"%s\n", (LPCWSTR)e);
	}

	return E_FAIL;
}

STDMETHODIMP CTextSubProvider::OpenMemory(BYTE* buff, int len, LPCWSTR name)
{
	CAutoLock cAutoLock(this);

	std::wstring n = name;

	try
	{
		m_frame_based = false;

		if(OpenSSF(SSF::MemoryInputStream(buff, len, false, false, false), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSSA(SSF::MemoryInputStream(buff, len, false, false, false), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSUB(SSF::MemoryInputStream(buff, len, false, false, false), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSRT(SSF::MemoryInputStream(buff, len, false, false, false), n.c_str())) return S_OK;

		m_frame_based = false;

		if(OpenSubViewer(SSF::MemoryInputStream(buff, len, false, false, false), n.c_str())) return S_OK;
	}
	catch(SSF::Exception& e)
	{
		wprintf(L"%s\n", (LPCWSTR)e);
	}

	return E_FAIL;
}

STDMETHODIMP CTextSubProvider::OpenText(LPCWSTR name)
{
	CAutoLock cAutoLock(this);

	return OpenSSF(SSF::WCharInputStream(m_ssf), name) ? S_OK : E_FAIL;
}

STDMETHODIMP CTextSubProvider::OpenSSF(LPCWSTR header, LPCWSTR name)
{
	CAutoLock cAutoLock(this);

	return OpenSSF(SSF::WCharInputStream(header), name) ? S_OK : E_FAIL;
}

STDMETHODIMP CTextSubProvider::OpenSSA(LPCWSTR header, LPCWSTR name)
{
	CAutoLock cAutoLock(this);

	return OpenSSA(SSF::WCharInputStream(header), name) ? S_OK : E_FAIL;
}

STDMETHODIMP CTextSubProvider::AppendText(REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str)
{
	CAutoLock cAutoLock(this);

	std::wregex rx1(L"<(b|i|u|s)[^>]*>", std::regex::ECMAScript | std::regex::icase);
	std::wregex rx2(L"<\\\\?/[bius]>", std::regex::ECMAScript | std::regex::icase);

	std::wstring text = SSF::SubtitleFile::Escape(str);

	text = std::regex_replace(text, rx1, std::wstring(L"[$1] {"));
	text = std::regex_replace(text, rx2, std::wstring(L"}"));

	std::wstring s = L"subtitle { @ { " + text + L" }; };";

	return AppendSSF(start, stop, s.c_str());
}

STDMETHODIMP CTextSubProvider::AppendSSF(REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str)
{
	CAutoLock cAutoLock(this);

	try
	{
		SSF::WCharInputStream stream(str);

		m_file->Append(stream, (float)start / 10000000, (float)stop / 10000000, true);

		return S_OK;
	}
	catch(SSF::Exception& e)
	{
		wprintf(L"%s\n", (LPCWSTR)e);
	}

	return E_FAIL;
}

STDMETHODIMP CTextSubProvider::AppendSSA(REFERENCE_TIME start, REFERENCE_TIME stop, LPCWSTR str)
{
	CAutoLock cAutoLock(this);

	int t;
	
	t = (int)(start / 100000);

	int ms0 = t % 100; t /= 100;
	int ss0 = t % 60; t /= 60;
	int mm0 = t % 60; t /= 60;
	int hh0 = t; 

	t = (int)(stop / 100000);

	int ms1 = t % 100; t /= 100;
	int ss1 = t % 60; t /= 60;
	int mm1 = t % 60; t /= 60;
	int hh1 = t; 

	std::list<std::wstring> sl;

	Util::Explode(std::wstring(str), sl, L",", 10);

	if(sl.size() < 10)
	{
		return E_FAIL;
	}

	int index = _wtoi(sl.front().c_str()); sl.pop_front();

	auto i = ++sl.begin();

	sl.insert(i, Util::Format(L"%d:%02d:%02d.%02d", hh0, mm0, ss0, ms0));
	sl.insert(i, Util::Format(L"%d:%02d:%02d.%02d", hh1, mm1, ss1, ms1));

	std::wstring ssf;
	
	if(!ConvertSSA(SSF::WCharInputStream(Util::Implode(sl, L",").c_str()), 6, index, ssf))
	{
		return E_FAIL;
	}
	
	return AppendSSF(start, stop, ssf.c_str());
}

STDMETHODIMP CTextSubProvider::SetFont(LPCWSTR name, float size, bool bold, bool italic, bool underline, bool strikethrough)
{
	CAutoLock cAutoLock(this);

	m_defstyle.face = name;
	m_defstyle.size = size;
	m_defstyle.bold = bold;
	m_defstyle.italic = italic;
	m_defstyle.underline = underline;
	m_defstyle.strikethrough = strikethrough;

	return S_OK;
}

STDMETHODIMP CTextSubProvider::SetColor(DWORD font, DWORD fill, DWORD background, DWORD shadow)
{
	CAutoLock cAutoLock(this);

	m_defstyle.color[0] = font;
	m_defstyle.color[1] = fill;
	m_defstyle.color[2] = background;
	m_defstyle.color[3] = shadow;

	return S_OK;
}

STDMETHODIMP CTextSubProvider::SetBackgroundSize(float size)
{
	CAutoLock cAutoLock(this);

	m_defstyle.outline = size;

	return S_OK;
}

STDMETHODIMP CTextSubProvider::SetShadowDepth(float depth)
{
	CAutoLock cAutoLock(this);

	m_defstyle.shadow = depth;

	return S_OK;
}

STDMETHODIMP CTextSubProvider::SetMargin(int alignment, int left, int top, int right, int bottom)
{
	CAutoLock cAutoLock(this);

	m_defstyle.alignment = alignment >= 1 && alignment <= 9 ? alignment : 2;
	m_defstyle.left = left;
	m_defstyle.top = top;
	m_defstyle.right = right;
	m_defstyle.bottom = bottom;

	return S_OK;
}

STDMETHODIMP CTextSubProvider::SetDefaultStyle()
{
	if(m_file != NULL)
	{
		if(SSF::Definition* def = m_file->GetDefByName(L"subtitle"))
		{
			// TODO
			/*
			def->SetChild(L"frame.reference", L"window");
			def->SetChild(L"frame.resolution.cx", 640.0f);
			def->SetChild(L"frame.resolution.cy", 480.0f);
			*/

			switch((m_defstyle.alignment - 1) / 3)
			{
			case 0:
				def->SetChild(L"style.placement.align.v", 1.0f);
				break;
			case 1:
				def->SetChild(L"style.placement.align.v", 0.5f);
				break;
			case 2:
				def->SetChild(L"style.placement.align.v", 0.0f);
				break;
			}

			switch((m_defstyle.alignment - 1) % 3)
			{
			case 0:
				def->SetChild(L"style.placement.align.h", 0.0f);
				break;
			case 1:
				def->SetChild(L"style.placement.align.h", 0.5f);
				break;
			case 2:
				def->SetChild(L"style.placement.align.h", 1.0f);
				break;
			}

			def->SetChild(L"style.placement.margin.l", (float)m_defstyle.left);
			def->SetChild(L"style.placement.margin.t", (float)m_defstyle.top);
			def->SetChild(L"style.placement.margin.r", (float)m_defstyle.right);
			def->SetChild(L"style.placement.margin.b", (float)m_defstyle.bottom);
			def->SetChild(L"style.font.face", m_defstyle.face.c_str());
			def->SetChild(L"style.font.size", m_defstyle.size);
			def->SetChild(L"style.font.weight", m_defstyle.bold ? L"bold" : L"normal");
			def->SetChild(L"style.font.undeline", m_defstyle.underline);
			def->SetChild(L"style.font.italic", m_defstyle.italic);
			def->SetChild(L"style.font.strikethrough", m_defstyle.strikethrough);
			def->SetChild(L"style.font.color.r", (float)m_defstyle.r(0));
			def->SetChild(L"style.font.color.g", (float)m_defstyle.g(0));
			def->SetChild(L"style.font.color.b", (float)m_defstyle.b(0));
			def->SetChild(L"style.font.color.a", (float)m_defstyle.a(0));
			def->SetChild(L"style.fill.color.r", (float)m_defstyle.r(1));
			def->SetChild(L"style.fill.color.g", (float)m_defstyle.g(1));
			def->SetChild(L"style.fill.color.b", (float)m_defstyle.b(1));
			def->SetChild(L"style.fill.color.a", (float)m_defstyle.a(1));
			def->SetChild(L"style.background.size", m_defstyle.outline);
			def->SetChild(L"style.background.color.r", (float)m_defstyle.r(2));
			def->SetChild(L"style.background.color.g", (float)m_defstyle.g(2));
			def->SetChild(L"style.background.color.b", (float)m_defstyle.b(2));
			def->SetChild(L"style.background.color.a", (float)m_defstyle.a(2));
			def->SetChild(L"style.shadow.depth", m_defstyle.shadow);
			def->SetChild(L"style.shadow.color.r", (float)m_defstyle.r(3));
			def->SetChild(L"style.shadow.color.g", (float)m_defstyle.g(3));
			def->SetChild(L"style.shadow.color.b", (float)m_defstyle.b(3));
			def->SetChild(L"style.shadow.color.a", (float)m_defstyle.a(3));
		}

		return S_OK;
	}

	return E_UNEXPECTED;
}

//

std::wstring CTextSubProvider::EscapeStyleName(LPCWSTR style)
{
	// only [0-9a-zA-Z_] characters are allowed in style names

	std::wstring s = L"__";

	while(*style != 0)
	{
		wchar_t c = *style;

		if(iswalnum(c)) s += c;
		else s += Util::Format(L"%x", c);

		style++;
	}

	return s;
}

std::wstring CTextSubProvider::EscapeFaceName(LPCWSTR face)
{
	std::wstring s = face;

	Util::Replace(s, L"'", L"\\'");

	return s;
}

DWORD CTextSubProvider::HexToDWORD(LPCWSTR str)
{
	if(*str == '&') str++;
	if(*str == 'H') str++;

	return wcstoul(str, NULL, 16);
}

bool CTextSubProvider::ConvertSSA(SSF::InputStream& s, int ver, int index, std::wstring& ssf)
{
	s.SetCharSet(DEFAULT_CHARSET);

	std::wstring layer;
	std::wstring hh[2], mm[2], ss[2], ms[2];
	std::wstring stylename;
	std::wstring actor;
	std::wstring left, right, top, bottom;
	std::wstring effect;
	std::wstring text;

	if(s.ReadString(layer, ',') != ',') return false; 
	if(ver < 5) layer = L"0";
	if(s.ReadString(hh[0], ':') != ':') return false;
	if(s.ReadString(mm[0], ':') != ':') return false;
	if(s.ReadString(ss[0], '.') != '.') return false;
	if(s.ReadString(ms[0], ',') != ',') return false;
	if(s.ReadString(hh[1], ':') != ':') return false;
	if(s.ReadString(mm[1], ':') != ':') return false;
	if(s.ReadString(ss[1], '.') != '.') return false;
	if(s.ReadString(ms[1], ',') != ',') return false;
	if(s.ReadString(stylename, ',') != ',') return false;
	if(s.ReadString(actor, ',') != ',') return false;
	if(s.ReadString(left, ',') != ',') return false;
	if(s.ReadString(right, ',') != ',') return false;
	if(s.ReadString(top, ',') != ',') return false;
	if(ver >= 6) {if(s.ReadString(bottom, ',') != ',') return false;}
	else bottom = top;
	if(s.ReadString(effect, ',') != ',') return false;

	stylename = Util::TrimLeft(stylename, L"*");

	auto i = m_styles.find(stylename);

	if(i == m_styles.end())
	{
		return false;
	}

	Style style = i->second;

	if(left != L"0000") style.left = _wtoi(left.c_str());
	if(top != L"0000") style.top = _wtoi(top.c_str());
	if(right != L"0000") style.right = _wtoi(right.c_str());
	if(bottom != L"0000") style.bottom = _wtoi(bottom.c_str());

	// effect

	std::vector<std::wstring> params;

	effect = Util::MakeLower(Util::Explode(effect, params, ';'));

	if(effect == L"banner" && params.size() >= 2)
	{
		bool rev = params.size() >= 3 && _wtoi(params[2].c_str()) != 0;

		effect = Util::Format(L"style.placement {margin {l: 'right'; r: 'left';}; align.h: '%s';};", rev ? L"right" : L"left");
		text = Util::Format(L"[{time: startstop; placement.align.h: '%s';}]", rev ? L"left" : L"right");

		// TODO: delay
	}
	else if(effect.find(L"scroll") == 0 && params.size() >= 4)
	{
		bool rev = effect.find(L"up") != std::wstring::npos;

		int top = _wtoi(params[1].c_str());
		int bottom = _wtoi(params[2].c_str());

		effect = Util::Format(L"style.placement {margin {t: %d; b: %d;}; clip {t: %d; b: %d;}; align.v: '%s';};", bottom, m_playres.y - top, top, bottom, rev ? L"top" : L"bottom");
		text = Util::Format(L"[{time: startstop; placement.align.v: '%s';}]", rev ? L"bottom" : L"top");

		// TODO: delay
	}
	else
	{
		effect.clear();
	}

	//

	struct {int h, m, s, ms; float f;} time[2];

	for(int i = 0; i < 2; i++)
	{
		time[i].h = _wtoi(hh[i].c_str());
		time[i].m = _wtoi(mm[i].c_str());
		time[i].s = _wtoi(ss[i].c_str());
		time[i].ms = _wtoi(ms[i].c_str()) * 10;
		time[i].f = (float)((double)(((time[i].h * 60 + time[i].m) * 60 + time[i].s) * 1000 + time[i].ms) / 1000);
	}

	//

	ModifierConverter mc(style.charset, style.wrapstyle);

	bool karaoke = false;

	int c = -1;

	while(c != 0 && c != '\n')
	{
		s.SetCharSet(mc.m_charset);

		std::wstring str;

		c = s.ReadString(str, '{');

		if(!str.empty())
		{
			text += ConvertSSAText(str, mc.m_wrapstyle);
		}

		if(c == '{')
		{
			c = s.ReadString(str, '}');

			if(c == '}')
			{
				mc.Process(str.c_str(), style, time[0].f, time[1].f);

				if(mc.m_karaoke)
				{
					if(karaoke)
					{
						text += L"}";
					}
				}

				text += L"[" + Util::Implode(mc.m_style, L", ") + L"]";

				if(mc.m_karaoke)
				{
					text += L"{";

					karaoke = true;
				}
			}
		}
	}

	if(karaoke)
	{
		text += L"}";
	}

	ssf = Util::Format(
		L"subtitle {\n"
		L" wrap: '%s';\n"
		L" layer: %d;\n"
		L" index: %d;\n"
		L" style: %s;\n"
		L" actor: '%s';\n" // not used
		L" style.placement.margin {l: %d; t: %d; r: %d; b: %d;};\n"
		L" time {start: %02d:%02d:%02d.%03d; stop: %02d:%02d:%02d.%03d;};\n"
		L" %s\n"
		L" @ {%s};\n"
		L"};\n",
		mc.m_wrapstyle == 1 ? L"normal" : mc.m_wrapstyle == 2 ? L"manual" : L"even",
		_wtoi(layer.c_str()),
		index,
		style.safename.c_str(),
		actor.c_str(),
		style.left, style.top, style.right, style.bottom,
		time[0].h, time[0].m, time[0].s, time[0].ms,
		time[1].h, time[1].m, time[1].s, time[1].ms,
		effect.c_str(),
		text.c_str());

	return true;
}

std::wstring CTextSubProvider::ConvertSSAText(const std::wstring& str, int wrapstyle)
{
	std::wstring text;

	SSF::WCharInputStream s(str.c_str(), false);

	int c = -1;
	
	while(c != 0)
	{
		std::wstring t;

		c = s.ReadString(t, '\\');

		if(!t.empty())
		{
			text += SSF::SubtitleFile::Escape(t.c_str());
		}

		if(c == '\\')
		{
			c = s.Get();

			switch(c)
			{
			case 'N': text += L"\\n"; break;
			case 'n': text += wrapstyle == 2 ? L"\\n" : L" "; break;
			case 'h': text += L"\\h"; break;
			}
		}
	}

	return text;
}

CTextSubProvider::ModifierConverter::ModifierConverter(int charset, int wrapstyle)
{
	static LPCWSTR modifiers[] = 
	{
		L"b", L"i", L"u", L"s", L"bord", L"shad", L"be", L"blur", 
		L"fn", L"fs", L"fsc", L"fscx", L"fscy", L"fsp", L"fr", L"frx", L"fry", L"frz", L"fe", 
		L"c", L"1c", L"2c", L"3c", L"4c", L"alpha", L"1a", L"2a", L"3a", L"4a",
		L"a", L"an", L"k", L"K", L"kf", L"ko", L"q", L"r", 
		L"t", L"move", L"pos", L"org", L"fad", L"fade", L"clip", L"p", L"pbo"
	};

	for(int i = 0; i < sizeof(modifiers) / sizeof(modifiers[0]); i++)
	{
		m_modifiers.insert(modifiers[i]);
	}

	m_charset = charset;
	m_wrapstyle = wrapstyle;
	m_karaoke = false;
}

void CTextSubProvider::ModifierConverter::Process(LPCWSTR str, const Style& style, float start, float stop)
{
	m_karaoke = false;
	m_style.clear();

	std::list<Modifier> ml;

	Parse(str, ml);

	for(auto i = ml.begin(); i != ml.end(); i++)
	{
		Modifier& m = *i;

		std::wstring s;

		if(m.name == L"b")
		{
			s = L"{font.weight: ";

			if(m.params.empty())
			{
				m.params.push_back(style.bold ? L"bold" : L"normal");
			}

			if(m.params[0] == L"0")
			{
				s += L"'normal'";
			}
			else if(m.params[0] == L"1")
			{
				s += L"'bold'";
			}
			else
			{
				s += Util::Format(L"%d", _wtoi(m.params[0].c_str()));
			}

			s += L";}";
		}
		else if(m.name == L"i")
		{
			s = Util::Format(L"{font.italic: %d;}", !m.params.empty() ? _wtoi(m.params[0].c_str()) : style.bold);
		}
		else if(m.name == L"u")
		{
			s = Util::Format(L"{font.underline: %d;}", !m.params.empty() ? _wtoi(m.params[0].c_str()) : style.underline);
		}
		else if(m.name == L"s")
		{
			s = Util::Format(L"{font.strikethrough: %d;}", !m.params.empty() ? _wtoi(m.params[0].c_str()) : style.strikethrough);
		}
		else if(m.name == L"bord")
		{
			s = Util::Format(L"{background.size: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) : style.outline);
		}
		else if(m.name == L"shad")
		{
			s = Util::Format(L"{shadow.depth: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) : style.shadow);
		}
		else if(m.name == L"be")
		{
			int blur = !m.params.empty() ? _wtoi(m.params[0].c_str()) : 0;

			s = Util::Format(L"{background.blur: %d; shadow.blur: %d;}", blur, blur);
		}
		else if(m.name == L"blur")
		{
			// TODO: full spatial blur?
		}
		else if(m.name == L"fn")
		{
			s = Util::Format(L"{font.face: '%s';}", !m.params.empty() ? EscapeFaceName(m.params[0].c_str()).c_str() : style.safeface.c_str());
		}
		else if(m.name == L"fs")
		{
			s = Util::Format(L"{font.size: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) : style.size);
		}
		else if(m.name == L"fsc")
		{
			s = Util::Format(L"{font.scale {cx: %f; cy: %f;};}", style.scalex, style.scaley);
		}
		else if(m.name == L"fscx")
		{
			s = Util::Format(L"{font.scale.cx: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) / 100 : style.scalex);
		}
		else if(m.name == L"fscy")
		{
			s = Util::Format(L"{font.scale.cy: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) / 100 : style.scaley);
		}
		else if(m.name == L"fsp")
		{
			s = Util::Format(L"{font.spacing: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) : style.spacing);
		}
		else if(m.name == L"fr" || m.name == L"frx")
		{
			s = Util::Format(L"{placement.angle.x: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) : style.anglex);
		}
		else if(m.name == L"fry")
		{
			s = Util::Format(L"{placement.angle.y: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) : style.angley);
		}
		else if(m.name == L"frz")
		{
			s = Util::Format(L"{placement.angle.z: %f;}", !m.params.empty() ? (float)_wtof(m.params[0].c_str()) : style.anglez);
		}
		else if(m.name == L"fe")
		{
			m_charset = !m.params.empty() ? _wtoi(m.params[0].c_str()) : style.charset;
		}
		else if(m.name == L"1c" || m.name == L"c")
		{
			DWORD c = !m.params.empty() ? HexToDWORD(m.params[0].c_str()) : style.color[0];

			s = Util::Format(L"{font.color {r: %d; g: %d; b: %d;};}", (c >> 0) & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
		}
		else if(m.name == L"2c")
		{
			DWORD c = !m.params.empty() ? HexToDWORD(m.params[0].c_str()) : style.color[1];

			s = Util::Format(L"{fill.color {r: %d; g: %d; b: %d;};}", (c >> 0) & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
		}
		else if(m.name == L"3c")
		{
			DWORD c = !m.params.empty() ? HexToDWORD(m.params[0].c_str()) : style.color[2];

			s = Util::Format(L"{background.color {r: %d; g: %d; b: %d;};}", (c >> 0) & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
		}
		else if(m.name == L"4c")
		{
			DWORD c = !m.params.empty() ? HexToDWORD(m.params[0].c_str()) : style.color[3];

			s = Util::Format(L"{shadow.color {r: %d; g: %d; b: %d;};}", (c >> 0) & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
		}
		else if(m.name == L"1a" || m.name == L"alpha")
		{
			DWORD c = !m.params.empty() ? 0xff - HexToDWORD(m.params[0].c_str()) : (style.color[0] >> 24);

			s = Util::Format(L"{font.color {a: %d;};}", c);
		}
		else if(m.name == L"2a")
		{
			DWORD c = !m.params.empty() ? 0xff - HexToDWORD(m.params[0].c_str()) : (style.color[1] >> 24);

			s = Util::Format(L"{fill.color {a: %d;};}", c);
		}
		else if(m.name == L"3a")
		{
			DWORD c = !m.params.empty() ? 0xff - HexToDWORD(m.params[0].c_str()) : (style.color[2] >> 24);

			s = Util::Format(L"{background.color {a: %d;};}", c);
		}
		else if(m.name == L"4a")
		{
			DWORD c = !m.params.empty() ? 0xff - HexToDWORD(m.params[0].c_str()) : (style.color[3] >> 24);

			s = Util::Format(L"{shadow.color {a: %d;};}", c);
		}
		else if(m.name == L"a" || m.name == L"an")
		{
			int alignment = style.alignment;

			if(!m.params.empty())
			{
				alignment = _wtoi(m.params[0].c_str());

				if(m.name == L"a")
				{
					int n = alignment & 3;
					
					if(alignment & 4) n += 6; // top
					else if(alignment & 8) n += 3; // middle

					alignment = n;
				}
			}

			if(alignment >= 1 && alignment <= 9)
			{
				s = Util::Format(L"{placement.align: %s;}", m_alignment[alignment - 1]);
			}
		}
		else if(m.name == L"q")
		{
			m_wrapstyle = !m.params.empty() ? _wtoi(m.params[0].c_str()) : style.wrapstyle;
		}
		else if(m.name == L"r")
		{
			s = (!m.params.empty() ? EscapeStyleName(m.params[0].c_str()) : style.safename);
		}
		else if(m.name == L"p")
		{
			// TODO: L"pbo"
	
			s = Util::Format(L"{content: '%s';}", !m.params.empty() && m.params[0] != L"0" ? L"polygon" : L"text");
		}
		else if(m.name == L"org")
		{
			if(m.params.size() == 2)
			{
				float x = (float)_wtof(m.params[0].c_str());
				float y = (float)_wtof(m.params[1].c_str());

				s = Util::Format(L"{placement.org {x: %f; y: %f;};}", x, y);
			}
		}
		else if(m.name == L"pos")
		{
			if(m.params.size() == 2)
			{
				float x = (float)_wtof(m.params[0].c_str());
				float y = (float)_wtof(m.params[1].c_str());

				m_style.push_back(Util::Format(L"{placement.pos {x: %f; y: %f;};}", x, y));
			}
		}
		else if(m.name == L"move")
		{
			if(m.params.size() >= 4)
			{
				float x0 = (float)_wtof(m.params[0].c_str());
				float y0 = (float)_wtof(m.params[1].c_str());
				float x1 = (float)_wtof(m.params[2].c_str());
				float y1 = (float)_wtof(m.params[3].c_str());

				m_style.push_back(Util::Format(L"{placement.pos {x: %f; y: %f;};}", x0, y0));

				if(m.params.size() == 6)
				{
					float t0 = (float)_wtoi(m.params[4].c_str()) / 1000;
					float t1 = (float)_wtoi(m.params[5].c_str()) / 1000;

					m_style.push_back(Util::Format(L"{placement.pos {x: %f; y: %f;}; time {start: %f; stop: %f;}}", x1, y1, t0, t1));
				}
				else
				{
					m_style.push_back(Util::Format(L"{placement.pos {x: %f; y: %f;}; time: startstop;}", x1, y1));
				}
			}
		}
		else if(m.name == L"clip")
		{
			if(m.params.size() == 4)
			{
				float x0 = (float)_wtof(m.params[0].c_str());
				float y0 = (float)_wtof(m.params[1].c_str());
				float x1 = (float)_wtof(m.params[2].c_str());
				float y1 = (float)_wtof(m.params[3].c_str());

				m_style.push_back(Util::Format(L"{placement.clip {l: %f; t: %f; r: %f; b: %f;};}", x0, y0, x1, y1));
			}
		}
		else if(m.name == L"fade" || m.name == L"fad")
		{
			LPCWSTR fmt = L"{color.a: %d;}";

			if(m.params.size() == 2)
			{
				std::wstring s0 = L"transp"; // Util::Format(fmt, 0, 0, 0, 0);
				std::wstring s1 = L"opaque"; // Util::Format(fmt, 255, 255, 255, 255);

				float t0 = (float)_wtoi(m.params[0].c_str()) / 1000;
				float t1 = (stop - start) - (float)_wtoi(m.params[1].c_str()) / 1000;

				m_style.push_back(s0);
				m_style.push_back(s1 + Util::Format(L" {time {start: 'start'; stop: %f;};}", t0));
				m_style.push_back(s1 + Util::Format(L" {time {start: %f; stop: %f;};}", t0, t1));
				m_style.push_back(s0 + Util::Format(L" {time {start: %f; stop: 'stop';};}", t1));
			}
			else if(m.params.size() == 7)
			{
				int a0 = 255 - _wtoi(m.params[0].c_str());
				int a1 = 255 - _wtoi(m.params[1].c_str());
				int a2 = 255 - _wtoi(m.params[2].c_str());

				std::wstring s0 = Util::Format(fmt, a0, a0, a0, a0);
				std::wstring s1 = Util::Format(fmt, a1, a1, a1, a1);
				std::wstring s2 = Util::Format(fmt, a2, a2, a2, a2);

				float t0 = (float)_wtoi(m.params[3].c_str()) / 1000;
				float t1 = (float)_wtoi(m.params[4].c_str()) / 1000;
				float t2 = (float)_wtoi(m.params[5].c_str()) / 1000;
				float t3 = (float)_wtoi(m.params[6].c_str()) / 1000;

				m_style.push_back(s0);
				m_style.push_back(s1 + Util::Format(L" {time {start: %f; stop: %f;};}", t0, t1));
				m_style.push_back(s1 + Util::Format(L" {time {start: %f; stop: %f;};}", t1, t2));
				m_style.push_back(s2 + Util::Format(L" {time {start: %f; stop: %f;};}", t2, t3));
			}
		}
		else if(m.name == L"t")
		{
			float t0 = start;
			float t1 = stop;
			float accel = 1.0f;
			std::wstring mods;
			bool startstop = false;

			if(m.params.size() == 1)
			{
				startstop = true;
				t0 -= start;
				t1 -= start;
				mods = m.params[0];
			}
			else if(m.params.size() == 2)
			{
				startstop = true;
				t0 -= start;
				t1 -= start;
				accel = (float)_wtof(m.params[0].c_str());
				mods = m.params[1];
			}
			else if(m.params.size() == 3)
			{
				t0 = (float)_wtoi(m.params[0].c_str()) / 1000;
				t1 = (float)_wtoi(m.params[1].c_str()) / 1000;
				mods = m.params[2];
			}
			else if(m.params.size() == 4)
			{
				t0 = (float)_wtoi(m.params[0].c_str()) / 1000;
				t1 = (float)_wtoi(m.params[1].c_str()) / 1000;
				accel = (float)_wtof(m.params[2].c_str());
				mods = m.params[3];
			}

			if(!mods.empty())
			{
				ModifierConverter mc(m_charset, m_wrapstyle);

				mc.Process(mods.c_str(), style, start, stop);

				if(!mc.m_style.empty())
				{
					if(startstop)
					{
						s = Util::Format(L"{time: startstop {transition: %f;};} ", accel);
					}
					else
					{
						s = Util::Format(L"{time {start: %f; stop: %f; transition: %f;};} ", t0, t1, accel);
					}

					s += Util::Implode(mc.m_style, L" ");
				}
			}
		}
		else if(m.name == L"k")
		{
			int delay = !m.params.empty() ? _wtoi(m.params[0].c_str()) : 100;

			std::wstring s0 = Util::Format(L"{time.id: 10000; font.color {r: %d; g: %d; b: %d; a: %d;};}", style.r(1), style.g(1), style.b(1), style.a(1));
			std::wstring s1 = Util::Format(L"k {time.stop: +%d; font.color {r: %d; g: %d; b: %d; a: %d;};}", delay, style.r(0), style.g(0), style.b(0), style.a(0));

			m_style.push_back(s0);
			m_style.push_back(s1);

			m_karaoke = true;
		}
		else if(m.name == L"ko")
		{
			int delay = !m.params.empty() ? _wtoi(m.params[0].c_str()) : 100;

			std::wstring s0 = Util::Format(L"{time.id: 10000; background.color {r: %d; g: %d; b: %d; a: %d;};}", style.r(1), style.g(1), style.b(1), style.a(1));
			std::wstring s1 = Util::Format(L"k {time.stop: +%d; background.color {r: %d; g: %d; b: %d; a: %d;};}", delay, style.r(0), style.g(0), style.b(0), style.a(0));

			m_style.push_back(s0);
			m_style.push_back(s1);

			m_karaoke = true;
		}
		else if(m.name == L"kf" || m.name == L"K")
		{
			int delay = !m.params.empty() ? _wtoi(m.params[0].c_str()) : 100;

			s = Util::Format(L"K {time.stop: +%d;}", delay);

			m_karaoke = true;
		}

		if(!s.empty())
		{
			m_style.push_back(s);
		}
	}
}

void CTextSubProvider::ModifierConverter::Parse(LPCWSTR str, std::list<Modifier>& mods)
{
	mods.clear();

	SSF::WCharInputStream s(str, false);

	std::wstring tmp;

	int c = s.ReadString(tmp, '\\');

	while(c == '\\')
	{
		Modifier m;

		c = s.Get();

		while(c != '(' && c != '\\' && c != SSF::Stream::EOS)
		{
			m.name += c;

			c = s.Get();
		}

		if(!m.name.empty())
		{
			if(c == '(')
			{
				c = s.ReadString(tmp, ')');

				if(c == ')')
				{
					Util::Explode(tmp, m.params, L",");
				}
			}
			else
			{
				for(int i = m.name.size(); i > 0; i--)
				{
					tmp = m.name.substr(0, i);

					if(m_modifiers.find(tmp) != m_modifiers.end())
					{
						std::wstring p = Util::Trim(m.name.substr(i));

						if(!p.empty()) 
						{
							m.params.push_back(p);
						}

						m.name = tmp;

						break;
					}
				}
			}

			mods.push_back(m);
		}

		if(c != '\\')
		{
			c = s.ReadString(tmp, '\\');
		}
	}
}

LPCWSTR CTextSubProvider::m_ssf =
	L"file#file {format: 'ssf'; version: 1;};\n"
	L"#B : b;\n"
	L"#U : u;\n"
	L"#I : i;\n"
	L"#S : s;\n"
	L"subtitle#subtitle {\n"
	L" frame {reference: 'window'; resolution: {cx: 640; cy: 480;};};\n"
	L"};\n";

LPCWSTR CTextSubProvider::m_alignment[] = 
{
	L"bottomleft", L"bottomcenter", L"bottomright",
	L"middleleft", L"middlecenter", L"middleright",
	L"topleft", L"topcenter", L"topright",
};
