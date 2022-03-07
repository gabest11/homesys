#pragma once

#include "DVBTuner.h"

class DVBCPresetParam : public DVBPresetParam
{
public:
	DVBCPresetParam()
	{
		CarrierFrequency = BDA_FREQUENCY_NOT_SET;
		InnerFEC = BDA_FEC_VITERBI;
		InnerFECRate = BDA_BCC_RATE_2_3;
		OuterFEC = BDA_FEC_RS_204_188;
		Modulation = BDA_MOD_64QAM;
		SymbolRate = 6900;
	}
};

class DVBCTuner : public DVBTuner
{
public:
	DVBCTuner();
	virtual ~DVBCTuner();
};
