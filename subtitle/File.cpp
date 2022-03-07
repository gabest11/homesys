/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "File.h"

#ifndef iswcsym
#define iswcsym(c) (iswalnum(c) || (c) == '_')
#endif

namespace SSF
{
	File::File()
	{
	}

	File::~File()
	{
	}

	void File::Parse(InputStream& s)
	{
		Reset();

		ParseDefs(s);

		Commit();

		if(s.Peek() != Stream::EOS)
		{
			wprintf(L"Warning: parsing ended before EOS!\n");
		}
	}

	void File::ParseDefs(InputStream& s, Reference* parent)
	{
		if(parent == NULL)
		{
			parent = GetRootRef();
		}

		while(s.SkipWhiteSpace(L";") != '}' && s.Peek() != Stream::EOS)
		{
			NodePriority priority = PNormal;
			std::list<std::wstring> types;
			std::wstring name;

			int c = s.SkipWhiteSpace();

			if(c == '*') 
			{
				s.Get(); 
				priority = PLow;
			}
			else if(c == '!') 
			{
				s.Get(); 
				priority = PHigh;
			}

			ParseTypes(s, types);

			if(s.SkipWhiteSpace() == '#')
			{
				s.Get();

				ParseName(s, name);
			}

			if(types.empty())
			{
				if(name.empty()) 
				{
					s.ThrowError(L"syntax error");
				}

				types.push_back(L"?");
			}

			Reference* ref = parent;

			while(types.size() > 1)
			{
				ref = CreateRef(CreateDef(ref, types.front().c_str()));

				types.pop_front();
			}

			Definition* def = NULL;

			if(!types.empty())
			{
				def = CreateDef(ref, types.front().c_str(), name.c_str(), priority);

				types.pop_front();
			}

			c = s.SkipWhiteSpace(L":=");

			if(c == '"' || c == '\'')
			{
				ParseQuotedString(s, def);
			}
			else if(iswdigit(c) || c == '+' || c == '-') 
			{
				ParseNumber(s, def);
			}
			else if(def->IsType(L"@")) 
			{
				ParseBlock(s, def);
			}
			else 
			{
				ParseRefs(s, def);
			}
		}

		s.Get();
	}

	void File::ParseTypes(InputStream& s, std::list<std::wstring>& types)
	{
		types.clear();

		std::wstring str;

		for(int c = s.SkipWhiteSpace(); iswcsym(c) || c == '.' || c == '@'; c = s.Peek())
		{
			c = s.Get();

			if(c == '.') 
			{
				if(str.empty()) 
				{
					s.ThrowError(L"'type' cannot be an empty string");
				}

				if(!iswcsym(s.Peek())) 
				{
					s.ThrowError(L"unexpected dot after type '%s'", str.c_str());
				}

				types.push_back(str);

				str.clear();
			}
			else
			{
				if(str.empty() && iswdigit(c)) 
				{
					s.ThrowError(L"'type' cannot start with a number");
				}

				if((!str.empty() || !types.empty()) && c == '@') 
				{
					s.ThrowError(L"unexpected @ in 'type'");
				}

				str += (WCHAR)c;
			}
		}

		if(!str.empty())
		{
			types.push_back(str);
		}
	}

	void File::ParseName(InputStream& s, std::wstring& name)
	{
		name.clear();

		for(int c = s.SkipWhiteSpace(); iswcsym(c); c = s.Peek())
		{
			if(name.empty() && iswdigit(c)) 
			{
				s.ThrowError(L"'name' cannot start with a number");
			}

			name += (WCHAR)s.Get();
		}
	}

	void File::ParseQuotedString(InputStream& s, Definition* def)
	{
		std::wstring v;

		int quote = s.SkipWhiteSpace();

		if(quote != '"' && quote != '\'') 
		{
			s.ThrowError(L"expected qouted string");
		}

		s.Get();

		for(int c = s.Peek(); c != Stream::EOS; c = s.Peek())
		{
			c = s.Get();

			if(c == '\\') 
			{
				c = s.Get();
			}
			else if(c == quote) 
			{
				def->SetAsValue(Definition::DefString, v.c_str()); 

				return;
			}
			
			if(c == '\n') 
			{
				s.ThrowError(L"qouted string terminated unexpectedly by a new-line character");
			}
			else if(c == Stream::EOS) 
			{
				s.ThrowError(L"qouted string terminated unexpectedly by EOS");
			}

			v += (WCHAR)c;
		}

		s.ThrowError(L"unterminated quoted string");
	}

	void File::ParseNumber(InputStream& s, Definition* def)
	{
		std::wstring v, u;

		for(int c = s.SkipWhiteSpace(); iswxdigit(c) || wcschr(L"+-.x:", c); c = s.Peek())
		{
			if((c == '+' || c == '-') && !v.empty()
			|| (c == '.' && (v.empty() || v.find(L".") != std::wstring::npos || v.find(L"x") != std::wstring::npos))
			|| (c == 'x' && v != L"0")
			|| (c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F') && v.find(L"0x") != 0
			|| (c == ':' && v.empty()))
			{
				s.ThrowError(L"unexpected character '%c' in number", c);
			}

			v += (WCHAR)s.Get();
		}

		if(v.empty()) 
		{
			s.ThrowError(L"invalid number");
		}

		for(int c = s.SkipWhiteSpace(); iswcsym(c); c = s.Peek())
		{
			u += (WCHAR)s.Get();
		}

		def->SetAsNumber(v.c_str(), u.c_str());
	}

	void File::ParseBlock(InputStream& s, Definition* def)
	{
		int line = s.GetLine();
		int col = s.GetCol();

		std::wstring v;

		int c = s.SkipWhiteSpace(L":=");

		if(c != '{') 
		{
			s.ThrowError(L"expected '{'");
		}

		s.Get();

		int depth = 0;

		for(int c = s.Peek(); c != Stream::EOS; c = s.Peek())
		{
			c = s.Get();

			if(c == '}' && depth == 0) 
			{
				def->SetAsValue(Definition::DefBlock, v.c_str()); 
				
				return;
			}

			if(c == '\\') 
			{
				v += (WCHAR)c; 
				c = s.Get();
			}
			else if(c == '{') 	
			{
				depth++;
			}
			else if(c == '}')
			{
				depth--;
			}
			
			if(c == Stream::EOS) 
			{
				s.ThrowError(L"block terminated unexpectedly by EOS");
			}

			v += (WCHAR)c;
		}

		s.ThrowError(L"unterminated block starting at %d, %d", line, col);
	}

	void File::ParseRefs(InputStream& s, Definition* parent, LPCWSTR term)
	{
		int c = s.SkipWhiteSpace();

		do
		{
			if(parent->IsValue()) 
			{
				s.ThrowError(L"cannot mix references with other values");
			}

			if(c == '{')
			{
				s.Get();

				ParseDefs(s, CreateRef(parent));
			}
			else if(iswcsym(c))
			{
				std::wstring str;

				ParseName(s, str);

				// TODO: allow spec references: parent.<type>, self.<type>, child.<type>

				Definition* def = GetDefByName(str.c_str());

				if(def == NULL)
				{
					s.ThrowError(L"cannot find definition of '%s'", str.c_str());
				}

				if(!parent->IsVisible(def)) 
				{
					s.ThrowError(L"cannot access '%s' from here", str.c_str());
				}

				parent->Add(def);
			}
			else if(!wcschr(term, c) && c != Stream::EOS)
			{
				s.ThrowError(L"unexpected character '%c'", c);
			}

			c = s.SkipWhiteSpace();
		}
		while(!wcschr(term, c) && c != Stream::EOS);
	}
}
