#pragma once

#include "../3rdparty/libdvbcsa/dvbcsa.h"
#include "../3rdparty/ffdecsa/ffdecsa.h"

class CSA
{
	dvbcsa_key_s* m_even;
	dvbcsa_key_s* m_odd;
	csa_context_t* m_ctx;

public:
	CSA();
	virtual ~CSA();

	void SetKey(const BYTE* cws);
	void Decrypt(BYTE* buff, int count);
};
