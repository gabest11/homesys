// anyseeIR.h: interface for the CanyseeIR class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ANYSEEIR_H__62A5E035_A469_47F7_B6F1_5C2577B9E61A__INCLUDED_)
#define AFX_ANYSEEIR_H__62A5E035_A469_47F7_B6F1_5C2577B9E61A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "anyseeAPI.h"

class CanyseeIR  
{
public:
	int GetBoardInfo(AMTBOARD_INFO &rBoardInfo);
	int SetDevice(DWORD dwNumber = 1);

	int Enable(bool bEnabled);
	int Read(DWORD &rdwKey);

	int Open(DWORD dwNumber, PDEVICE_INFO pDeviceInfo = NULL);
	int Close(DWORD dwNumber = 1);

////////////////////////////////////////////////////////////////////////////////////////

	DWORD Version(void);
	DWORD GetNumberOfDevices(void);

	CanyseeIR();
	virtual ~CanyseeIR();

/////////////////////////////////////////////////////////////////////////////////////////

private:
	PVOID	m_pReserved;
};

#endif // !defined(AFX_ANYSEEIR_H__62A5E035_A469_47F7_B6F1_5C2577B9E61A__INCLUDED_)
