#pragma once

#define DSMF_VERSION 0x02

#define DSMSW 0x44534D53ui64
#define DSMSW_SIZE 4

enum dsmp_t
{
	DSMP_FILEINFO = 0,
	DSMP_STREAMINFO = 1,
	DSMP_MEDIATYPE = 2,
	DSMP_CHAPTERS = 3,
	DSMP_SAMPLE = 4,
	DSMP_SYNCPOINTS = 5,
	DSMP_RESOURCE = 6,
	DSMP_KEY = 7,
};

enum dsmk_t
{
	DSMK_XOR = 0,
	DSMK_XOR_MACHINESPEC,
};

struct DSMKey
{
	BYTE type;
	DWORD id;
	int len;
	BYTE* data;
	BYTE* machinespec;

	DSMKey()
	{
		memset(this, 0, sizeof(*this));

		machinespec = new BYTE[8];

		TCHAR path[MAX_PATH];

		if(GetWindowsDirectory(path, MAX_PATH) > 0)
		{
			WIN32_FILE_ATTRIBUTE_DATA attr;

			GetFileAttributesEx(path, GetFileExInfoStandard, &attr);

			BYTE* key = (BYTE*)&attr.ftCreationTime;

			for(int i = 0; i < 8; i++)
			{
				machinespec[i] = key[7 - i];
			}
		}
	}

	~DSMKey()
	{
		if(data != NULL) delete [] data;
		if(machinespec != NULL) delete [] machinespec;
	}

	void Encode(BYTE* bytes, size_t count)
	{
		if(type == DSMK_XOR || type == DSMK_XOR_MACHINESPEC)
		{
			if(len == 2 || len == 4 || len == 8)
			{
				int mask = len - 1;

				for(size_t i = 0; i < count; i++)
				{
					size_t pos = i & mask;

					BYTE b = data[pos];
					
					if(type == DSMK_XOR_MACHINESPEC)
					{
						b ^= machinespec[pos & 7];
					}

					bytes[i] ^= (BYTE)(b + i + count);
				}
			}
			else
			{
				for(size_t i = 0; i < count; i++)
				{
					size_t pos = i % len;

					BYTE b = data[pos];
					
					if(type == DSMK_XOR_MACHINESPEC)
					{
						b ^= machinespec[pos & 7];
					}

					bytes[i] ^= (BYTE)(b + i + count);
				}
			}
		}
	}

	void Decode(BYTE* bytes, size_t count)
	{
		if(type == DSMK_XOR || type == DSMK_XOR_MACHINESPEC)
		{
			Encode(bytes, count);
		}
	}
};