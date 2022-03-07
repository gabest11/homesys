/*****************************************************************
|
|    AP4 - dref Atoms 
|
|    Copyright 2002 Gilles Boccon-Gibod
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "stdafx.h"
#include "Ap4.h"
#include "Ap4DrefAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4ContainerAtom.h"

/*----------------------------------------------------------------------
|       AP4_DrefAtom::AP4_DrefAtom
+---------------------------------------------------------------------*/
AP4_DrefAtom::AP4_DrefAtom(AP4_Atom** refs, AP4_Cardinal refs_count) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_DREF, AP4_FULL_ATOM_HEADER_SIZE+4, true)
{
    for (unsigned i=0; i<refs_count; i++) {
        m_Children.Add(refs[i]);
        m_Size += refs[i]->GetSize();
    }
}

/*----------------------------------------------------------------------
|       AP4_DrefAtom::AP4_DrefAtom
+---------------------------------------------------------------------*/
AP4_DrefAtom::AP4_DrefAtom(AP4_Size         size,
                           AP4_ByteStream&  stream,
                           AP4_AtomFactory& atom_factory) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_DREF, size, true, stream)
{
    // read the number of entries
    AP4_UI32 entry_count;
    stream.ReadUI32(entry_count);

    // read children
    AP4_Size bytes_available = size-AP4_FULL_ATOM_HEADER_SIZE-4;
    while (entry_count--) {
        AP4_Atom* atom; 
        while (AP4_SUCCEEDED(atom_factory.CreateAtomFromStream(stream, 
                                                               bytes_available,
                                                               atom,
															   this))) {
            m_Children.Add(atom);
        }
    }
}

/*----------------------------------------------------------------------
|       AP4_DrefAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DrefAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    // write the number of entries
    result = stream.WriteUI32(m_Children.ItemCount());
    if (AP4_FAILED(result)) return result;

    // write the children
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|       AP4_RdrfAtom::AP4_RdrfAtom
+---------------------------------------------------------------------*/
AP4_RdrfAtom::AP4_RdrfAtom(AP4_Size         size,
                           AP4_ByteStream&  stream,
                           AP4_AtomFactory& atom_factory) :
    AP4_Atom(AP4_ATOM_TYPE_RDRF, size, true, stream)
{
    AP4_Size bytes_available = size-AP4_FULL_ATOM_HEADER_SIZE;

	while(bytes_available > 8)
	{
		AP4_UI32 type = 0;
		AP4_UI32 size = 0;

		if(AP4_FAILED(stream.ReadUI32(type)) || AP4_FAILED(stream.ReadUI32(size)))
			break;

		bytes_available -= 8;

		if(size > bytes_available) 
			break; 

		AP4_Offset start;

		stream.Tell(start);

		switch(type)
		{
		case AP4_ATOM_TYPE_URL:
			char* str = new char[size + 1];
			stream.Read(str, size);
			str[size] = '\0';
			m_url = str;
			delete [] str;
			break;
		}

		stream.Seek(start + size);

		bytes_available -= size;
	}
}
