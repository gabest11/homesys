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
#include "Node.h"
#include "NodeFactory.h"
#include "Exception.h"
#include "Split.h"
#include "../util/String.h"

namespace SSF
{
	Node::Node(NodeFactory* pnf, LPCWSTR name)
		: m_pnf(pnf)
		, m_type(1, '?')
		, m_name(name)
		, m_priority(PNormal)
		, m_predefined(false)
		, m_parent(NULL)
	{
	}

	void Node::Add(Node* node)
	{
		for(auto i = m_nodes.begin(); i != m_nodes.end(); i++) // TODO: slow
		{
			if(*i == node)
			{
				m_nodes.erase(i);
				m_nodes.push_front(node);

				return;
			}
		}

		m_nodes.push_back(node);
		m_name2node[node->m_name] = node;
	}

	bool Node::IsNameUnknown()
	{
		return m_name.empty() || !!iswdigit(m_name[0]);
	}

	bool Node::IsTypeUnknown()
	{
		return m_type.empty() || m_type == L"?";
	}

	bool Node::IsType(LPCWSTR type)
	{
		return m_type == type;
	}

	void Node::GetChildDefs(std::list<Definition*>& l, LPCWSTR type, bool first)
	{
		std::list<Definition*> rdl[3];

		if(first)
		{
			Definition* def = m_pnf->GetDefByName(m_type.c_str());

			if(def != NULL)
			{
				def->GetChildDefs(rdl[def->m_priority], type, false);
			}
		}

		for(auto i = m_nodes.begin(); i != m_nodes.end(); i++)
		{
			Node* n = *i;

			if(n != NULL)
			{
				n->GetChildDefs(rdl[n->m_priority], type, false);
			}
		}

		for(int i = 0; i < sizeof(rdl) / sizeof(rdl[0]); i++)
		{
			l.insert(l.end(), rdl[i].begin(), rdl[i].end());
		}
	}

	// Reference

	Reference::Reference(NodeFactory* pnf, LPCWSTR name)
		: Node(pnf, name)
	{
	}

	Reference::~Reference()
	{
	}

	void Reference::GetChildDefs(std::list<Definition*>& l, LPCWSTR type, bool first)
	{
		std::list<Definition*> rdl[3];

		for(auto i = m_nodes.begin(); i != m_nodes.end(); i++)
		{
			Definition* def = dynamic_cast<Definition*>(*i);

			if(def != NULL)
			{
				if(!type || def->m_type == type) // TODO: faster lookup
				{
					rdl[def->m_priority].push_back(def);
				}
			}
		}

		for(int i = 0; i < sizeof(rdl) / sizeof(rdl[0]); i++)
		{
			l.insert(l.end(), rdl[i].begin(), rdl[i].end());
		}
	}

	void Reference::Dump(OutputStream& s, int level, bool last)
	{
		if(m_predefined) return;

		std::wstring tabs(level * 4, ' ');

		s.PutString(L" {\n");

		for(auto i = m_nodes.begin(); i != m_nodes.end(); i++)
		{
			Definition* def = dynamic_cast<Definition*>(*i);

			if(def != NULL)
			{
				def->Dump(s, level + 1, *i == m_nodes.back());
			}
		}

		s.PutString((tabs + L"}").c_str());
	}

	// Definition

	Definition::Definition(NodeFactory* pnf, LPCWSTR name)
		: Node(pnf, name)
		, m_deftype(DefNode)
		, m_autotype(false)
	{
	}

	Definition::~Definition()
	{
		RemoveFromCache();
	}

	bool Definition::IsVisible(Definition* def)
	{
		Node* n = m_parent;

		while(n != NULL)
		{
			auto i = n->m_name2node.find(def->m_name);

			if(i != n->m_name2node.end())
			{
				return true;
			}

			n = n->m_parent;
		}

		return false;
	}

	void Definition::Add(Node* n)
	{
//		if(Reference* pRef = dynamic_cast<Reference*>(pNode))
		{
			m_deftype = DefNode;

			if(IsTypeUnknown() && !n->IsTypeUnknown())
			{
				m_type = n->m_type;
				m_autotype = true;
			}

			RemoveFromCache(n->m_type.c_str());
		}

		__super::Add(n);
	}

	Definition& Definition::operator[] (LPCWSTR type) 
	{
		auto i = m_type2def.find(type);

		if(i != m_type2def.end())
		{
			return *i->second;
		}

		Definition* ret = new Definition(m_pnf, L"");

		ret->m_priority = PLow;
		ret->m_type = type;
		
		m_type2def[type] = ret;

		std::list<Definition*> l;

		GetChildDefs(l, type);

		for(auto i = l.begin(); i != l.end(); i++)
		{
			Definition* def = *i;

			ret->m_priority = def->m_priority;
			ret->m_parent = def->m_parent;

			if(def->IsValue())
			{
				ret->SetAsValue(def->m_deftype, def->m_value.c_str(), def->m_unit.c_str());
			}
			else
			{
				ret->m_deftype = DefNode; 
				ret->m_nodes.insert(ret->m_nodes.end(), def->m_nodes.begin(), def->m_nodes.end());
			}
		}

		return *ret;
	}

	void Definition::RemoveFromCache(LPCWSTR type)
	{
		if(type == NULL)
		{
			for(auto i = m_type2def.begin(); i != m_type2def.end(); i++)
			{
				delete i->second;
			}
		}
		else 
		{
			auto i = m_type2def.find(type);

			if(i != m_type2def.end())
			{
				delete i->second;

				m_type2def.erase(i);
			}
		}
	}

	bool Definition::IsValue(DefType t)
	{
		return t ? m_deftype == t : m_deftype != DefNode;
	}

	bool Definition::HasValue(LPCWSTR type)
	{
		return (*this)[type].IsValue();
	}

	void Definition::SetAsValue(DefType t, LPCWSTR v, LPCWSTR u)
	{
		m_nodes.clear();
		m_name2node.clear();
		m_deftype = t;
		m_value = v;
		m_unit = u;
	}

	void Definition::SetAsNumber(LPCWSTR v, LPCWSTR u)
	{
		SetAsValue(DefNumber, v, u);

		Number<float> n;

		GetAsNumber(n); // verification only, this will throw an exception if it isn't a number
	}

	template<class T> void Definition::GetAsNumber(Number<T>& n, std::map<std::wstring, T>* n2n)
	{
		std::wstring str = m_value;
		
		Util::Replace(str, L" ", L"");

		n.value = 0;
		n.unit = m_unit;
		n.sign = 0;

		if(n2n != NULL)
		{
			if(m_deftype == DefNode) 
			{
				throw Exception(L"expected value type");
			}

			auto i = n2n->find(str);

			if(i != n2n->end())
			{
				n.value = i->second;

				return;
			}
		}

		if(m_deftype != DefNumber) 
		{
			throw Exception(L"expected number");
		}

		n.sign = str.find(L"+") == 0 ? 1 : str.find(L"-") == 0 ? -1 : 0;

		str = Util::TrimLeft(str, L"+-");

		if(str.find(L"0x") == 0)
		{
			if(n.sign) 
			{
				throw Exception(L"hex values must be unsigned");
			}

			n.value = (T)wcstoul(str.substr(2).c_str(), NULL, 16);
		}
		else
		{
			std::wstring num_string = m_value + m_unit;

			if(m_num_string != num_string)
			{
				Split sa(':', str);
				Split sa2('.', !sa.empty() ? sa.back() : L"");

				if(sa.empty() || sa2.empty() || sa2.size() > 2) 
				{
					throw Exception(L"invalid number");
				}

				float f = 0;

				for(auto i = sa.begin(); i != sa.end(); i++)
				{
					f *= 60; 
					f += wcstoul(i->c_str(), NULL, 10);
				}

				if(sa2.size() > 1) 
				{
					f += (float)wcstoul(sa2[1].c_str(), NULL, 10) / pow(10.0f, (float)sa2[1].size());
				}

				if(n.unit == L"ms") {f /= 1000; n.unit = L"s";}
				else if(n.unit == L"m") {f *= 60; n.unit = L"s";}
				else if(n.unit == L"h") {f *= 3600; n.unit = L"s";}

				m_num.value = f;
				m_num.unit = n.unit;
				m_num_string = num_string;

				n.value = (T)f;
			}
			else
			{
				n.value = (T)m_num.value;
				n.unit = m_num.unit;
			}

			if(n.sign) 
			{
				n.value *= n.sign;
			}
		}
	}

	void Definition::GetAsString(std::wstring& str)
	{
		if(m_deftype == DefNode) 
		{
			throw Exception(L"expected value type");
		}

		str = m_value; 
	}

	void Definition::GetAsNumber(Number<int>& n, std::map<std::wstring, int>* n2n) 
	{
		GetAsNumber<int>(n, n2n);
	}

	void Definition::GetAsNumber(Number<DWORD>& n, std::map<std::wstring, DWORD>* n2n) 
	{
		GetAsNumber<DWORD>(n, n2n);
	}

	void Definition::GetAsNumber(Number<float>& n, std::map<std::wstring, float>* n2n) 
	{
		GetAsNumber<float>(n, n2n);
	}

	void Definition::GetAsBoolean(bool& b)
	{
		if(m_s2b.empty())
		{
			m_s2b[L"true"] = true;
			m_s2b[L"on"] = true;
			m_s2b[L"yes"] = true;
			m_s2b[L"1"] = true;
			m_s2b[L"false"] = false;
			m_s2b[L"off"] = false;
			m_s2b[L"no"] = false;
			m_s2b[L"0"] = false;
		}

		auto i = m_s2b.find(m_value);

		if(i == m_s2b.end()) // m_deftype != boolean && m_deftype != number || 
		{
			throw Exception(L"expected boolean");
		}

		b = i->second;
	}

	bool Definition::GetAsTime(Time& t, std::map<std::wstring, float>& offset, std::map<std::wstring, float>* n2n, int default_id)
	{
		Definition& time = (*this)[L"time"];

		std::wstring id = time[L"id"].IsValue() ? (std::wstring)time[L"id"] : Util::Format(L"%d", default_id);

		float scale = time[L"scale"].IsValue() ? time[L"scale"] : 1.0f;

		if(time[L"start"].IsValue() && time[L"stop"].IsValue())
		{
			time[L"start"].GetAsNumber(t.start, n2n);
			time[L"stop"].GetAsNumber(t.stop, n2n);

			if(t.start.unit.empty())
			{
				t.start.value *= scale;
			}

			if(t.stop.unit.empty())
			{
				t.stop.value *= scale;
			}

			float o = 0;

			auto i = offset.find(id);

			if(i != offset.end())
			{
				o = i->second;
			}

			if(t.start.sign != 0)
			{
				t.start.value = o + t.start.value;
			}

			if(t.stop.sign != 0)
			{
				t.stop.value = t.start.value + t.stop.value;
			}

			offset[id] = t.stop.value;

			return true;
		}

		return false;
	}

	Definition::operator LPCWSTR()
	{
		if(m_deftype == DefNode) 
		{
			throw Exception(L"expected value type");
		}

		return m_value.c_str();
	}

	Definition::operator float()
	{
		float d;

		GetAsNumber(d);

		return d;
	}

	Definition::operator bool()
	{
		bool b;

		GetAsBoolean(b);

		return b;
	}

	Definition* Definition::SetChildAsValue(LPCWSTR path, DefType t, LPCWSTR v, LPCWSTR u)
	{
		Definition* def = this;

		Split split('.', path);

		for(int i = 0, j = split.size() - 1; i <= j; i++)
		{
			const std::wstring& type = split[i];

			if(def->m_nodes.empty() || dynamic_cast<Reference*>(def->m_nodes.back()) == NULL)
			{
				m_pnf->CreateRef(def);
			}

			Reference* ref = dynamic_cast<Reference*>(def->m_nodes.back());

			if(ref == NULL)
			{
				continue;
			}

			bool last = i == j;

			def = NULL;

			for(auto i = ref->m_nodes.rbegin(); i != ref->m_nodes.rend(); i++)
			{
				Definition* child = dynamic_cast<Definition*>(*i);

				if(child->IsType(type.c_str()))
				{
					if(child->IsNameUnknown()) 
					{
						def = child;
					}

					break;
				}
			}

			if(def == NULL)
			{
				def = m_pnf->CreateDef(ref, type.c_str());
			}

			if(last)
			{
				def->SetAsValue(t, v, u);

				return def;
			}
		}

		return NULL;
	}

	Definition* Definition::SetChildAsNumber(LPCWSTR path, LPCWSTR v, LPCWSTR u)
	{
		Definition* def = SetChildAsValue(path, DefNumber, v, u);

		Number<float> n;

		def->GetAsNumber(n); // verification

		return def;
	}

	Definition* Definition::SetChild(LPCWSTR path, float v)
	{
		wchar_t buff[32];

		swprintf(buff, L"%f", v);

		return SetChildAsNumber(path, buff);
	}

	Definition* Definition::SetChild(LPCWSTR path, LPCWSTR v)
	{
		return SetChildAsValue(path, DefString, v);
	}

	Definition* Definition::SetChild(LPCWSTR path, bool v)
	{
		return SetChildAsValue(path, DefBoolean, v ? L"true" : L"false");
	}

	void Definition::Dump(OutputStream& s, int level, bool last)
	{
		if(m_predefined) return;

		std::wstring tabs(level * 4, ' ');

		std::wstring str = tabs;

		if(m_predefined) 
		{
			str += '?';
		}
		
		if(m_priority == PLow) 
		{
			str += '*';
		}
		else if(m_priority == PHigh) 
		{
			str += '!';
		}

		if(!IsTypeUnknown() && !m_autotype) 
		{
			str += m_type;
		}

		if(!IsNameUnknown()) 
		{
			str += L"#" + m_name;
		}

		str += ':';

		s.PutString(L"%s", str.c_str());

		if(!m_nodes.empty())
		{
			for(auto i = m_nodes.begin(); i != m_nodes.end(); i++)
			{
				Node* n = *i;

				if(Reference* ref = dynamic_cast<Reference*>(n))
				{
					ref->Dump(s, level, last);
				}
				else 
				{
					s.PutString(L" %s", n->m_name.c_str());
				}
			}

			s.PutString(L";\n");

			if(!last && (!m_nodes.empty() || level == 0)) 
			{
				s.PutString(L"\n");
			}
		}
		else if(m_deftype == DefString)
		{
			std::wstring str = m_value;
			Util::Replace(str, L"\"", L"\\\"");
			s.PutString(L" \"%s\";\n", str.c_str());
		}
		else if(m_deftype == DefNumber)
		{
			std::wstring str = m_value;
			if(!m_unit.empty()) str += m_unit;
			s.PutString(L" %s;\n", str.c_str());
		}
		else if(m_deftype == DefBoolean)
		{
			s.PutString(L" %s;\n", m_value.c_str());
		}
		else if(m_deftype == DefBlock)
		{
			s.PutString(L" {%s};\n", m_value.c_str());
		}
		else
		{
			s.PutString(L" null;\n");
		}
	}
}
