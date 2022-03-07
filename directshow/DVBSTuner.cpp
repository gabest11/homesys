#pragma once

#include "stdafx.h"
#include "DVBSTuner.h"
#include "FireDTV.h"

DVBSTuner::DVBSTuner()
	: m_LNBSource(BDA_LNB_SOURCE_NOT_SET)
{
	m_dvb.type = DVB_Satellite;
	m_dvb.networkprovider = CLSID_DVBSNetworkProvider;
	m_dvb.tuningspace = __uuidof(DVBSTuningSpace);
	m_dvb.locator = __uuidof(DVBSLocator);
}

DVBSTuner::~DVBSTuner()
{
}

HRESULT DVBSTuner::TuneDVB(const DVBPresetParam* p)
{
	if(0)
	if(const DVBSPresetParam* ps = dynamic_cast<const DVBSPresetParam*>(p))
	{
		// if(m_LNBSource != pp->LNBSource)
		{
			if(ps->LNBSource >= BDA_LNB_SOURCE_A && ps->LNBSource <= BDA_LNB_SOURCE_D)
			{
				/*
				if(CComQIPtr<IDVBSLocator2> pDVBSLocator2 = pLocator)
				{
					// IDVBSLocator2

					if(FAILED(pDVBSLocator2->put_DiseqLNBSource(pp->LNBSource)))
						return false;
				}
				*/

				for(int i = 0; i < 2; i++)
				{
					int N = 0xF0 | ((ps->LNBSource - 1) << 2);
					// int N = 0xC0 | ((ps->LNBSource - 1) << 2);
					// int N = 0xF0 | (ps->LNBSource - 1);

					if(m_anysee != NULL)
					{
						/*
						BYTE cmd1[] = {0xE0, 0x10, 0x30};

						m_anysee->SendDiSEqCData(cmd1, sizeof(cmd1));
						*/
						/**/
						BYTE cmd2[] = {0xE0, 0x10, 0x38, N};

						m_anysee->SendDiSEqCData(cmd2, sizeof(cmd2));
						
						m_anysee->SelectLNB(N);
					}
					else
					{
						FIRESAT_LNB_CMD instance;
						FIRESAT_LNB_CMD cmd;

						memset(&instance, 0, sizeof(instance));
						memset(&cmd, 0, sizeof(cmd));

						cmd.Voltage = 0xFF;
						cmd.ContTone = 0xFF;
						cmd.Burst = 0xFF;
						cmd.NrDiseqcCmds = 1;
						cmd.DiseqcCmd[0].Length = 4;
						cmd.DiseqcCmd[0].Framing = 0xE0;
						cmd.DiseqcCmd[0].Address = 0x10;
						cmd.DiseqcCmd[0].Command = 0x38;
						cmd.DiseqcCmd[0].Data[0] = N;

						// FIXME: m_ksps->Set(__uuidof(Firesat), FIRESAT_LNB_CONTROL, &instance, sizeof(instance), &cmd, sizeof(cmd));
					}

					Sleep(10);
				}
			}

			m_LNBSource = ps->LNBSource;
		}
	}

	return __super::TuneDVB(p);
}