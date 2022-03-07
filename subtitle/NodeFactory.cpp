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
#include "NodeFactory.h"
#include "Exception.h"
#include "Split.h"
#include "../util/String.h"

namespace SSF
{
	NodeFactory::NodeFactory()
		: m_counter(0)
		, m_predefined(false)
	{
		m_root = CreateRef(NULL);
	}

	NodeFactory::~NodeFactory()
	{
		for(auto i = m_nodes.begin(); i != m_nodes.end(); i++)
		{
			delete i->second;
		}
	}

	std::wstring NodeFactory::GenName()
	{
		wchar_t buff[32];

		return std::wstring(buff, swprintf(buff, L"%I64d", m_counter++));
	}

	void NodeFactory::Reset()
	{
		for(auto i = m_nodes.begin(); i != m_nodes.end(); i++)
		{
			delete i->second;
		}
		
		m_nodes.clear();
		m_newnodes.clear();

		m_root = CreateRef(NULL);
	}

	void NodeFactory::Commit()
	{
		m_newnodes.clear();
	}

	void NodeFactory::Rollback()
	{
		for(auto i = m_newnodes.rbegin(); i != m_newnodes.rend(); i++)
		{
			auto j = m_nodes.find(*i);

			if(j != m_nodes.end())
			{
				delete j->second; // TODO: remove it from "parent"->m_nodes too

				m_nodes.erase(j);
			}
		}
	}

	Reference* NodeFactory::GetRootRef() const
	{
		return m_root;
	}

	Reference* NodeFactory::CreateRef(Definition* parent)
	{
		std::wstring name = GenName();

		Reference* ref = new Reference(this, name.c_str());

		m_nodes[name] = ref;

		m_newnodes.push_back(name);

		if(parent != NULL)
		{
			parent->Add(ref);

			ref->m_parent = parent;
		}

		return ref;
	}

	Definition* NodeFactory::CreateDef(Reference* parent, LPCWSTR type, LPCWSTR name, NodePriority priority)
	{
		Definition* def = NULL;

		std::wstring str = name;

		if(!str.empty())
		{
			def = GetDefByName(name);

			if(def != NULL)
			{
				if(!def->m_predefined)
				{
					throw Exception(L"redefinition of '%s' is not allowed", name);
				}

				if(!def->IsTypeUnknown() && !def->IsType(type))
				{
					throw Exception(L"cannot redefine type of %s to %s", name, type);
				}
			}
		}
		else
		{
			str = GenName();
		}

		if(def == NULL)
		{
			def = new Definition(this, str.c_str());

			m_nodes[str] = def;
			
			m_newnodes.push_back(str);

			if(parent != NULL)
			{
				parent->Add(def);
				
				def->m_parent = parent;
			}
		}

		def->m_type = type;
		def->m_priority = priority;
		def->m_predefined = m_predefined;

		return def;
	}

	Definition* NodeFactory::GetDefByName(LPCWSTR name) const
	{
		auto i = m_nodes.find(name);

		if(i != m_nodes.end())
		{
			return dynamic_cast<Definition*>(i->second);
		}

		return NULL;
	}

	void NodeFactory::GetNewDefs(std::list<Definition*>& defs)
	{
		defs.clear();

		for(auto i = m_newnodes.begin(); i != m_newnodes.end(); i++)
		{
			Definition* def = GetDefByName(i->c_str());

			if(def != NULL)
			{
				defs.push_back(def);
			}
		}
	}

	void NodeFactory::Dump(OutputStream& s) const
	{
		if(m_root != NULL)
		{
			for(auto i = m_root->m_nodes.begin(); i != m_root->m_nodes.end(); i++)
			{
				(*i)->Dump(s);
			}
		}
	}
}
