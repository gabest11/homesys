#pragma once

#include "stdafx.h"
#include "DVBCTuner.h"

DVBCTuner::DVBCTuner()
{
	m_dvb.type = DVB_Cable;
	m_dvb.networkprovider = CLSID_DVBCNetworkProvider;
	m_dvb.tuningspace = __uuidof(DVBTuningSpace);
	m_dvb.locator = __uuidof(DVBCLocator);
}

DVBCTuner::~DVBCTuner()
{
}
