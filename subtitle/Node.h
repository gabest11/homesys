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

#pragma once

#include "Stream.h"

namespace SSF
{
	class Definition;
	class NodeFactory;

	enum NodePriority {PLow, PNormal, PHigh};

	class Node
	{
	protected:
		NodeFactory* m_pnf;

	public:
		Node* m_parent;
		std::list<Node*> m_nodes;
		std::unordered_map<std::wstring, Node*> m_name2node;
		std::wstring m_type;
		std::wstring m_name;
		NodePriority m_priority;
		bool m_predefined;

		Node(NodeFactory* pnf, LPCWSTR name);
		virtual ~Node() {}

		bool IsNameUnknown();
		bool IsTypeUnknown();
		bool IsType(LPCWSTR type);

		virtual void Add(Node* node);
		virtual void GetChildDefs(std::list<Definition*>& l, LPCWSTR type = NULL, bool first = true);
		virtual void Dump(OutputStream& s, int level = 0, bool last = false) = 0;
	};

	class Reference : public Node
	{
	public:
		Reference(NodeFactory* pnf, LPCWSTR name);
		virtual ~Reference();

		void GetChildDefs(std::list<Definition*>& l, LPCWSTR type = NULL, bool first = true);
		void Dump(OutputStream& s, int level = 0, bool last = false);
	};

	class Definition : public Node
	{
	public:
		template<typename T> struct Number 
		{
			T value; 
			int sign; 
			std::wstring unit;
		};

		struct Time 
		{
			Number<float> start;
			Number<float> stop;
		};

		enum DefType
		{
			DefNode, 
			DefString, 
			DefNumber, 
			DefBoolean, 
			DefBlock
		};

	private:
		DefType m_deftype;
		bool m_autotype;
		std::wstring m_value;
		std::wstring m_unit;
		Number<float> m_num;
		std::wstring m_num_string;
		std::map<std::wstring, Definition*> m_type2def;
		std::map<std::wstring, bool> m_s2b;

		void RemoveFromCache(LPCWSTR type = NULL);

		template<typename T> void GetAsNumber(Number<T>& n, std::map<std::wstring, T>* n2n = NULL);

	public:
		Definition(NodeFactory* pnf, LPCWSTR name);
		virtual ~Definition();

		bool IsVisible(Definition* def);

		void Add(Node* node);
		void Dump(OutputStream& s, int level = 0, bool last = false);

		Definition& operator[] (LPCWSTR type);

		bool IsValue(DefType t = (DefType)0);
		bool HasValue(LPCWSTR type);

		void SetAsValue(DefType t, LPCWSTR v, LPCWSTR u = L"");
		void SetAsNumber(LPCWSTR v, LPCWSTR u = L"");

		void GetAsString(std::wstring& str);
		void GetAsNumber(Number<int>& n, std::map<std::wstring, int>* n2n = NULL);
		void GetAsNumber(Number<DWORD>& n, std::map<std::wstring, DWORD>* n2n = NULL);
		void GetAsNumber(Number<float>& n, std::map<std::wstring, float>* n2n = NULL);
		void GetAsBoolean(bool& b);
		bool GetAsTime(Time& t, std::map<std::wstring, float>& offset, std::map<std::wstring, float>* n2n = NULL, int default_id = 0);
		
		template<typename T> void GetAsNumber(T& t, std::map<std::wstring, T>* n2n = NULL) 
		{
			Number<T> n; 
			GetAsNumber(n, n2n); 
			t = n.value;
		}

		operator LPCWSTR();
		operator float();
		operator bool();

		Definition* SetChildAsValue(LPCWSTR path, DefType t, LPCWSTR v, LPCWSTR u = L"");
		Definition* SetChildAsNumber(LPCWSTR path, LPCWSTR v, LPCWSTR u = L"");
		Definition* SetChild(LPCWSTR path, float v);
		Definition* SetChild(LPCWSTR path, LPCWSTR v);
		Definition* SetChild(LPCWSTR path, bool v);
	};
}
