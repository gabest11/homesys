#pragma once

#include "stdafx.h"
#include "DVBTTuner.h"

DVBTTuner::DVBTTuner()
{
	m_dvb.type = DVB_Terrestrial;
	m_dvb.networkprovider = CLSID_DVBTNetworkProvider;
	m_dvb.tuningspace = __uuidof(DVBTuningSpace);
	m_dvb.locator = __uuidof(DVBTLocator);
}

DVBTTuner::~DVBTTuner()
{
}
