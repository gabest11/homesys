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
#include "Exception.h"

namespace SSF
{
	Exception::Exception(LPCWSTR fmt, ...) 
	{
		va_list args;

		va_start(args, fmt);

		WCHAR* buff = NULL;

		int len = _vscwprintf(fmt, args) + 1;

		if(len > 0)
		{
			buff = new WCHAR[len];

			vswprintf_s(buff, len, fmt, args);

			m_msg = std::wstring(buff, len - 1);
		}

		va_end(args);

		delete [] buff;
	}
}