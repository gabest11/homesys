// This is the main DLL file.

#include "stdafx.h"
#include "tuner.h"
#include <vcclr.h>
#include <msclr\lock.h>

using namespace System::IO;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;
using namespace Microsoft::Win32;

std::wstring Translate(String^ s)
{
	if(s != nullptr)
	{
		pin_ptr<const wchar_t> ptr = PtrToStringChars(s);

		return std::wstring((const wchar_t*)(ptr), s->Length);
	}

	return L"";
}

String^ Translate(LPCWSTR s)
{
	return gcnew String(s);
}

DateTime Translate(const CTime& src)
{
	return DateTime(src.GetYear(), src.GetMonth(), src.GetDay(), src.GetHour(), src.GetMinute(), src.GetSecond());
}

namespace Homesys
{
	Tuner::Tuner()
		: m_tuner(NULL)
		, m_version(-1)
		, m_recording(false)
		, m_presetId(0)
		, m_dummy(false)
	{
		m_idle = DateTime::Now;

		// TODO
	}

	Tuner::~Tuner()
	{
		this->!Tuner();
	}

	Tuner::!Tuner()
	{
		Close();
	}

	IEnumerable<TunerDevice^>^ Tuner::EnumTuners()
	{
		List<TunerDevice^>^ tuners = gcnew List<TunerDevice^>();

		std::list<TunerDesc> tds;

		GenericTuner::EnumTuners(tds, true, true);

		for(auto i = tds.begin(); i != tds.end(); i++)
		{
			TunerDevice^ tuner = gcnew TunerDevice();

			tuner->dispname = gcnew String(i->id.c_str());
			tuner->name = gcnew String(i->name.c_str());
			tuner->type = TunerDeviceType::None;

			switch(i->type)
			{
			case TunerDesc::Analog:
				tuner->type = TunerDeviceType::Analog;
				break;
			case TunerDesc::DVBT:
				tuner->type = TunerDeviceType::DVBT;
				break;
			case TunerDesc::DVBS:
				tuner->type = TunerDeviceType::DVBS;
				break;
			case TunerDesc::DVBC:
				tuner->type = TunerDeviceType::DVBC;
				break;
			case TunerDesc::DVBF:
				tuner->type = TunerDeviceType::DVBF;
				break;
			}

			if(tuner->type != TunerDeviceType::None)
			{
				tuners->Add(tuner);
			}
		}

		return tuners;
	}

	IEnumerable<TunerDevice^>^ Tuner::EnumTunersNoType()
	{
		List<TunerDevice^>^ tuners = gcnew List<TunerDevice^>();

		std::list<TunerDesc> tds;

		GenericTuner::EnumTunersNoType(tds);

		for(auto i = tds.begin(); i != tds.end(); i++)
		{
			TunerDevice^ tuner = gcnew TunerDevice();

			tuner->dispname = gcnew String(i->id.c_str());
			tuner->name = gcnew String(i->name.c_str());
			tuner->type = TunerDeviceType::None;

			tuners->Add(tuner);
		}

		return tuners;
	}

	struct TunerThreadParam
	{
		GenericTuner* tuner;
		TunerDesc desc;
		CComPtr<ISmartCardServer> scs;
		enum {Open, Close} action;
	};

	static DWORD WINAPI TunerThreadProc(void* param)
	{
		//CoInitialize(0);
		CoInitializeEx(0, COINIT_MULTITHREADED);

		TunerThreadParam* p = (TunerThreadParam*)param;

		DWORD ret = 0;
		
		if(p->action == TunerThreadParam::Open)
		{
			ret = p->tuner->Open(p->desc, p->scs) ? TRUE : FALSE;
		}
		else if(p->action == TunerThreadParam::Close)
		{
			p->tuner->Close();

			ret = 1;
		}

		CoUninitialize();

		return ret;
	}

	void Tuner::Open(TunerDevice^ td, Guid id, SmartCardServer^ scs)
	{
		Close();

		m_id = id;

		TunerDesc desc;

		desc.id = Translate(td->dispname);
		desc.name = Translate(td->name);

		GenericTuner* tuner = NULL;

		switch(td->type)
		{
		case TunerDeviceType::Analog:
			tuner = new AnalogTuner(true);
			desc.type = TunerDesc::Analog;
			break;
		case TunerDeviceType::DVBT:
			tuner = new DVBTTuner();
			desc.type = TunerDesc::DVBT;
			break;
		case TunerDeviceType::DVBS:
			tuner = new DVBSTuner();
			desc.type = TunerDesc::DVBS;
			break;
		case TunerDeviceType::DVBC:
			tuner = new DVBCTuner();
			desc.type = TunerDesc::DVBC;
			break;
		case TunerDeviceType::DVBF:
			tuner = new DVBFTuner();
			desc.type = TunerDesc::DVBF;
			break;
		default:
			throw gcnew Exception(String::Format("Unexpected tuner type ({0})", td->type));
		}
		
		if(!tuner->Open(desc, scs != nullptr ? scs->m_obj : NULL))
		{
			delete tuner;

			throw gcnew Exception(String::Format("Cannot open tuner ({0})", td->name));
		}
		
/*
		TunerThreadParam p;

		p.tuner = tuner;
		p.desc = desc;
		p.scs = scs != nullptr ? scs->m_obj : NULL;
		p.action = TunerThreadParam::Open;

		DWORD tid = 0;
	
		HANDLE hThread = CreateThread(NULL, 0, TunerThreadProc, &p, 0, &tid);

		WaitForSingleObject(hThread, INFINITE);

		DWORD ret = 0;
		
		GetExitCodeThread(hThread, &ret);

		CloseHandle(hThread);

		if(!ret)
		{
			delete tuner;

			throw gcnew Exception(String::Format("Cannot open tuner ({0})", td->name));
		}
*/
		tuner->Run();

		m_tuner = tuner;
	}

	void Tuner::Close()
	{
		if(m_tuner != NULL)
		{
			m_tuner->Close();
			/*
			TunerThreadParam p;

			p.tuner = m_tuner;
			p.action = TunerThreadParam::Close;

			DWORD tid = 0;
	
			HANDLE hThread = CreateThread(NULL, 0, TunerThreadProc, &p, 0, &tid);

			WaitForSingleObject(hThread, INFINITE);

			CloseHandle(hThread);
			*/
			delete m_tuner;

			m_tuner = NULL;
		}
	}

	bool Tuner::Timeshift(String^ dir)
	{
		if(m_tuner->Timeshift(Translate(dir).c_str(), Translate(m_id.ToString()).c_str()))
		{
			m_fn = Translate(m_tuner->GetTimeshiftFile());
			m_version = GetCounter();
			m_dummy = m_tuner->IsTimeshiftDummy();

			return true;
		}

		return false;
	}

	bool Tuner::Timeshift(String^ dir, Local::TuneRequest^ tr, int presetId)
	{
		bool ret = false;

		m_presetId = 0;

		GenericPresetParam* p = Convert(tr);

		if(p != NULL)
		{
			if(m_tuner->Timeshift(Translate(dir).c_str(), Translate(m_id.ToString()).c_str(), p))
			{
				m_fn = Translate(m_tuner->GetTimeshiftFile());
				m_version = GetCounter();
				m_dummy = m_tuner->IsTimeshiftDummy();
		
				ret = true;
			}

			delete p;

			m_presetId = presetId;
		}

		return ret;
	}
	
	String^ Tuner::GetFileName()
	{
		return m_fn;
	}

	int Tuner::GetFileVersion()
	{
		return m_version;
	}

	IEnumerable<int>^ Tuner::GetConnectorTypes()
	{
		List<int>^ connectors = gcnew List<int>();

		std::list<PhysicalConnectorType> types;

		m_tuner->GetConnectorTypes(types);

		for(auto i = types.begin(); i != types.end(); i++)
		{
			connectors->Add(*i);
		}

		return connectors;
	}

	String^ Tuner::GetConnectorName(int type)
	{
		return gcnew String(GenericTuner::ToString((PhysicalConnectorType)type).c_str());
	}

	bool Tuner::IsTunerConnectorType(int type)
	{
		return GenericTuner::IsTunerConnectorType((PhysicalConnectorType)type);
	}

	TunerStat^ Tuner::GetStat()
	{
		TunerStat^ stat = gcnew TunerStat();

		stat->frequency = m_tuner->GetFreq();

		TunerSignal s;
		
		m_tuner->GetSignal(s);

		stat->locked = s.locked;
		stat->present = s.present;
		stat->strength = s.strength;
		stat->quality = s.quality;
		stat->received = m_tuner->GetReceivedBytes();
		stat->scrambled = m_tuner->IsScrambled();

		return stat;
	}

	void Tuner::Tune(Local::TuneRequest^ tr, int presetId)
	{
		m_presetId = 0;

		GenericPresetParam* p = Convert(tr);

		if(p != NULL)
		{
			m_tuner->Tune(p);

			delete p;

			m_presetId = presetId;
		}
	}

	IEnumerable<TunerProgram^>^ Tuner::GetPrograms()
	{
		std::list<::TunerProgram> src;

		m_tuner->GetPrograms(src);

		List<TunerProgram^>^ programs = gcnew List<TunerProgram^>();

		for(auto i = src.begin(); i != src.end(); i++)
		{
			TunerProgram^ p = gcnew TunerProgram();

			p->name = gcnew String(i->name.c_str());
			p->provider = gcnew String(i->provider.c_str());
			p->sid = i->sid;
			p->scrambled = i->scrambled;

			programs->Add(p);
		}

		return programs;
	}

	void Tuner::ResetIdleTime()
	{
		m_idle = DateTime::Now;
	}

	TimeSpan Tuner::GetIdleTime()
	{
		return DateTime::Now - m_idle;
	}

	void Tuner::StartRecording(String^ path, String^ format, String^ title, Guid recordingId)
	{
		if(m_tuner == NULL)
		{
			Marshal::ThrowExceptionForHR(E_UNEXPECTED);
		}

		title = String::Format("{0} ({1})", title, DateTime::Now.Ticks / TimeSpan::TicksPerMillisecond);

		array<wchar_t>^ charsToEscape = gcnew array<wchar_t> {'\\', '/', ':', '?', '*', '<', '>', '|'};
		
		for each(wchar_t c in charsToEscape) title = title->Replace(c, '_');

		if(m_tuner->GetDesc().type == TunerDesc::Analog)
		{
			format = L".dsm"; // FIXME
		}

		path = Path::Combine(path, title) + format;

		if(!m_tuner->StartRecording(Translate(path).c_str()))
		{
			Marshal::ThrowExceptionForHR(E_UNEXPECTED);
		}

		m_recording = true;
		m_recordingId = recordingId;
		m_recordingPath = path;
	}

	void Tuner::StopRecording()
	{
		if(m_tuner != NULL)
		{
			m_tuner->StopRecording();
		}

		m_recording = false;
	}

	bool Tuner::IsRecording()
	{
		return m_recording;
	}

	Guid Tuner::GetRecordingId()
	{
		return m_recordingId;
	}

	String^ Tuner::GetRecordingPath()
	{
		return m_recordingPath;
	}

	int Tuner::GetCounter()
	{
		msclr::lock l((UInt32^)s_counter);

		return s_counter++;
	}

	int Tuner::GetPresetId()
	{
		return m_presetId;
	}

	GenericPresetParam* Tuner::Convert(Local::TuneRequest^ tr)
	{
		GenericPresetParam* p = NULL;
		AnalogPresetParam* analog = NULL;
		DVBTPresetParam* dvbt = NULL;
		DVBSPresetParam* dvbs = NULL;
		DVBCPresetParam* dvbc = NULL;
		DVBFPresetParam* dvbf = NULL;

		switch(m_tuner->GetDesc().type)
		{
		case TunerDesc::Analog:
			analog = new AnalogPresetParam();
			analog->Standard = (AnalogVideoStandard)tr->standard;
			analog->Connector = tr->connector->num;
			analog->Frequency = tr->freq;
			p = analog;
			break;
		case TunerDesc::DVBT:
			dvbt = new DVBTPresetParam();
			dvbt->sid = tr->sid;
			if(tr->freq) dvbt->CarrierFrequency = tr->freq;
			if(tr->ifec) dvbt->InnerFEC = (FECMethod)tr->ifec;
			if(tr->ifecRate) dvbt->InnerFECRate = (BinaryConvolutionCodeRate)tr->ifecRate;
			if(tr->ofec) dvbt->OuterFEC = (FECMethod)tr->ofec; 
			if(tr->ofecRate) dvbt->OuterFECRate = (BinaryConvolutionCodeRate)tr->ofecRate;
			if(tr->modulation) dvbt->Modulation = (ModulationType)tr->modulation;
			if(tr->symbolRate) dvbt->SymbolRate = tr->symbolRate;
			if(tr->bandwidth) dvbt->Bandwidth = tr->bandwidth;
			if(tr->lpifec) dvbt->LPInnerFEC = (FECMethod)tr->lpifec;
			if(tr->lpifecRate) dvbt->LPInnerFECRate = (BinaryConvolutionCodeRate)tr->lpifecRate;
			if(tr->halpha) dvbt->HAlpha = (HierarchyAlpha)tr->halpha;
			if(tr->guard) dvbt->Guard = (GuardInterval)tr->guard;
			if(tr->transmissionMode) dvbt->Mode = (TransmissionMode)tr->transmissionMode;
			if(tr->otherFreqInUse) dvbt->OtherFrequencyInUse = (VARIANT_BOOL)tr->otherFreqInUse;
			p = dvbt;
			break;
		case TunerDesc::DVBS:
			dvbs = new DVBSPresetParam();
			dvbs->sid = tr->sid;
			dvbs->CarrierFrequency = tr->freq;
			if(tr->ifec) dvbs->InnerFEC = (FECMethod)tr->ifec;
			if(tr->ifecRate) dvbs->InnerFECRate = (BinaryConvolutionCodeRate)tr->ifecRate;
			if(tr->ofec) dvbs->OuterFEC = (FECMethod)tr->ofec; 
			if(tr->ofecRate) dvbs->OuterFECRate = (BinaryConvolutionCodeRate)tr->ofecRate;
			if(tr->modulation) dvbs->Modulation = (ModulationType)tr->modulation;
			if(tr->symbolRate) dvbs->SymbolRate = tr->symbolRate;
			if(tr->polarisation) dvbs->SignalPolarisation = (Polarisation)tr->polarisation;
			if(tr->west) dvbs->WestPosition = (VARIANT_BOOL)tr->west;
			if(tr->orbitalPosition) dvbs->OrbitalPosition = tr->orbitalPosition;
			if(tr->azimuth) dvbs->Azimuth = tr->azimuth;
			if(tr->elevation) dvbs->Elevation = tr->elevation;
			if(tr->lnbSource) dvbs->LNBSource = (LNB_Source)tr->lnbSource;
			if(tr->lowOscillator) dvbs->LowOscillator = tr->lowOscillator;
			if(tr->highOscillator) dvbs->HighOscillator = tr->highOscillator;
			if(tr->lnbSwitch) dvbs->LNBSwitch = tr->lnbSwitch;
			if(tr->spectralInversion) dvbs->SpectralInversion = (SpectralInversion)tr->spectralInversion;
			p = dvbs;
			break;
		case TunerDesc::DVBC:
			dvbc = new DVBCPresetParam();
			dvbc->sid = tr->sid;
			dvbc->CarrierFrequency = tr->freq;
			if(tr->ifec) dvbc->InnerFEC = (FECMethod)tr->ifec;
			if(tr->ifecRate) dvbc->InnerFECRate = (BinaryConvolutionCodeRate)tr->ifecRate;
			if(tr->ofec) dvbc->OuterFEC = (FECMethod)tr->ofec; 
			if(tr->ofecRate) dvbc->OuterFECRate = (BinaryConvolutionCodeRate)tr->ofecRate;
			if(tr->modulation) dvbc->Modulation = (ModulationType)tr->modulation;
			if(tr->symbolRate) dvbc->SymbolRate = tr->symbolRate;
			p = dvbc;
			break;
		case TunerDesc::DVBF:
			dvbf = new DVBFPresetParam();
			dvbf->sid = tr->sid;
			dvbf->CarrierFrequency = tr->path->GetHashCode();
			dvbf->path = Translate(tr->path);
			p = dvbf;
			break;
		}

		return p;
	}

	int Tuner::GetPort()
	{
		int port = 0;

		if(DVBTuner* t = dynamic_cast<DVBTTuner*>(m_tuner))
		{
			port = (int)t->GetPort();
		}

		return port;
	}

	//

	SmartCardServer::SmartCardServer()
	{
		m_obj = new ::SmartCardServer();

		m_obj->AddRef();
	}

	SmartCardServer::~SmartCardServer()
	{
		this->!SmartCardServer();
	}

	SmartCardServer::!SmartCardServer()
	{
		m_obj->Release();
	}

	List<Homesys::Local::SmartCardDevice^>^ SmartCardServer::GetSmartCardDevices()
	{
		List<Homesys::Local::SmartCardDevice^>^ dst = gcnew List<Homesys::Local::SmartCardDevice^>();

		std::vector<::SmartCardDevice> src;

		m_obj->GetDevices(src);

		for(auto i = src.begin(); i != src.end(); i++)
		{
			Homesys::Local::SmartCardDevice^ dev = gcnew Homesys::Local::SmartCardDevice();

			dev->name = Translate(i->name.c_str());
			dev->inserted = i->inserted;
			dev->serial = Translate(i->serial.c_str());
			dev->systemId = i->system.id;
			dev->systemName = Translate(i->system.name.c_str());
			dev->subscriptions = gcnew List<Homesys::Local::SmartCardSubscription^>();

			for(auto j = i->subscriptions.begin(); j != i->subscriptions.end(); j++)
			{
				Homesys::Local::SmartCardSubscription^ s = gcnew Homesys::Local::SmartCardSubscription();

				s->id = j->id;
				s->name = Translate(j->name.c_str());
				s->date = gcnew List<DateTime>();

				for(auto k = j->date.begin(); k != j->date.end(); k++)
				{
					s->date->Add(Translate(*k));
				}

				dev->subscriptions->Add(s);
			}

			dst->Add(dev);
		}

		return dst;
	}

	//

	CoInitSec::CoInitSec()
	{
		CoInitializeSecurity(0, -1, 0, 0, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_SECURE_REFS, 0);
	}
}
