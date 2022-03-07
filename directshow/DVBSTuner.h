#pragma once

#include "DVBTuner.h"

class DVBSPresetParam : public DVBPresetParam
{
public:

	// IDVBSLocator

	Polarisation SignalPolarisation;
	VARIANT_BOOL WestPosition;
	long OrbitalPosition;
	long Azimuth;
	long Elevation;

	// IDVBSLocator2

	LNB_Source LNBSource;

	// IDVBSTuningSpace

	long LowOscillator;
	long HighOscillator;
	long LNBSwitch;
	SpectralInversion SpectralInversion;

	DVBSPresetParam()
	{
		InnerFEC = BDA_FEC_VITERBI;
		OuterFEC = BDA_FEC_RS_204_188; 
		Modulation = BDA_MOD_QPSK;

		SignalPolarisation = BDA_POLARISATION_NOT_SET;
		WestPosition = VARIANT_FALSE;
		OrbitalPosition = 0; 
		Azimuth = 0;
		Elevation = 0;

		LNBSource = BDA_LNB_SOURCE_NOT_SET;

		LowOscillator = 9750000;
		HighOscillator = 10600000;
		LNBSwitch = 11700000;
		SpectralInversion = BDA_SPECTRAL_INVERSION_NOT_SET;
	}

	std::wstring ToString() const
	{
		return __super::ToString() + Util::Format(
			L", SignalPolarisation=%d, WestPosition=%d, OrbitalPosition=%d, Azimuth=%d, Elevation=%d, LNBSource=%d, LowOscillator=%d, HighOscillator=%d, LNBSwitch=%d, SpectralInversion=%d",
			SignalPolarisation, WestPosition, OrbitalPosition, Azimuth, Elevation, LNBSource, LowOscillator, HighOscillator, LNBSwitch, SpectralInversion
			);
	}
};

class DVBSTuner : public DVBTuner
{
	LNB_Source m_LNBSource;

protected:
	HRESULT TuneDVB(const DVBPresetParam* p);

public:
	DVBSTuner();
	virtual ~DVBSTuner();
};