#pragma once

#include "stdafx.h"
#include "DVBTuner.h"
#include "DVBTTuner.h"
#include "DVBCTuner.h"
#include "DVBSTuner.h"
#include "TSWriter.h"
#include "FakeTIF.h"
#include "TeletextFilter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "../directshow/moreuuids.h"

#include "FireDTV.h"

#define TTBDADRVAPI_STATIC_LIBRARY

#include "../3rdparty/technotrend/ttBdaDrvApi.h"

namespace Realtek
{
	[uuid("BB992B31-931D-41B1-85EA-A01B4E306CA5")]
	class KSPROPSETID_ModeProperties {};

	enum 
	{
		DemodSupportModeProperty = 2,
		SetDemodModeProperty = 3,
	};

	[uuid("F25C6BC9-DBD7-4A0C-B228-9A58789F2E1D")]
	class KSPROPSETID_ControlProperties {};

	enum
	{
		ModulationProperty = 10,
		SymbolRateProperty = 11,
	};

	enum
	{
		SUPPORT_DVBT = 0x01,
		SUPPORT_DTMB = 0x02,
		SUPPORT_FM = 0x04,
		SUPPORT_DAB_ISDB1SEG = 0x08,
		SUPPORT_DVBH = 0x10,
		SUPPORT_DVBC = 0x20,
		SUPPORT_ATSC = 0x40,
		SUPPORT_ATSCMH = 0x80,
		SUPPORT_ISDBTFSEG = 0x100,
		SUPPORT_ISDBTSB = 0x200,
		SUPPORT_DVBS = 0x400,
		SUPPORT_CMMB = 0x800,
		SUPPORT_ONLY_ISDT1SEG = 0x1000,
		SUPPORT_ONLY_DAB = 0x2000,
	};

	enum
	{
		DVBT = 0,
		DTMB,
		DVBC,
		ATSC,
		OPEN_CAB,
		ISDBT_FSEG
	};
}

DVBTuner::DVBTuner()
	: m_sid(-1)
	, m_freq(0)
	, m_anysee(NULL)
	, m_technotrend(NULL)
	, m_dummy(false)
{
}

DVBTuner::~DVBTuner()
{
	Close();
}

void DVBTuner::Close()
{
	if(m_anysee != NULL)
	{
		delete m_anysee; 

		m_anysee = NULL;
	}

	if(m_technotrend != NULL)
	{
		delete m_technotrend; 

		m_technotrend = NULL;
	}

	m_realtek = NULL;
	m_firesat = NULL;
	m_wintvci = NULL;
	m_geniaci = NULL;

	m_source = NULL;
	m_writer = NULL;
	m_demux = NULL;
	m_tuner = NULL;
	m_networkprovider = NULL;

	__super::Close();
}

bool DVBTuner::Record(LPCWSTR path, int preset, int timeout)
{
	if(preset >= 0)
	{
		return __super::Record(path, preset, timeout);
	}

	Stop();

	HRESULT hr = S_OK;

	ForeachInterface<ITSDemuxFilter>(m_dst, [&] (IBaseFilter* pBF, ITSDemuxFilter* demux) -> HRESULT
	{
		demux->SetWriterOutput(preset == -2 ? ITSDemuxFilter::WriteFullTS : ITSDemuxFilter::WriteProgram); 

		return S_CONTINUE;
	});

	CComQIPtr<ITSWriterFilter>(m_writer)->SetOutput(path);

	Run();

	m_start = clock();
	m_timeout = timeout;

	m_state = TunerState::Recording;

	return SUCCEEDED(hr);
}

bool DVBTuner::Timeshift(LPCWSTR path, LPCWSTR id, const GenericPresetParam* p)
{
	const DVBPresetParam* pp = dynamic_cast<const DVBPresetParam*>(p);

	if(pp == NULL)
	{
		return false;
	}

	Stop();

	if(m_dst == NULL || m_demux == NULL)
	{
		return false;
	}

	int freq = pp->CarrierFrequency / 1000;

	CComQIPtr<ITSDemuxFilter> demux = m_demux;

	if(FAILED(demux->HasProgram(pp->sid, freq)))
	{
		TuneDVB(pp);

		WaitSignal();

		demux->SetFreq(freq);

		CComQIPtr<ITSSourceFilter>(m_source)->SaveInput();

		CComQIPtr<IMediaControl> mc = m_dst;

		mc->Run();

		std::list<TSDemuxProgram*> programs;

		demux->GetPrograms(programs, pp->sid, false);

		mc->Stop();
	}

	m_sid = pp->sid;
	m_freq = pp->CarrierFrequency;

	HRESULT hr = demux->SetProgram(pp->sid, true);
	
	m_dummy = hr == S_FALSE;

	if(!RenderMDSM(path, id))
	{
		return false;
	}

	Run();

	m_state = TunerState::Timeshifting;

	return true;
}

bool DVBTuner::StartRecording(LPCWSTR path)
{
	std::wstring s = Util::MakeLower(path);

	std::wstring::size_type i = s.rfind(L".ts");

	if(i != std::wstring::npos && i == s.size() - 3)
	{
		return CComQIPtr<ITSWriterFilter>(m_writer)->SetOutput(path) == S_OK;
	}
	
	return __super::StartRecording(path);
}

void DVBTuner::StopRecording()
{
	CComQIPtr<ITSWriterFilter>(m_writer)->SetOutput(NULL);

	__super::StopRecording();
}

void DVBTuner::Stop()
{
	__super::Stop();

	if(m_dst != NULL)
	{
		Foreach(m_demux, PINDIR_OUTPUT, [this] (IPin* pin) -> HRESULT
		{
			if(DirectShow::GetCLSID(DirectShow::GetConnected(pin)) != __uuidof(CTSWriterFilter))
			{
				m_dst->NukeDownstream(pin);
			}

			return S_CONTINUE;
		});

		m_dst->Clean();
	}
}

int DVBTuner::GetFreq()
{
	return m_freq;
}

void DVBTuner::GetSignal(TunerSignal& signal)
{
	signal.Reset();

	HRESULT hr;

	if(CComQIPtr<IBDA_Topology> topology = m_tuner)
	{
		ULONG nc = 0;
		ULONG nt[32];

		hr = topology->GetNodeTypes(&nc, 32, nt);

		for(ULONG i = 0; i < nc; i++)
		{
			CComPtr<IUnknown> unk;

			hr = topology->GetControlNode(0, 1, nt[i], &unk);

			if(CComQIPtr<IBDA_SignalStatistics> stats = unk)
			{
				BOOLEAN locked = 0;

				if(SUCCEEDED(stats->get_SignalLocked(&locked)))
				{
					signal.locked = locked ? 1 : 0;
				}

				BOOLEAN present = 0;

				if(SUCCEEDED(stats->get_SignalPresent(&present)))
				{
					signal.present = present ? 1 : 0;
				}

				LONG quality = 0;

				if(SUCCEEDED(stats->get_SignalQuality(&quality)))
				{
					signal.quality = (int)quality;
				}

				LONG strength = 0;

				if(SUCCEEDED(stats->get_SignalStrength(&strength)))
				{
					signal.strength = (int)strength;
				}

				break;
			}
		}
	}
}

__int64 DVBTuner::GetReceivedBytes()
{
	CComQIPtr<ITSDemuxFilter> demux = m_demux;

	if(demux != NULL)
	{
		return demux->GetReceivedBytes();
	}

	return 0;
}

bool DVBTuner::IsScrambled()
{
	CComQIPtr<ITSDemuxFilter> demux = m_demux;

	if(demux != NULL)
	{
		return demux->IsScrambled(m_sid) == S_OK;
	}

	return false;
}

void DVBTuner::GetConnectorTypes(std::list<PhysicalConnectorType>& types)
{
	types.clear();
	types.push_back(PhysConn_Video_Tuner);
}

bool DVBTuner::GetOutput(std::list<IPin*>& video, std::list<IPin*>& audio, IPin*& vbi)
{
	Foreach(m_demux, PINDIR_OUTPUT, [&] (IPin* pin) -> HRESULT
	{
		Foreach(pin, [&] (AM_MEDIA_TYPE* pmt) -> HRESULT
		{
			if(pmt->majortype == MEDIATYPE_Video)
			{
				video.push_back(pin);

				return S_OK;
			}
			else if(pmt->majortype == MEDIATYPE_Audio)
			{
				audio.push_back(pin);

				return S_OK;
			}
			else if(pmt->majortype == MEDIATYPE_TeletextPacket)
			{
				HRESULT hr;

				CComPtr<IBaseFilter> pBF = new CTeletextFilter(NULL, &hr);

				hr = m_dst->AddFilter(pBF, L"Teletext Filter");

				if(SUCCEEDED(hr)) 
				{
					hr = m_dst->ConnectFilterDirect(pin, pBF, NULL);

					if(SUCCEEDED(hr))
					{
						vbi = DirectShow::GetFirstPin(pBF, PINDIR_OUTPUT);

						return S_OK;
					}
					else
					{
						m_dst->RemoveFilter(pBF);
					}
				}
			}

			return S_CONTINUE;
		});

		return S_CONTINUE;
	});

	return !video.empty() || !audio.empty();
}

bool DVBTuner::CreateNetworkProvider(IBaseFilter** ppNP)
{
	HRESULT hr;

	CComPtr<IBaseFilter> np;

	hr = np.CoCreateInstance(m_dvb.networkprovider);

	if(FAILED(hr))
	{
		return false;
	}

	*ppNP = np.Detach();

	return true;
}

bool DVBTuner::GetTuningSpace(ITuningSpace** ppTS)
{
	CComPtr<IDVBTuningSpace> ts;

	if(FAILED(ts.CoCreateInstance(m_dvb.tuningspace)))
	{
		return false;
	}

	if(FAILED(ts->put__NetworkType(m_dvb.networkprovider)))
	{
		return false;
	}

	if(FAILED(ts->put_SystemType(m_dvb.type)))
	{
		return false;
	}

	CComPtr<ILocator> locator;

	if(FAILED(locator.CoCreateInstance(m_dvb.locator)))
	{
		return false;
	}

	if(FAILED(ts->put_DefaultLocator(locator)))
	{
		return false;
	}

	*ppTS = ts.Detach();

	return true;
}

[uuid("DD5A9B44-348A-4607-BF72-CFD8239E4432")]
interface IUSB2CI : public IUnknown
{
	STDMETHOD(Init)(DWORD_PTR p0) = 0;
	STDMETHOD(F1)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
	STDMETHOD(F2)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
	STDMETHOD(SendPMT)(const BYTE* pmt, int size) = 0;
	STDMETHOD(F4)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
	STDMETHOD(F5)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
	STDMETHOD(F6)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
	STDMETHOD(F7)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
	STDMETHOD(F8)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
	STDMETHOD(F9)(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5) = 0;
};

[uuid("885E2C2B-33CE-45E8-891D-BEEB5BF36768")]
class CUSB2CI 
	: public CBaseFilter
	, public CCritSec
	, public IUSB2CI
{
	CComPtr<IBaseFilter> m_pInner;

public:
	CUSB2CI(IBaseFilter* pInner)
		: CBaseFilter(NAME("CUSB2CI"), NULL, this, __uuidof(this))
		, m_pInner(pInner)
	{
	}

	virtual ~CUSB2CI()
	{
	}

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return
			QI(IUSB2CI)
			 __super::NonDelegatingQueryInterface(riid, ppv);
	}

	int GetPinCount() {return 0;}
	CBasePin* GetPin(int n) {return NULL;}

	// IUSB2CI

	STDMETHODIMP Init(DWORD_PTR p0)
	{
		CComQIPtr<IUSB2CI> pUSB2CI(m_pInner);

		HRESULT hr = pUSB2CI->Init(p0);

		return hr;
	}

	STDMETHODIMP F1(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}

	STDMETHODIMP F2(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}

	STDMETHODIMP SendPMT(const BYTE* pmt, int size)
	{
		CComQIPtr<IUSB2CI> pUSB2CI(m_pInner);

		HRESULT hr = pUSB2CI->SendPMT(pmt, size);

		return hr;
	}

	STDMETHODIMP F4(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}

	STDMETHODIMP F5(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}

	STDMETHODIMP F6(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}

	STDMETHODIMP F7(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}

	STDMETHODIMP F8(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}

	STDMETHODIMP F9(DWORD_PTR p0, DWORD_PTR p1, DWORD_PTR p2, DWORD_PTR p3, DWORD_PTR p4, DWORD_PTR p5)
	{
		return S_OK;
	}
};

bool DVBTuner::RenderTuner(IGraphBuilder2* pGB)
{
	HRESULT hr;

	//

	CComPtr<IBaseFilter> tuner;

	std::wstring id = m_desc.id;

	std::wstring::size_type pos = id.find(L"$homesys$");

	if(pos != std::wstring::npos)
	{
		id = id.substr(0, pos);
	}

	hr = pGB->AddFilterForDisplayName(id.c_str(), &tuner);

	if(CComQIPtr<IKsPropertySet> ksps = tuner)
	{
		if(SUCCEEDED(ksps->Set(__uuidof(Firesat), FIRESAT_TEST_INTERFACE, NULL, 0, NULL, 0)))
		{
			m_firesat = ksps;
		}
		else 
		{
			ULONG mode = 0;
			ULONG len = 0;

			if(SUCCEEDED(ksps->Get(
				__uuidof(Realtek::KSPROPSETID_ModeProperties), 
				Realtek::DemodSupportModeProperty, 
				NULL, 0,
				&mode, sizeof(mode), 
				&len)) && len == sizeof(mode))
			{
				printf("%08x\n", mode);

				HRESULT hr2 = E_FAIL;

				if((mode & Realtek::SUPPORT_DVBT) != 0 && dynamic_cast<DVBTTuner*>(this) != NULL) // m_dvb.networkprovider == CLSID_DVBTNetworkProvider)
				{
					mode = Realtek::DVBT;

					hr2 = ksps->Set(__uuidof(Realtek::KSPROPSETID_ModeProperties), Realtek::SetDemodModeProperty, NULL, 0, &mode, sizeof(mode));
				}
				else if((mode & Realtek::SUPPORT_DVBC) != 0 && dynamic_cast<DVBCTuner*>(this) != NULL) // && m_dvb.networkprovider == CLSID_DVBCNetworkProvider)
				{
					mode = Realtek::DVBC;

					hr2 = ksps->Set(__uuidof(Realtek::KSPROPSETID_ModeProperties), Realtek::SetDemodModeProperty, NULL, 0, &mode, sizeof(mode));
				}

				//if(FAILED(hr2)) return false;

				m_realtek = ksps;
			}
		}
	}

	//

	CComPtr<IBaseFilter> np;

	if(!CreateNetworkProvider(&np))
	{
		return false;
	};

	hr = pGB->AddFilter(np, L"Network Provider");

	if(FAILED(hr))
	{
		return false;
	}

	if(CComQIPtr<ITuner> t = np)
	{
		CComPtr<ITuningSpace> ts;

		if(!GetTuningSpace(&ts))
		{
			return false;
		}

		hr = t->put_TuningSpace(ts);

		if(FAILED(hr))
		{
			_tprintf(_T("put_TuningSpace FAILED %08x\n"), hr);// , & ts);

			if(m_realtek == NULL)
			{
				return false;
			}

			hr = S_OK;
		}
	}

	CComPtr<IBaseFilter> pBF = np;

	if(SUCCEEDED(hr) && tuner != NULL)
	{
		hr = pGB->ConnectFilterDirect(pBF, tuner, NULL);

		if(FAILED(hr) && m_realtek != NULL)
		{
			_tprintf(_T("Trying generic Network Provider filter...\n"));

			hr = pGB->RemoveFilter(np);

			if(FAILED(hr))
			{
				return false;
			}

			np = NULL;

			hr = np.CoCreateInstance(CLSID_NetworkProvider);

			if(FAILED(hr))
			{
				return false;
			}

			hr = pGB->AddFilter(np, L"Network Provider");

			if(FAILED(hr))
			{
				return false;
			}

			pBF = np;

			hr = pGB->ConnectFilterDirect(pBF, tuner, NULL);
		}

		if(SUCCEEDED(hr))
		{
			pBF = tuner;
		}
		else
		{
			hr = pGB->RemoveFilter(tuner);
		}
	}

	Foreach(KSCATEGORY_BDA_RECEIVER_COMPONENT, [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
	{
		HRESULT hr;

		if(wcsstr(m_desc.name.c_str(), L"ASUS") != NULL && wcsstr(name, L"anysee") != NULL
		|| wcsstr(m_desc.name.c_str(), L"anysee") != NULL && wcsstr(name, L"ASUS") != NULL)
		{
			return S_CONTINUE;
		}

		CComPtr<IBaseFilter> receiver;
		
		hr = m->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&receiver);
		
		if(SUCCEEDED(hr))
		{
			hr = pGB->AddFilter(receiver, name);

			if(SUCCEEDED(hr))
			{
				hr = pGB->ConnectFilterDirect(pBF, receiver, NULL);

				if(SUCCEEDED(hr))
				{
					pBF = receiver;
					
					return S_OK;
				}
				else
				{
					hr = pGB->RemoveFilter(receiver);

					ASSERT(SUCCEEDED(hr));
				}
			}
		}

		return S_CONTINUE;
	});
/*
	{
		HRESULT hr;

		CComPtr<IBaseFilter> wintvci;

		m_wintvci = new WinTVCI(&wintvci);
		
		if(wintvci != NULL)
		{
			hr = pGB->AddFilter(wintvci, L"WinTVCI");

			if(SUCCEEDED(hr))
			{
				hr = pGB->ConnectFilterDirect(pBF, wintvci, NULL);

				if(SUCCEEDED(hr))
				{
					pBF = wintvci;
				}
				else
				{
					hr = pGB->RemoveFilter(wintvci);

					ASSERT(SUCCEEDED(hr));

					m_wintvci = NULL;
				}
			}
			else
			{
				m_wintvci = NULL;
			}
		}
	}
*/
	//

	CComPtr<IBaseFilter> tssink = new CTSSinkFilter(NULL, &hr);

	hr = pGB->AddFilter(tssink, L"TS Sink");

	if(FAILED(hr)) 
	{
		return false;
	}

	hr = pGB->ConnectFilterDirect(pBF, tssink, NULL);
	
	if(FAILED(hr)) 
	{
		return false;
	}

	{
		IPin* pin = DirectShow::GetFirstPin(tssink, PINDIR_OUTPUT);
/*
		hr = pGB->Render(pin);
*/
		CComPtr<IBaseFilter> tif = new CFakeTIF(NULL, &hr);

		hr = pGB->AddFilter(tif, L"TIF");
		
		hr = pGB->ConnectFilterDirect(pin, tif, NULL);
	}

	pGB->Dump();

	CComPtr<IGraphBuilder2> dst;

	dst = new CFGManagerCustom(_T(""), NULL);

	CComPtr<IBaseFilter> tssource = new CTSSourceFilter(NULL, &hr);

	hr = dst->AddFilter(tssource, L"TS Source");

	if(FAILED(hr))
	{
		return false;
	}

	CComQIPtr<ITSSinkFilter>(tssink)->SetSource(CComQIPtr<ITSSourceFilter>(tssource));

	pGB = dst;
	pBF = tssource;

	//

	CComPtr<ICommonInterface> ci;
	
	{
		HRESULT hr;

		CComPtr<IBaseFilter> wintvci;

		m_wintvci = new WinTVCI(&wintvci);
		
		if(wintvci != NULL)
		{
			hr = pGB->AddFilter(wintvci, L"WinTVCI");

			if(SUCCEEDED(hr))
			{
				hr = pGB->ConnectFilterDirect(pBF, wintvci, NULL);

				if(SUCCEEDED(hr))
				{
					pBF = wintvci;

					ci = m_wintvci;
				}
				else
				{
					hr = pGB->RemoveFilter(wintvci);

					ASSERT(SUCCEEDED(hr));

					m_wintvci = NULL;
				}
			}
			else
			{
				m_wintvci = NULL;
			}
		}
	}
	/*
	if(wcsstr(m_desc.id.c_str(), L"vid_0572&pid_88e6") != NULL)
	{
		m_geniaci = new GeniaCI();

		// TODO: init succeeded?

		ci = m_geniaci;
	}
	*/
	//
	
	CComPtr<IBaseFilter> demux = new CTSDemuxFilter(NULL, &hr);

	hr = pGB->AddFilter(demux, L"TS Demux");

	if(FAILED(hr))
	{
		return false;
	}

	hr = pGB->ConnectFilterDirect(pBF, demux, NULL);

	if(FAILED(hr)) 
	{
		return false;
	}

	CComPtr<IBaseFilter> tswriter = new CTSWriterFilter(NULL, &hr);

	hr = pGB->AddFilter(tswriter, L"TS Writer");

	if(FAILED(hr))
	{
		return false;
	}

	hr = pGB->ConnectFilterDirect(demux, tswriter, NULL);

	if(FAILED(hr)) 
	{
		return false;
	}

	m_dst = pGB;
	m_networkprovider = np;
	m_tuner = tuner;
	m_source = tssource;
	m_writer = tswriter;
	m_demux = demux;

	if(wcsstr(m_desc.name.c_str(), L"anysee") != NULL && wcsstr(m_desc.name.c_str(), L"DVB-S") != NULL)
	{
		m_anysee = new AnyseeTuner();

		if(!m_anysee->Open())
		{
			delete m_anysee;

			m_anysee = NULL;
		}
	}
	
	if(TechnotrendTuner::IsCompatible(m_desc.name.c_str()))
	{
		m_technotrend = new TechnotrendTuner();

		if(!m_technotrend->Open(m_desc.name.c_str()))
		{
			delete m_technotrend;

			m_technotrend = NULL;
		}
	}

	if(CComQIPtr<ITSDemuxFilter> demux = m_demux)
	{
		demux->SetKsPropertySet(CComQIPtr<IKsPropertySet>(m_tuner));
		demux->SetSmartCardServer(m_scs);
		demux->SetCommonInterface(ci);
	}

	return true;
}

bool DVBTuner::WaitSignal()
{
	clock_t t = clock();
		
	for(int i = 0; i < 100 && (clock() - t) < 300; i++)
	{
		TunerSignal signal;

		GetSignal(signal);

		if(signal.present > 0)
		{
			return true;
		}

		Sleep(1);
	}

	return false;
}

bool DVBTuner::Tune(const GenericPresetParam* p)
{
	const DVBPresetParam* pp = dynamic_cast<const DVBPresetParam*>(p);

	if(pp == NULL)
	{
		return false;
	}

	Stop();

	if(m_dst == NULL || m_demux == NULL)
	{
		return false;
	}

	int freq = pp->CarrierFrequency / 1000;

	CComQIPtr<ITSDemuxFilter> demux = m_demux;

	if(FAILED(demux->HasProgram(pp->sid, freq)))
	{
		TuneDVB(pp);

		WaitSignal();

		demux->SetFreq(freq);

		CComQIPtr<IMediaControl> mc = m_dst;

		mc->Run();

		std::list<TSDemuxProgram*> programs;

		demux->GetPrograms(programs, pp->sid, false);

		mc->Stop();
	}

	m_sid = pp->sid;
	m_freq = pp->CarrierFrequency;

	HRESULT hr = demux->SetProgram(pp->sid, true);
	
	m_dummy = hr == S_FALSE;

	Run();

	return true;
}

HRESULT DVBTuner::TuneDVB(const DVBPresetParam* p)
{
	CheckPointer(m_tuner, E_UNEXPECTED);

	HRESULT hr = E_FAIL;

	/*
	if(const DVBSPresetParam* ps = dynamic_cast<const DVBSPresetParam*>(p))
	{
		if(m_technotrend != NULL)
		{
			if(m_technotrend->Tune(ps))
			{
				return S_OK;
			}			
		}
	}
	*/

	/*
	if(CComQIPtr<ITuner> t = m_networkprovider)
	{
		CComPtr<ITuningSpace> ts;

		if(SUCCEEDED(t->get_TuningSpace(&ts)))
		{
			CComPtr<ITuneRequest> tr;

			if(SUCCEEDED(ts->CreateTuneRequest(&tr)))
			{
				CComPtr<ILocator> l;

				if(SUCCEEDED(ts->get_DefaultLocator(&l)))
				{
					l->put_CarrierFrequency(p->CarrierFrequency);
					l->put_InnerFEC(p->InnerFEC);
					l->put_InnerFECRate(p->InnerFECRate);
					l->put_Modulation(p->Modulation);
					l->put_OuterFEC(p->OuterFEC);
					l->put_OuterFECRate(p->OuterFECRate);
					l->put_SymbolRate(p->SymbolRate);
					
					if(const DVBSPresetParam* ps = dynamic_cast<const DVBSPresetParam*>(p))
					{
						if(CComQIPtr<IDVBSTuningSpace> tss = ts)
						{
							tss->put_LowOscillator(ps->LowOscillator);
							tss->put_HighOscillator(ps->HighOscillator);
							tss->put_LNBSwitch(ps->LNBSwitch);
							tss->put_SpectralInversion(ps->SpectralInversion);
						}

						if(CComQIPtr<IDVBSLocator> ls = l)
						{
							ls->put_Azimuth(ps->Azimuth);
							ls->put_Elevation(ps->Elevation);
							ls->put_OrbitalPosition(ps->OrbitalPosition);
							ls->put_SignalPolarisation(ps->SignalPolarisation);
							ls->put_WestPosition(ps->WestPosition);
						}

						if(CComQIPtr<IDVBSLocator2> ls2 = l)
						{
							// ls2->put_SignalRollOff(BDA_ROLL_OFF_35);
							// ls2->put_SignalPilot(BDA_PILOT_ON);

							// TODO
						}
					}
					else if(const DVBTPresetParam* pt = dynamic_cast<const DVBTPresetParam*>(p))
					{
						if(CComQIPtr<IDVBTLocator> lt = l)
						{
							lt->put_Bandwidth(pt->Bandwidth);
							lt->put_Guard(pt->Guard);
							lt->put_HAlpha(pt->HAlpha);
							lt->put_LPInnerFEC(pt->LPInnerFEC);
							lt->put_LPInnerFECRate(pt->LPInnerFECRate);
							lt->put_Mode(pt->Mode);
							lt->put_OtherFrequencyInUse(pt->OtherFrequencyInUse);
						}
					}
					else if(const DVBCPresetParam* pc = dynamic_cast<const DVBCPresetParam*>(p))
					{
						if(CComQIPtr<IDVBCLocator> lc = l)
						{
						}
					}

					hr = tr->put_Locator(l);
				}

				hr = t->put_TuneRequest(tr);
			}
		}
	}
	
	return hr;
	*/

	if(m_realtek != NULL)
	{
		if(const DVBCPresetParam* ps = dynamic_cast<const DVBCPresetParam*>(p))
		{
			ULONG mode = 0;
			ULONG len = 0;

			if(SUCCEEDED(m_realtek->Get(__uuidof(Realtek::KSPROPSETID_ModeProperties), Realtek::DemodSupportModeProperty, NULL, 0, &mode, sizeof(mode), &len)) && len == sizeof(mode))
			{
				if((mode & Realtek::SUPPORT_DVBC) != 0 && dynamic_cast<DVBCTuner*>(this) != NULL)
				{
					mode = Realtek::DVBC;
			
					m_realtek->Set(__uuidof(Realtek::KSPROPSETID_ModeProperties), Realtek::SetDemodModeProperty, NULL, 0, &mode, sizeof(mode));
				}
			}
		}

		ULONG modulation = p->Modulation;
		ULONG symbolrate = p->SymbolRate;

		m_realtek->Set(__uuidof(Realtek::KSPROPSETID_ControlProperties), Realtek::ModulationProperty, NULL, 0, &modulation, sizeof(modulation));
		m_realtek->Set(__uuidof(Realtek::KSPROPSETID_ControlProperties), Realtek::SymbolRateProperty, NULL, 0, &symbolrate, sizeof(symbolrate));
	}

	CComQIPtr<IBDA_LNBInfo> lnb;
	CComQIPtr<IBDA_FrequencyFilter> ff;
	CComQIPtr<IBDA_AutoDemodulate> ad;
	CComQIPtr<IBDA_DigitalDemodulator> dd;

	if(CComQIPtr<IBDA_Topology> topology = m_tuner)
	{
		ULONG nc = 0;
		ULONG nt[32];

		hr = topology->GetNodeTypes(&nc, 32, nt);

		for(ULONG i = 0; i < nc; i++)
		{
			CComPtr<IUnknown> unk;

			hr = topology->GetControlNode(0, 1, nt[i], &unk);

			ULONG ic = 0;
			GUID it[32];

			hr = topology->GetNodeInterfaces(nt[i], &ic, 32, it);

			for(ULONG j = 0; j < ic; j++)
			{
				if(it[j] == KSPROPSETID_BdaLNBInfo) lnb = unk;
				else if(it[j] == KSPROPSETID_BdaFrequencyFilter) ff = unk;
				else if(it[j] == KSPROPSETID_BdaAutodemodulate) ad = unk;
				else if(it[j] == KSPROPSETID_BdaDigitalDemodulator) dd = unk;
			}
		}
		
		if(CComQIPtr<IBDA_DeviceControl> dc = m_tuner)
		{
			hr = dc->StartChanges();
		}

		if(FAILED(hr))
		{
			return hr;
		}
		
		if(lnb != NULL)
		{
			if(const DVBSPresetParam* ps = dynamic_cast<const DVBSPresetParam*>(p))
			{
				hr = lnb->put_LocalOscilatorFrequencyLowBand(ps->LowOscillator);
				hr = lnb->put_LocalOscilatorFrequencyHighBand(ps->HighOscillator);
				hr = lnb->put_HighLowSwitchFrequency(ps->LNBSwitch);
			}
		}

		if(ff != NULL)
		{
			hr = ff->put_FrequencyMultiplier(1000);
			hr = ff->put_Frequency(p->CarrierFrequency);

			if(const DVBTPresetParam* pt = dynamic_cast<const DVBTPresetParam*>(p))
			{
				hr = ff->put_Bandwidth(pt->Bandwidth);
			}
			else
			{
				hr = ff->put_Bandwidth(8);
			}
			
			if(const DVBSPresetParam* ps = dynamic_cast<const DVBSPresetParam*>(p))
			{
				hr = ff->put_Polarity(ps->SignalPolarisation);

				ULONG range = 0;

				switch(ps->LNBSource)
				{
				case 1: range = 0x00000000; break;
				case 2: range = 0x00000001; break;
				case 3: range = 0x00010000; break;
				case 4: range = 0x00010001; break;
				default: range = 0xffffffff; break;
				}

				hr = ff->put_Range(range);

				if(m_anysee != NULL)
				{
					m_anysee->SelectLNB(0xF0 | ((ps->LNBSource - 1) << 2));
				}

				if(m_firesat != NULL)
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
					cmd.DiseqcCmd[0].Data[0] = 0xF0 | ((ps->LNBSource - 1) << 2);

					m_firesat->Set(__uuidof(Firesat), FIRESAT_LNB_CONTROL, &instance, sizeof(instance), &cmd, sizeof(cmd));
				}
			}
		}
		
		if(0) // ad != NULL)
		{
			hr = ad->put_AutoDemodulate();
		}

		if(dd != NULL)
		{	
			ModulationType modulation = p->Modulation;
			FECMethod ifec = p->InnerFEC;
			BinaryConvolutionCodeRate ifecrate = p->InnerFECRate;
			FECMethod ofec = p->OuterFEC;
			BinaryConvolutionCodeRate ofecrate = p->OuterFECRate;
			ULONG symbolrate = p->SymbolRate;

			if(m_technotrend != NULL && modulation == BDA_MOD_NBC_8PSK)
			{
				modulation = BDA_MOD_8VSB;
			}

			hr = dd->put_ModulationType(&modulation);
			hr = dd->put_InnerFECMethod(&ifec);
			hr = dd->put_InnerFECRate(&ifecrate);
			hr = dd->put_OuterFECMethod(&ofec);
			hr = dd->put_OuterFECRate(&ofecrate);
			hr = dd->put_SymbolRate(&symbolrate);
			
			if(m_anysee != NULL)
			{
				m_anysee->SetMode(p->Modulation, p->InnerFECRate);
			}

			if(const DVBSPresetParam* ps = dynamic_cast<const DVBSPresetParam*>(p))
			{
				SpectralInversion si = ps->SpectralInversion;

				hr = dd->put_SpectralInversion(&si);
			}
			
			if(CComQIPtr<IBDA_DigitalDemodulator2> dd2 = dd)
			{
				if(const DVBTPresetParam* pt = dynamic_cast<const DVBTPresetParam*>(p))
				{
					GuardInterval gi = pt->Guard;
					TransmissionMode tm = pt->Mode;

					hr = dd2->put_GuardInterval(&gi);
					hr = dd2->put_TransmissionMode(&tm);
				}
			}
		}
	
		hr = E_FAIL;

		if(CComQIPtr<IBDA_DeviceControl> dc = m_tuner)
		{
			hr = dc->CheckChanges();

			if(SUCCEEDED(hr))
			{
				hr = dc->CommitChanges();
			}
		}

		if(1)
		{
			std::wstring s = p->ToString();

			wprintf(L"TuneDVB(%s)\n", s.c_str());
		}
	}

	return hr;
}

bool DVBTuner::TuneMayChangeFormat()
{
	return true;
}

void DVBTuner::GetPrograms(std::list<TunerProgram>& programs)
{
	programs.clear();

	CComQIPtr<ITSDemuxFilter> demux = m_demux;

	if(demux != NULL)
	{
		std::list<TSDemuxProgram*> src;

		demux->GetPrograms(src, 0, true);

		for(auto i = src.begin(); i != src.end(); i++)
		{
			TunerProgram p;

			p.name = (*i)->name;
			p.provider = (*i)->provider;
			p.type = (*i)->type;
			p.pid = (*i)->pid;
			p.sid = (*i)->sid;
			p.pcr = (*i)->pcr;
			p.scrambled = (*i)->scrambled || !(*i)->ecms.empty();

			// TODO: streams

			programs.push_back(p);
		}
	}
}

WORD DVBTuner::GetPort()
{
	WORD port = 0;

	if(m_writer != NULL)
	{
		CComQIPtr<ITSWriterFilter>(m_writer)->GetPort(&port);
	}

	return port;
}

//

DVBTuner::AnyseeTuner::AnyseeTuner()
	: m_nim(NULL)
{
}

DVBTuner::AnyseeTuner::~AnyseeTuner()
{
	Close();
}

DWORD WINAPI DVBTuner::AnyseeTuner::ThreadProc(void* p)
{
	CoInitialize(0);

	AnyseeTuner* t = (AnyseeTuner*)p;

	CanyseeNIMEx* nim = new CanyseeNIMEx();

	if(nim->GetNumberOfDevices() > 0 && nim->Open(1) == 0)
	{
		nim->EnableLNB(TRUE);
/*
		nim->SetLNBType(Universal_LNBType);
		nim->SetLOFBand(9750, true);
		nim->SetLOFBand(10600, false);
*/
		nim->SetPilot(AMTNIM_Pilot_Auto);

		t->m_nim = nim;
	}
	else
	{
		delete nim;
	}

	CoUninitialize();

	return 0;
}

bool DVBTuner::AnyseeTuner::Open()
{
	Close();

	DWORD id;

	WaitForSingleObject(CreateThread(NULL, 0, ThreadProc, this, 0, &id), INFINITE);

	return m_nim != NULL;
}

void DVBTuner::AnyseeTuner::Close()
{
	if(m_nim != NULL)
	{
		m_nim->Close();

		m_nim = NULL;
	}
}

bool DVBTuner::AnyseeTuner::SetMode(ModulationType modulation, BinaryConvolutionCodeRate ifecrate)
{
	if(m_nim == NULL)
	{
		return false;
	}

	eNIM_Mode mode = enNIM_None;

	switch(modulation)
	{
	case BDA_MOD_NBC_QPSK:
		switch(ifecrate)
		{
		case BDA_BCC_RATE_1_2: mode = enNIM_LDPCBCH_QPSK_1_2; break;
		case BDA_BCC_RATE_2_3: mode = enNIM_LDPCBCH_QPSK_2_3; break;
		case BDA_BCC_RATE_3_4: mode = enNIM_LDPCBCH_QPSK_3_4; break;
		case BDA_BCC_RATE_3_5: mode = enNIM_LDPCBCH_QPSK_3_5; break;
		case BDA_BCC_RATE_4_5: mode = enNIM_LDPCBCH_QPSK_4_5; break;
		case BDA_BCC_RATE_5_6: mode = enNIM_LDPCBCH_QPSK_5_6; break;
		case BDA_BCC_RATE_5_11: break;
		case BDA_BCC_RATE_7_8: break;
		case BDA_BCC_RATE_1_4: break;
		case BDA_BCC_RATE_1_3: break;
		case BDA_BCC_RATE_2_5: break;
		case BDA_BCC_RATE_6_7: break;
		case BDA_BCC_RATE_8_9: mode = enNIM_LDPCBCH_QPSK_8_9; break;
		case BDA_BCC_RATE_9_10: mode = enNIM_LDPCBCH_QPSK_9_10; break;
		} 
		break;
	case BDA_MOD_NBC_8PSK:
		switch(ifecrate)
		{
		case BDA_BCC_RATE_1_2: break;
		case BDA_BCC_RATE_2_3: mode = enNIM_LDPCBCH_8PSK_2_3; break;
		case BDA_BCC_RATE_3_4: mode = enNIM_LDPCBCH_8PSK_3_4; break;
		case BDA_BCC_RATE_3_5: mode = enNIM_LDPCBCH_8PSK_3_5; break;
		case BDA_BCC_RATE_4_5: break;
		case BDA_BCC_RATE_5_6: mode = enNIM_LDPCBCH_8PSK_5_6; break;
		case BDA_BCC_RATE_5_11: break;
		case BDA_BCC_RATE_7_8:  break;
		case BDA_BCC_RATE_1_4: break;
		case BDA_BCC_RATE_1_3: break;
		case BDA_BCC_RATE_2_5: break;
		case BDA_BCC_RATE_6_7: break;
		case BDA_BCC_RATE_8_9: mode = enNIM_LDPCBCH_8PSK_8_9; break;
		case BDA_BCC_RATE_9_10: mode = enNIM_LDPCBCH_8PSK_9_10; break;
		}
		break;
	}

	if(mode != enNIM_None)
	{
		return m_nim->SetMode(mode) == 0;
	}

	return false;
}

void DVBTuner::AnyseeTuner::SelectLNB(BYTE DiSEqC10Flag, bool bDiSEqC1v1)
{
	m_nim->SelectLNB(DiSEqC10Flag, bDiSEqC1v1);
}

int DVBTuner::AnyseeTuner::SendDiSEqCData(PBYTE pData, DWORD dwLength, EnumDiSEqCToneBurst ToneBurst)
{
	return m_nim->SendDiSEqCData(pData, dwLength, ToneBurst);
}

//

DVBTuner::TechnotrendTuner::TechnotrendTuner()
	: m_device(NULL)
{
}

DVBTuner::TechnotrendTuner::~TechnotrendTuner()
{
	Close();
}

bool DVBTuner::TechnotrendTuner::IsCompatible(LPCWSTR name)
{
	static LPCWSTR names[] = 
	{
		BDG2_NAME,
		BDG2_NAME_C_TUNER,
		BDG2_NAME_C_TUNER_FAKE,
		BDG2_NAME_S_TUNER,
		BDG2_NAME_S_TUNER_FAKE,
		BDG2_NAME_T_TUNER,
		BDG2_NAME_NEW,
		BDG2_NAME_C_TUNER_NEW,
		BDG2_NAME_S_TUNER_NEW,
		BDG2_NAME_T_TUNER_NEW,
		BUDGET3NAME,
		BUDGET3NAME_TUNER,
		BUDGET3NAME_ATSC_TUNER,
		BUDGET3NAME_TUNER_ANLG,
		BUDGET3NAME_ANLG,
		USB2BDA_DVB_NAME,
		USB2BDA_DSS_NAME,
		USB2BDA_DSS_NAME_TUNER,
		USB2BDA_DVB_NAME_C_TUNER,
		USB2BDA_DVB_NAME_C_TUNER_FAKE,
		USB2BDA_DVB_NAME_S_TUNER,
		USB2BDA_DVB_NAME_S_TUNER_FAKE,
		USB2BDA_DVB_NAME_T_TUNER,
		USB2BDA_DVBS_NAME_PIN,
		USB2BDA_DVBS_NAME_PIN_TUNER,
		PREMIUM_NAME_DIGITAL_CAPTURE,
		PREMIUM_NAME_DIGITAL_TS_CAPTURE,
		PREMIUM_NAME_DIGITAL_TUNER
	};

	for(int i = 0; i < sizeof(names) / sizeof(names[0]); i++)
	{
		if(wcscmp(name, names[i]) == 0) return true;
	}

	return false;
}

bool DVBTuner::TechnotrendTuner::Open(LPCWSTR name)
{
	Close();

	static DEVICE_CAT types[] = 
	{
	    BUDGET_2,
		BUDGET_3,
		USB_2,
		USB_2_PINNACLE,
		USB_2_DSS,
		PREMIUM
	};

	for(int i = 0; i < sizeof(types) / sizeof(types[0]); i++)
	{
		UINT count = bdaapiEnumerate(types[i]);

		for(UINT i = 0; i < count; i++)
		{
			HANDLE device = bdaapiOpen(types[i], i);

			if(device == NULL) break;

			TS_FilterNames fn;

			bdaapiGetDevNameAndFEType(device, &fn);

			std::string s = fn.szTunerFilterName;

			std::wstring ws(s.begin(), s.end());

			if(ws == name)
			{
				m_device = device;

				return true;
			}

			bdaapiClose(device);
		}
	}

	return false;
}

void DVBTuner::TechnotrendTuner::Close()
{
	if(m_device != NULL)
	{
		bdaapiClose(m_device);

		m_device = NULL;
	}
}

bool DVBTuner::TechnotrendTuner::Tune(const DVBPresetParam* p)
{
	//if(p->Modulation != BDA_MOD_NBC_8PSK) return false;

	structDVB_TunReq req;

	memset(&req, 0, sizeof(req));

	if(const DVBSPresetParam* pp = dynamic_cast<const DVBSPresetParam*>(p))
	{
		req.dvbType = DVB_S;
		req.data.DVBS_TunReq.dwFrequency = p->CarrierFrequency;
		req.data.DVBS_TunReq.dwFrequencyMultiplier = 1000;
		req.data.DVBS_TunReq.tPolarity = pp->SignalPolarisation;
		req.data.DVBS_TunReq.dwModulationType = p->Modulation == BDA_MOD_NBC_8PSK ? BDA_MOD_8VSB : p->Modulation;
		req.data.DVBS_TunReq.dwSymbolRate = p->SymbolRate;
		req.data.DVBS_TunReq.dwSpectralInversion = pp->SpectralInversion;
		req.data.DVBS_TunReq.dwHighBandLOF = pp->HighOscillator;
		req.data.DVBS_TunReq.dwLowBandLOF = pp->LowOscillator;
		req.data.DVBS_TunReq.dwSwitchFrequency = pp->LNBSwitch;
		req.data.DVBS_TunReq.dwMode = 1; // ?
		req.data.DVBS_TunReq.dwFECRate = pp->InnerFECRate;
		req.data.DVBS_TunReq.dwFECType = pp->InnerFEC;
		req.data.DVBS_TunReq.dwOuterFECRate = pp->OuterFECRate;
		req.data.DVBS_TunReq.dwOuterFECType = pp->OuterFEC;

		switch(pp->LNBSource)
		{
		case 1: req.data.DVBS_TunReq.dwRange = 0x00000000; break;
		case 2: req.data.DVBS_TunReq.dwRange = 0x00000001; break;
		case 3: req.data.DVBS_TunReq.dwRange = 0x00010000; break;
		case 4: req.data.DVBS_TunReq.dwRange = 0x00010001; break;
		default: req.data.DVBS_TunReq.dwRange = 0xffffffff; break;
		}
	}
	else
	{
		return false;
	}

	TYPE_RET_VAL ret;

	ret = bdaapiTune(m_device, &req);

	return ret == RET_SUCCESS;
}
