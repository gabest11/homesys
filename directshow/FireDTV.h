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

// firedtv/firesat

[uuid("AB132414-D060-11D0-8583-00C04FD9BAF3")]
class Firesat {};

// static const GUID KSPROPSETID_Firesat = {0xab132414, 0xd060, 0x11d0, {0x85, 0x83, 0x00, 0xc0, 0x4f, 0xd9, 0xba, 0xf3}};

#define FIRESAT_LNB_CONTROL 12
#define FIRESAT_HOST2CA 22
#define FIRESAT_TEST_INTERFACE 29

struct FIRESAT_CA_DATA
{
	UCHAR uSlot;
	UCHAR uTag;
	BOOL bMore; // don’t care; set by driver
	USHORT uLength;
	UCHAR uData[1024];
};

struct FIRESAT_LNB_CMD
{
	UCHAR Voltage; // 0xFF...Don´t Care
	UCHAR ContTone; // 0xFF...Don´t Care
	UCHAR Burst; // 0xFF...Don´t Care
	UCHAR NrDiseqcCmds; // 1 - 3

	struct
	{
		UCHAR Length; // 3 - 6
		UCHAR Framing;
		UCHAR Address;
		UCHAR Command;
		UCHAR Data[3];
	} DiseqcCmd[3];
};
