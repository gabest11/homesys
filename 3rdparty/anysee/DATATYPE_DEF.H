#ifndef		__DATATYPE_DEF_H__
#define		__DATATYPE_DEF_H__
/* -----------------------------------------------------------------------------
	ｵ･ﾀﾌﾅﾍ ﾁｾｷ・ﾁ､ﾀﾇ ﾆﾄﾀﾏ
	ｸｸｵ鄲ﾌ: [K.Y.T] Airfox
	ｸｸｵ邉ｯ: 2005.03.22
   ----------------------------------------------------------------------------- */

#ifndef	PVOID
	typedef void *PVOID;
#endif

#ifndef	PCHAR
	typedef char *PCHAR;
#endif
	
#ifndef	PUCHAR
	typedef unsigned char *PUCHAR;
#endif

#ifndef	UCHAR
	typedef unsigned char UCHAR;
#endif

#ifndef	BYTE
	typedef unsigned char BYTE;
#endif

#ifndef	PBYTE
	typedef BYTE *PBYTE;
#endif

#ifndef	WORD
	typedef unsigned short int WORD;
#endif

#ifndef	PWORD
	typedef WORD *PWORD;
#endif

#ifndef	DWORD
	typedef unsigned long int DWORD;
#endif

#ifndef	PDWORD
	typedef DWORD *PDWORD;
#endif

#ifndef	QWORD
	typedef unsigned __int64 QWORD;
#endif

#ifndef	PQWORD
	typedef QWORD *PQWORD;
#endif

#ifndef	ULONGLONG
	#define	ULONGLONG	QWORD
#endif

#ifndef	LONG
	typedef long	LONG;
#endif

#ifndef	ULONG
	#define	ULONG	DWORD
#endif

#ifndef	HANDLE
	typedef void *HANDLE;
#endif

#ifndef	BOOLEAN
	typedef UCHAR BOOLEAN;
#endif

#ifndef	KB
	#define	KB	1024
#endif
#ifndef	MB
	#define	MB	(KB*KB)
#endif
#ifndef	GB
	#define	GB	((QWORD)MB*KB)
#endif
#ifndef	TB
	#define	TB	(GB*KB)
#endif

#ifndef	KHZ
	#define	KHZ	1000
#endif
#ifndef	MHZ
	#define	MHZ	1000000
#endif
#ifndef	GHZ
	#define	GHZ	1000000000
#endif
#ifndef	THZ
	#define	THZ	1000000000000
#endif

#ifndef	_27MHZ
	#define	_27MHZ	27000000
#endif

#define FIELD_TYPE_UNSIGNED 0
#define FIELD_TYPE_SIGNED   1

//   ｺｮ ﾁ､ﾀﾇｹｮ
#define BIT0	0x01
#define BIT1	0x02
#define BIT2	0x04
#define BIT3	0x08
#define BIT4	0x10
#define BIT5	0x20
#define BIT6	0x40
#define BIT7	0x80 
#define BIT8	0x0100
#define BIT9	0x0200
#define BIT10	0x0400
#define BIT11	0x0800
#define BIT12	0x1000
#define BIT13	0x2000
#define BIT14	0x4000
#define BIT15	0x8000 
#define BIT16	0x00010000 
#define BIT17	0x00020000 
#define BIT18	0x00040000 
#define BIT19	0x00080000 
#define BIT20	0x00100000 
#define BIT21	0x00200000 
#define BIT22	0x00400000 
#define BIT23	0x00800000 
#define BIT24	0x01000000 
#define BIT25	0x02000000 
#define BIT26	0x04000000 
#define BIT27	0x08000000 
#define BIT28	0x10000000 
#define BIT29	0x20000000 
#define BIT30	0x40000000 
#define BIT31	0x80000000 

#define	AMT_TRUE		1
#define	AMT_FALSE		0

#endif // __DATATYPE_DEF_H__
