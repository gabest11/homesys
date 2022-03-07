#pragma once

#include "DVBTuner.h"

class DVBTPresetParam : public DVBPresetParam
{
public:
	// IDVBTLocator

	long Bandwidth;
	FECMethod LPInnerFEC;
	BinaryConvolutionCodeRate LPInnerFECRate;
	HierarchyAlpha HAlpha;
	GuardInterval Guard;
	TransmissionMode Mode;
	VARIANT_BOOL OtherFrequencyInUse;

	DVBTPresetParam()
	{
		InnerFEC = BDA_FEC_VITERBI;
		InnerFECRate = BDA_BCC_RATE_3_4;
		OuterFEC = BDA_FEC_RS_204_188; 
		Modulation = BDA_MOD_64QAM;

		Bandwidth = 8;
		LPInnerFEC = BDA_FEC_METHOD_NOT_SET;
		LPInnerFECRate = BDA_BCC_RATE_3_4;
		HAlpha = BDA_HALPHA_NOT_SET;
		Guard = BDA_GUARD_1_8;
		Mode = BDA_XMIT_MODE_8K;
		OtherFrequencyInUse = VARIANT_FALSE;
	}

	std::wstring ToString() const
	{
		return __super::ToString() + Util::Format(
			L", Bandwidth=%d, LPInnerFEC=%d, LPInnerFECRate=%d, HAlpha=%d, Guard=%d, Mode=%d, OtherFrequencyInUse=%d",
			Bandwidth, LPInnerFEC, LPInnerFECRate, HAlpha, Guard, Mode, OtherFrequencyInUse
			);
	}
};

class DVBTTuner : public DVBTuner
{
public:
	DVBTTuner();
	virtual ~DVBTTuner();
};
