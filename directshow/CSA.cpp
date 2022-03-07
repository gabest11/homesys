#include "stdafx.h"
#include "CSA.h"

CSA::CSA()
{
	//m_even = dvbcsa_key_alloc();
	//m_odd = dvbcsa_key_alloc();
	m_ctx = csa_context_alloc();
}

CSA::~CSA(void)
{
	//dvbcsa_key_free(m_even);
	//dvbcsa_key_free(m_odd);
	csa_context_free(m_ctx);
}

void CSA::SetKey(const BYTE* cws)
{
	//dvbcsa_key_set(cws + 0, m_even);
	//dvbcsa_key_set(cws + 8, m_odd);

	set_control_word(cws + 0, m_ctx->keys[0]);
	set_control_word(cws + 8, m_ctx->keys[1]);
}

void CSA::Decrypt(BYTE* buff, int count)
{
	if(1)
	{
		unsigned char* cluster[3] = {buff, buff + 188 * count, NULL};

		count -= decrypt_packets(m_ctx, &cluster[0]);

		if(cluster[0] != NULL)
		{
			count -= decrypt_packets(m_ctx, &cluster[0]);

			ASSERT(cluster[0] == NULL);
		}

		ASSERT(count == 0);

		if(count > 0)
		{
			printf("CSA %d\n", count);
		}
	}
	else
	{
		for(int i = 0; i < count; i++, buff += 188)
		{
			if(buff[3] & 0x80)
			{
				int offset = 4;

				if(buff[3] & 0x20)
				{
					offset += (buff[4] + 1); // skip adaption field
				}

				dvbcsa_decrypt((buff[3] & 0x40) ? m_odd : m_even, &buff[offset], 188 - offset);

				buff[3] &= 0x3f;
			}
		}
	}
}
