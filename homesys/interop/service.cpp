#include "stdafx.h"
#include "service.h"
#include "convert.h"
#include "managed_ptr.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Collections::Specialized;
using namespace System::Diagnostics;
using namespace System::IO;
using namespace System::ServiceModel;
using namespace System::ServiceProcess;
using namespace System::Net;
using namespace System::Xml;
using namespace System::Xml::Serialization;
using namespace System::Text;

using namespace msclr;

#define BEGIN_CALL_WITH_TIMEOUT(ms) \
	clock_t __start = clock(); \
	try { \
		(*m_service)->InnerChannel->OperationTimeout = TimeSpan(10000ll * ms); \

#define BEGIN_CALL BEGIN_CALL_WITH_TIMEOUT(60000)

#define END_CALL \
	} \
	catch(Exception^ e) \
	{ \
		Console::WriteLine(e); \
	} \
	__finally \
	{ \
		clock_t __diff = clock() - __start; \
		if(__diff > 100) printf("%d %s\n", __diff, __FUNCTION__); \
	} \

namespace Homesys
{
	#pragma region Convert

	static void Convert(LocalService::TunerConnector^ src, TunerConnector& dst)
	{
		dst.id = src->id;
		dst.name = Convert(src->name);
		dst.num = src->num;
		dst.type = src->type;
	}

	static void Convert(const TunerConnector& src, LocalService::TunerConnector^ dst)
	{
		dst->id = src.id;
		dst->name = Convert(src.name);
		dst->num = src.num;
		dst->type = src.type;
	}

	static void Convert(LocalService::Channel^ src, Channel& dst)
	{
		dst.id = src->id;
		dst.name = Convert(src->name);
		dst.radio = src->radio;
		dst.rank = src->rank;

		if(src->logo && src->logo->Length > 0)
		{
			pin_ptr<BYTE> data = &src->logo[0];

			dst.logo.resize(src->logo->Length);

			memcpy(dst.logo.data(), data, src->logo->Length);
		}
	}

	static Cast Convert(LocalService::Cast^ src)
	{
		Cast dst;

		dst.id = src->id;
		dst.name = Convert(src->name);
		dst.movieName = Convert(src->movieName);

		return dst;
	}

	static void Convert(LocalService::Movie^ src, Movie& dst)
	{
		dst.id = src->id;
		dst.title = Convert(src->title);
		dst.desc = Convert(src->desc);
		dst.rating = src->rating;
		dst.year = src->year;
		dst.episodeCount = src->episodeCount;
		dst.movieTypeShort = Convert(src->movieTypeShort);
		dst.movieTypeLong = Convert(src->movieTypeLong);
		dst.dvbCategory = Convert(src->dvbCategory);
		dst.favorite = src->favorite;
		dst.hasPicture = src->hasPicture;
	}

	static void Convert(LocalService::Episode^ src, Episode& dst)
	{
		dst.id = src->id;
		dst.title = Convert(src->title);
		dst.desc = Convert(src->desc);
		dst.episodeNumber = src->episodeNumber;

		if(src->movie != nullptr)
		{
			Convert(src->movie, dst.movie);
		}

		dst.actors.clear();

		if(src->actors != nullptr)
		{
			for each(LocalService::Cast^ c in src->actors)
			{
				dst.actors.push_back(Convert(c));
			}
		}

		dst.directors.clear();

		if(src->directors != nullptr)
		{
			for each(LocalService::Cast^ c in src->directors)
			{
				dst.directors.push_back(Convert(c));
			}
		}
	}

	static void Convert(LocalService::Program^ src, Program& dst)
	{
		dst.id = src->id;
		dst.channelId = src->channelId;
		dst.start = Convert(src->start);
		dst.stop = Convert(src->stop);
		dst.state = (RecordingState::Type)src->state;

		if(src->episode != nullptr)
		{
			Convert(src->episode, dst.episode);
		}
	}

	static void Convert(LocalService::MovieType^ src, MovieType& dst)
	{
		dst.id = src->id;
		dst.nameShort = Convert(src->nameShort);
		dst.nameLong = Convert(src->nameLong);
	}

	static void Convert(LocalService::TuneRequest^ src, TuneRequest& dst)
	{
		dst.freq = src->freq;
		dst.standard = src->standard;
		if(src->connector != nullptr) Convert(src->connector, dst.connector);
		dst.sid = src->sid;
		dst.ifec = src->ifec;
		dst.ifecRate = src->ifecRate;
		dst.ofec = src->ofec;
		dst.ofecRate = src->ofecRate;
		dst.modulation = src->modulation;
		dst.symbolRate = src->symbolRate;
		dst.polarisation = src->polarisation;
		dst.west = src->west;
		dst.orbitalPosition = src->orbitalPosition;
		dst.azimuth = src->azimuth;
		dst.elevation = src->elevation;
		dst.lnbSource = src->lnbSource;
		dst.lowOscillator = src->lowOscillator;
		dst.highOscillator = src->highOscillator;
		dst.lnbSwitch = src->lnbSwitch;
		dst.spectralInversion = src->spectralInversion;
		dst.bandwidth = src->bandwidth;
		dst.lpifec = src->lpifec;
		dst.lpifecRate = src->lpifecRate;
		dst.halpha = src->halpha;
		dst.guard = src->guard;
		dst.transmissionMode = src->transmissionMode;
		dst.otherFreqInUse = src->otherFreqInUse;
		dst.path = Convert(src->path);
	}

	static void Convert(const TuneRequest& src, LocalService::TuneRequest^ dst)
	{
		dst->freq = src.freq;
		dst->standard = src.standard;
		dst->connector = gcnew LocalService::TunerConnector();
		Convert(src.connector, dst->connector);
		dst->sid = src.sid;
		dst->ifec = src.ifec;
		dst->ifecRate = src.ifecRate;
		dst->ofec = src.ofec;
		dst->ofecRate = src.ofecRate;
		dst->modulation = src.modulation;
		dst->symbolRate = src.symbolRate;
		dst->polarisation = src.polarisation;
		dst->west = src.west;
		dst->orbitalPosition = src.orbitalPosition;
		dst->azimuth = src.azimuth;
		dst->elevation = src.elevation;
		dst->lnbSource = src.lnbSource;
		dst->lowOscillator = src.lowOscillator;
		dst->highOscillator = src.highOscillator;
		dst->lnbSwitch = src.lnbSwitch;
		dst->spectralInversion = src.spectralInversion;
		dst->bandwidth = src.bandwidth;
		dst->lpifec = src.lpifec;
		dst->lpifecRate = src.lpifecRate;
		dst->halpha = src.halpha;
		dst->guard = src.guard;
		dst->transmissionMode = src.transmissionMode;
		dst->otherFreqInUse = src.otherFreqInUse;
		dst->path = !src.path.empty() ? Convert(src.path) : nullptr;
	}

	static void Convert(LocalService::Preset^ src, Preset& dst)
	{
		dst.id = src->id;
		dst.channelId = src->channelId;
		dst.provider = Convert(src->provider);
		dst.name = Convert(src->name);
		dst.scrambled = src->scrambled;
		dst.enabled = src->enabled;

		Convert(src->tunereq, dst.tunereq);
	}

	static void Convert(const Preset& src, LocalService::Preset^ dst)
	{
		dst->id = src.id;
		dst->channelId = src.channelId;
		dst->provider = Convert(src.provider);
		dst->name = Convert(src.name);
		dst->scrambled = src.scrambled;
		dst->enabled = src.enabled;
		dst->tunereq = gcnew LocalService::TuneRequest();

		Convert(src.tunereq, dst->tunereq);
	}

	static void Convert(LocalService::WishlistRecording^ src, WishlistRecording& dst)
	{
		dst.id = Convert(src->id);
		dst.title = Convert(src->title);
		dst.desc = Convert(src->description);
		dst.actor = Convert(src->actor);
		dst.director = Convert(src->director);
		dst.channelId = src->channelId;
	}

	static void Convert(const WishlistRecording& src, LocalService::WishlistRecording^ dst)
	{
		dst->id = Convert(src.id);
		dst->title = Convert(src.title);
		dst->description = Convert(src.desc);
		dst->actor = Convert(src.actor);
		dst->director = Convert(src.director);
		dst->channelId = src.channelId;
	}

	#pragma endregion

    ref class ServiceClientPool
    {
		ref class Entry
		{
		public:
			LocalService::ServiceClient^ p;
			DateTime t;
		};

        Dictionary<int, Entry^>^ m_map;
		TimeSpan m_keepalive;

	public:
		ServiceClientPool()
        {
            m_map = gcnew Dictionary<int, Entry^>();
			m_keepalive = TimeSpan(0, 1, 30);
        }

		LocalService::ServiceClient^ operator -> ()
		{
			lock l(m_map);

			DateTime now = DateTime::Now;

			List<int>^ remove;

			for each(KeyValuePair<int, Entry^>^ pair in m_map)
			{
				if(now - pair->Value->t > m_keepalive)
				{
					if(remove == nullptr)
					{
						remove = gcnew List<int>();
					}

					pair->Value->p->Close();

					remove->Add(pair->Key);
				}
			}

			if(remove != nullptr)
			{
				for each(int threadId in remove)
				{
					m_map->Remove(threadId);
				}
			}

			int threadId = System::Threading::Thread::CurrentThread->ManagedThreadId;

            Entry^ e;

            if(!m_map->TryGetValue(threadId, e) || e->p->State == CommunicationState::Faulted)
            {
				m_map->Remove(threadId);

				e = gcnew Entry();

				NetNamedPipeBinding^ binding = gcnew NetNamedPipeBinding();
				EndpointAddress^ endpointAddress = gcnew EndpointAddress("net.pipe://localhost/homesys/service");

				binding->MaxReceivedMessageSize = Int32::MaxValue / 2;
				binding->ReaderQuotas->MaxArrayLength = Int32::MaxValue / 2;

				e->p = gcnew LocalService::ServiceClient(binding, endpointAddress);

	            m_map->Add(threadId, e);
            }

			e->t = now;

            return e->p;
        }
    };

	class Service::ManagedServicePtr : public managed_ptr<ServiceClientPool>
	{
	};

	Service::Service()
	{
		try
		{
			m_service = new ManagedServicePtr();
		}
		catch(Exception^ e)
		{
			Debug::WriteLine(e);
		}
	}

	Service::~Service()
	{
		delete m_service;
	}

	bool Service::IsConnected()
	{
		BEGIN_CALL

		(*m_service)->get_IsOnline(); // dummy low overhead call to make sure we are really connected

		return true;

		END_CALL

		return false;
	}

	bool Service::GetMachine(MachineReg& machine)
	{
		BEGIN_CALL

		if(LocalService::MachineReg^ m = (*m_service)->get_Machine())
		{
			machine.id = Convert(m->id);
			machine.name = Convert(m->name);
			machine.published = m->published;
			machine.downloadPath = Convert(m->downloadPath);
			machine.recordingPath = Convert(m->recordingPath);
			machine.recordingFormat = Convert(m->recordingFormat);
			machine.timeshiftPath = Convert(m->timeshiftPath);
			machine.timeshiftDur = Convert(m->timeshiftDur);
			machine.username = Convert(m->username);
			machine.password = Convert(m->password);

			return true;
		}

		END_CALL

		return false;
	}

	bool Service::IsOnline()
	{
		BEGIN_CALL

		return (*m_service)->get_IsOnline();

		END_CALL

		return false;
	}

	bool Service::GetUsername(std::wstring& username)
	{
		BEGIN_CALL

		username = Convert((*m_service)->get_Username());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetUsername(const std::wstring& username)
	{
		BEGIN_CALL

			(*m_service)->set_Username(Convert(username));

			return true;

		END_CALL

		return false;
	}

	bool Service::GetPassword(std::wstring& password)
	{
		BEGIN_CALL

		password = Convert((*m_service)->get_Password());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetPassword(const std::wstring& password)
	{
		BEGIN_CALL

		(*m_service)->set_Password(Convert(password));

		return true;

		END_CALL

		return false;
	}

	bool Service::GetLanguage(std::wstring& lang)
	{
		BEGIN_CALL

		lang = Convert((*m_service)->get_Language());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetLanguage(const std::wstring& lang)
	{
		BEGIN_CALL

		(*m_service)->set_Language(Convert(lang));

		return true;

		END_CALL

		return false;
	}

	bool Service::GetRegion(std::wstring& region)
	{
		BEGIN_CALL

		region = Convert((*m_service)->get_Region());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetRegion(const std::wstring& region)
	{
		BEGIN_CALL

		(*m_service)->set_Region(Convert(region));

		return true;

		END_CALL

		return false;
	}

	bool Service::GetRecordingPath(std::wstring& path)
	{
		BEGIN_CALL

		path = Convert((*m_service)->get_RecordingPath());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetRecordingPath(const std::wstring& path)
	{
		BEGIN_CALL

		(*m_service)->set_RecordingPath(Convert(path));

		return true;

		END_CALL

		return false;
	}

	bool Service::GetRecordingFormat(std::wstring& format)
	{
		BEGIN_CALL

		format = Convert((*m_service)->get_RecordingFormat());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetRecordingFormat(const std::wstring& format)
	{
		BEGIN_CALL

		(*m_service)->set_RecordingFormat(Convert(format));		

		return true;

		END_CALL

		return false;
	}

	bool Service::GetTimeshiftPath(std::wstring& path)
	{
		BEGIN_CALL

		path = Convert((*m_service)->get_TimeshiftPath());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetTimeshiftPath(const std::wstring& path)
	{
		BEGIN_CALL

		(*m_service)->set_TimeshiftPath(Convert(path));

		return true;

		END_CALL

		return false;
	}

	bool Service::GetTimeshiftDuration(CTimeSpan& dur)
	{
		BEGIN_CALL

		dur = Convert((*m_service)->get_TimeshiftDuration());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetTimeshiftDuration(const CTimeSpan& dur)
	{
		BEGIN_CALL

		(*m_service)->set_TimeshiftDuration(Convert(dur));

		return true;

		END_CALL

		return false;
	}

	bool Service::GetDownloadPath(std::wstring& path)
	{
		BEGIN_CALL

		path = Convert((*m_service)->get_DownloadPath());

		return true;

		END_CALL

		return false;
	}

	bool Service::SetDownloadPath(const std::wstring& path)
	{
		BEGIN_CALL

		(*m_service)->set_DownloadPath(Convert(path));

		return true;

		END_CALL

		return false;
	}

	bool Service::GetAllTuners(std::list<TunerDevice>& tuners)
	{
		tuners.clear();

		BEGIN_CALL

		array<LocalService::TunerDevice^>^ tds = (*m_service)->get_AllTuners();

		for each(LocalService::TunerDevice^ src in tds)
		{
			TunerDevice dst;

			dst.dispname = Convert(src->dispname);
			dst.name = Convert(src->name);
			dst.type = (TunerDevice::Type)src->type;

			tuners.push_back(dst);
		}

		return true;

		END_CALL

		tuners.clear();

		return false;
	}

	bool Service::GetTuners(std::list<TunerReg>& tuners)
	{
		tuners.clear();

		BEGIN_CALL

		array<LocalService::TunerReg^>^ trs = (*m_service)->get_Tuners();

		for each(LocalService::TunerReg^ src in trs)
		{
			TunerReg dst;

			dst.id = Convert(src->id);
			dst.dispname = Convert(src->dispname);
			dst.name = Convert(src->name);
			dst.type = (TunerDevice::Type)src->type;

			for each(LocalService::TunerConnector^ c in src->connectors)
			{
				TunerConnector connector;

				Convert(c, connector);

				dst.connectors.push_back(connector);
			}

			tuners.push_back(dst);
		}

		return true;

		END_CALL

		tuners.clear();

		return false;
	}

	bool Service::HasTunerType(int type)
	{
		std::list<TunerReg> tuners;

		GetTuners(tuners);

		for(auto i = tuners.begin(); i != tuners.end(); i++)
		{
			if(i->type == type)
			{
				return true;
			}
		}

		return false;
	}

	bool Service::GetSmartCards(std::list<SmartCardDevice>& cards)
	{
		cards.clear();

		BEGIN_CALL

		array<LocalService::SmartCardDevice^>^ cs = (*m_service)->get_SmartCards();

		for each(LocalService::SmartCardDevice^ c in cs)
		{
			SmartCardDevice card;

			card.name = Convert(c->name);
			card.inserted = c->inserted;
			card.serial = Convert(c->serial);
			card.system.id = c->systemId;
			card.system.name = Convert(c->systemName);
			
			for each(LocalService::SmartCardSubscription^ s in c->subscriptions)
			{
				SmartCardSubscription subscription;

				subscription.id = s->id;
				subscription.name = Convert(s->name);

				for each(DateTime dt in s->date)
				{
					subscription.date.push_back(Convert(dt));
				}

				card.subscriptions.push_back(subscription);
			}

			cards.push_back(card);
		}

		return true;

		END_CALL
		
		cards.clear();

		return false;
	}

	bool Service::GetChannels(std::list<Channel>& channels)
	{
		channels.clear();

		BEGIN_CALL

		array<LocalService::Channel^>^ cs = (*m_service)->get_Channels();

		for each(LocalService::Channel^ c in cs)
		{
			Channel channel;

			Convert(c, channel);

			channels.push_back(channel);
		}

		return true;

		END_CALL
		
		channels.clear();

		return false;
	}

	bool Service::GetRecordings(std::list<Recording>& recordings)
	{
		recordings.clear();

		BEGIN_CALL

		array<LocalService::Recording^>^ rs = (*m_service)->get_Recordings();

		for each(LocalService::Recording^ src in rs)
		{
			Recording dst;

			dst.id = Convert(src->id);
			dst.webId = src->webId;
			dst.name = Convert(src->name);
			dst.start = Convert(src->start);
			dst.stop = Convert(src->stop);
			dst.startDelay = Convert(src->startDelay);
			dst.stopDelay = Convert(src->stopDelay);
			dst.state = (RecordingState::Type)src->state;
			dst.channelId = src->channelId;
			dst.path = Convert(src->path);
			dst.rating = src->rating;

			if(src->program)
			{
				Convert(src->program, dst.program);
			}

			if(src->preset)
			{
				Convert(src->preset, dst.preset);
			}

			recordings.push_back(dst);
		}

		return true;

		END_CALL

		recordings.clear();

		return false;
	}

	bool Service::GetWishlistRecordings(std::list<WishlistRecording>& recordings)
	{
		recordings.clear();

		BEGIN_CALL

		array<LocalService::WishlistRecording^>^ rs = (*m_service)->get_WishlistRecordings();

		for each(LocalService::WishlistRecording^ src in rs)
		{
			WishlistRecording dst;

			Convert(src, dst);

			recordings.push_back(dst);
		}

		return true;

		END_CALL

		recordings.clear();

		return false;
	}

	bool Service::GetParentalSettings(ParentalSettings& ps)
	{
		BEGIN_CALL

		LocalService::ParentalSettings^ src = (*m_service)->get_Parental();

		ps.password = Convert(src->password);
		ps.enabled = src->enabled;
		ps.rating = src->rating;
		ps.inactivity = src->inactivity;

		return true;

		END_CALL
		
		return false;
	}

	bool Service::SetParentalSettings(const ParentalSettings& ps)
	{
		BEGIN_CALL

		LocalService::ParentalSettings^ dst = gcnew LocalService::ParentalSettings();

		dst->password = Convert(ps.password);
		dst->enabled = ps.enabled;
		dst->rating = ps.rating;
		dst->inactivity = ps.inactivity;

		(*m_service)->set_Parental(dst);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetMostRecentVersion(AppVersion& version)
	{
		BEGIN_CALL

		LocalService::AppVersion^ v = (*m_service)->get_MostRecentVersion();

		version.version = v->version;
		version.url = Convert(v->url);
		version.required = v->required;

		return true;

		END_CALL

		return false;
	}

    bool Service::GetInstalledVersion(UINT64& version)
	{
		BEGIN_CALL

		version = (*m_service)->get_InstalledVersion();

		return true;

		END_CALL
		
		return false;
	}

    bool Service::GetLastGuideUpdate(CTime& t)
	{
		BEGIN_CALL

		t = Convert((*m_service)->get_LastGuideUpdate());

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetLastGuideError(std::wstring& error)
	{
		BEGIN_CALL

		error = Convert((*m_service)->get_LastGuideError());

		return true;

		END_CALL
		
		return false;
	}

    bool Service::GetGuideUpdatePercent(int& percent)
	{
		BEGIN_CALL

		percent = (*m_service)->get_GuideUpdatePercent();

		return true;

		END_CALL
		
		return false;
	}

	bool Service::RegisterUser(LPCWSTR username, LPCWSTR password, LPCWSTR email)
	{
		BEGIN_CALL

		(*m_service)->RegisterUser(Convert(username), Convert(password), Convert(email));

		return true;

		END_CALL

		return false;
	}

    bool Service::UserExists(LPCWSTR username)
	{
		BEGIN_CALL

		return (*m_service)->UserExists(Convert(username));

		END_CALL
		
		return false;
	}

    bool Service::LoginExists(LPCWSTR username, LPCWSTR password)
	{
		BEGIN_CALL

		return (*m_service)->LoginExists(Convert(username), Convert(password));

		END_CALL
		
		return false;
	}

	bool Service::RegisterMachine(LPCWSTR name, const std::list<TunerDevice>& devices)
	{
		BEGIN_CALL

		List<LocalService::TunerDevice^>^ tds = gcnew List<LocalService::TunerDevice^>();

		for(auto i = devices.begin(); i != devices.end(); i++)
		{
			LocalService::TunerDevice^ td = gcnew LocalService::TunerDevice();

			td->dispname = Convert(i->dispname);
			td->name = Convert(i->name);
			td->type = (LocalService::TunerDeviceType)i->type;

			tds->Add(td);
		}

		(*m_service)->RegisterMachine(Convert(name), tds->ToArray());

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetUserInfo(UserInfo& info)
	{
		BEGIN_CALL

		LocalService::UserInfo^ src = (*m_service)->GetUserInfo();

		info.email = Convert(src->email);
		info.lastName = Convert(src->lastName);
		info.firstName = Convert(src->firstName);
		info.country = Convert(src->country);
		info.address = Convert(src->address);
		info.postalCode = Convert(src->postalCode);
		info.phoneNumber = Convert(src->phoneNumber);
		info.gender = Convert(src->gender);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::SetUserInfo(const UserInfo& info)
	{
		BEGIN_CALL

		LocalService::UserInfo^ dst = gcnew LocalService::UserInfo();

		dst->email = Convert(info.email);
		dst->lastName = Convert(info.lastName);
		dst->firstName = Convert(info.firstName);
		dst->country = Convert(info.country);
		dst->address = Convert(info.address);
		dst->postalCode = Convert(info.postalCode);
		dst->phoneNumber = Convert(info.phoneNumber);
		dst->gender = Convert(info.gender);
		
		(*m_service)->SetUserInfo(dst);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::HasUserInfo()
	{
		UserInfo info;

		if(GetUserInfo(info))
		{
			if(!info.email.empty()
			&& !info.lastName.empty()
			&& !info.firstName.empty()
			&& !info.country.empty()
			&& !info.address.empty()
			&& !info.postalCode.empty()
			&& !info.gender.empty())
			{
				return true;
			}

		}

		return false;
	}

	bool Service::GoOnline()
	{
		BEGIN_CALL

		return (*m_service)->GoOnline();

		END_CALL
		
		return false;
	}

    bool Service::GetTuner(const GUID& tunerId, TunerReg& tuner)
	{
		BEGIN_CALL

		LocalService::TunerReg^ tr = (*m_service)->GetTuner(Convert(tunerId));

		tuner.id = Convert(tr->id);
		tuner.dispname = Convert(tr->dispname);
		tuner.name = Convert(tr->name);
		tuner.type = (TunerDevice::Type)tr->type;

		for each(LocalService::TunerConnector^ c in tr->connectors)
		{
			TunerConnector connector;

			Convert(c, connector);

			tuner.connectors.push_back(connector);
		}

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetTunerConnectors(const GUID& tunerId, std::list<TunerConnector>& connectors)
	{
		connectors.clear();

		BEGIN_CALL

		array<LocalService::TunerConnector^>^ cs = (*m_service)->GetTunerConnectors(Convert(tunerId));

		for each(LocalService::TunerConnector^ c in cs)
		{
			TunerConnector connector;

			Convert(c, connector);

			connectors.push_back(connector);
		}

		return true;

		END_CALL
		
		connectors.clear();

		return false;
	}

    bool Service::GetTunerStat(const GUID& tunerId, TunerStat& stat)
	{
		BEGIN_CALL

		if(LocalService::TunerStat^ src = (*m_service)->GetTunerStat(Convert(tunerId)))
		{
			stat.freq = src->frequency;
			stat.locked = src->locked;
			stat.present = src->present;
			stat.strength = src->strength;
			stat.quality = src->quality;
			stat.received = src->received;
			stat.scrambled = src->scrambled;
		}

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetPresets(const GUID& tunerId, bool enabled, std::list<Preset>& presets)
	{
		BEGIN_CALL

		array<LocalService::Preset^>^ ps = (*m_service)->GetPresets(Convert(tunerId), enabled);

		for each(LocalService::Preset^ p in ps)
		{
			Preset preset;

			Convert(p, preset);

			presets.push_back(preset);
		}

		return true;

		END_CALL
		
		return false;
	}

	bool Service::SetPresets(const GUID& tunerId, const std::list<int>& presetIds)
	{
		BEGIN_CALL

		List<int>^ ids = gcnew List<int>();

		for(auto i = presetIds.begin(); i != presetIds.end(); i++)
		{
			ids->Add(*i);
		}

		(*m_service)->SetPresets(Convert(tunerId), ids->ToArray());

		return true;

		END_CALL
		
		return false;
	}

    bool Service::GetPreset(int presetId, Preset& preset)
	{
		BEGIN_CALL

		LocalService::Preset^ p = (*m_service)->GetPreset(presetId);

		if(p != nullptr)
		{
			Convert(p, preset);

			return true;
		}

		END_CALL

		return false;
	}

    bool Service::GetCurrentPreset(const GUID& tunerId, Preset& preset)
	{
		BEGIN_CALL

		LocalService::Preset^ p = (*m_service)->GetCurrentPreset(Convert(tunerId));

		if(p != nullptr)
		{
			Convert(p, preset);

			return true;
		}

		END_CALL
		
		return false;
	}

    bool Service::GetCurrentPresetId(const GUID& tunerId, int& presetId)
	{
		BEGIN_CALL

		presetId = (*m_service)->GetCurrentPresetId(Convert(tunerId));

		return true;

		END_CALL
		
		return false;
	}

    bool Service::SetPreset(const GUID& tunerId, int presetId)
	{
		BEGIN_CALL

		(*m_service)->SetPreset(Convert(tunerId), presetId);

		return true;

		END_CALL
		
		return false;
	}

    bool Service::UpdatePreset(const Preset& preset)
	{
		BEGIN_CALL

		LocalService::Preset^ p = gcnew LocalService::Preset();

		Convert(preset, p);

		(*m_service)->UpdatePreset(p);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::UpdatePresets(const std::list<Preset>& presets)
	{
		BEGIN_CALL

		List<LocalService::Preset^>^ ps = gcnew List<LocalService::Preset^>;

		for(auto i = presets.begin(); i != presets.end(); i++)
		{
			LocalService::Preset^ p = gcnew LocalService::Preset();

			Convert(*i, p);

			ps->Add(p);
		}

		(*m_service)->UpdatePresets(ps->ToArray());

		return true;

		END_CALL
		
		return false;
	}

    bool Service::CreatePreset(const GUID& tunerId, Preset& preset)
	{
		BEGIN_CALL

		LocalService::Preset^ p = (*m_service)->CreatePreset(Convert(tunerId));

		Convert(p, preset);

		return true;

		END_CALL

		return false;
	}

    bool Service::DeletePreset(int presetId)
	{
		BEGIN_CALL

		(*m_service)->DeletePreset(presetId);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetChannels(const GUID& tunerId, std::list<Channel>& channels)
	{
		channels.clear();

		BEGIN_CALL

		array<LocalService::Channel^>^ cs = (*m_service)->GetChannels(Convert(tunerId));

		for each(LocalService::Channel^ c in cs)
		{
			Channel channel;

			Convert(c, channel);

			channels.push_back(channel);
		}

		return true;

		END_CALL
		
		channels.clear();

		return false;
	}

    bool Service::GetChannel(int channelId, Channel& channel)
	{
		BEGIN_CALL

		if(channelId > 0)
		{
			LocalService::Channel^ c = (*m_service)->GetChannel(channelId);

			if(c != nullptr)
			{
				Convert(c, channel);

				return true;
			}
		}

		// END_CALL
		
	} 
	catch(Exception^ e) 
	{ 
		Debug::WriteLine(e); 
	} 
	__finally 
	{ 
		clock_t __diff = clock() - __start; 
		if(__diff > 100) printf("%d %s\n", __diff, __FUNCTION__); 
	} 
		return false;
	}

	bool Service::GetPrograms(const GUID& tunerId, int channelId, const CTime& start, const CTime& stop, std::list<Program>& programs)
	{
		programs.clear();

		BEGIN_CALL

		array<LocalService::Program^>^ ps = (*m_service)->GetPrograms(Convert(tunerId), channelId, Convert(start), Convert(stop));

		if(ps != nullptr)
		{
			for each(LocalService::Program^ p in ps)
			{
				Program program;

				Convert(p, program);

				programs.push_back(program);
			}

			return true;
		}

		END_CALL

		programs.clear();

		return false;
	}

    bool Service::GetProgram(int programId, Program& program)
	{
		BEGIN_CALL

		Convert((*m_service)->GetProgram(programId), program);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetCurrentProgram(const GUID& tunerId, int channelId, Program& program, CTime now)
	{
		program = Program();

		Preset preset;

		if(tunerId != GUID_NULL && channelId == 0)
		{
			if(!GetCurrentPreset(tunerId, preset))
			{
				return false;
			}

			channelId = preset.channelId;
		}

		if(channelId > 0)
		{
			if(now == 0)
			{
				now = CTime::GetCurrentTime();
			}

			std::list<Program> programs;

			if(GetPrograms(tunerId, channelId, now, now, programs) && !programs.empty())
			{
				program = programs.front();

				return true;
			}
		}
		else if(preset.id > 0)
		{
			program.id = INT_MAX;
			program.channelId = 0;
			program.start = program.stop = 0;
			program.state = RecordingState::NotScheduled;
			program.episode.id = 0;
			program.episode.movie.id = 0;
			program.episode.movie.title = preset.name;

			return true;
		}

		return false;
	}

	bool Service::GetFavoritePrograms(int count, bool picture, std::list<Program>& programs)
	{
		programs.clear();

		BEGIN_CALL

		array<LocalService::Program^>^ ps = (*m_service)->GetFavoritePrograms(count, picture);

		if(ps != nullptr)
		{
			for each(LocalService::Program^ p in ps)
			{
				Program program;

				Convert(p, program);

				programs.push_back(program);
			}

			return true;
		}

		END_CALL

		programs.clear();

		return false;
	}

	bool Service::GetRelatedPrograms(int programId, std::list<Program>& programs)
	{
		programs.clear();

		BEGIN_CALL

		array<LocalService::Program^>^ ps = (*m_service)->GetRelatedPrograms(programId);

		if(ps != nullptr)
		{
			for each(LocalService::Program^ p in ps)
			{
				Program program;

				Convert(p, program);

				programs.push_back(program);
			}

			return true;
		}

		END_CALL
		
		programs.clear();

		return false;
	}

    bool Service::GetRecommendedMovieTypes(std::list<MovieType>& movieTypes)
	{
		BEGIN_CALL

		array<LocalService::MovieType^>^ mts = (*m_service)->GetRecommendedMovieTypes();

		if(mts != nullptr)
		{
			for each(LocalService::MovieType^ mt in mts)
			{
				MovieType movieType;

				Convert(mt, movieType);

				movieTypes.push_back(movieType);
			}

			return true;
		}

		END_CALL
		
		return false;
	}

    bool Service::GetRecommendedProgramsByMovieType(int movieTypeId, std::list<Program>& programs)
	{
		programs.clear();

		BEGIN_CALL

		array<LocalService::Program^>^ ps = (*m_service)->GetRecommendedProgramsByMovieType(movieTypeId);

		if(ps != nullptr)
		{
			for each(LocalService::Program^ p in ps)
			{
				Program program;

				Convert(p, program);

				programs.push_back(program);
			}

			return true;
		}

		END_CALL
		
		programs.clear();

		return false;
	}

    bool Service::GetRecommendedProgramsByChannel(int channelId, std::list<Program>& programs)
	{
		programs.clear();

		BEGIN_CALL

		array<LocalService::Program^>^ ps = (*m_service)->GetRecommendedProgramsByChannel(channelId);

		if(ps != nullptr)
		{
			for each(LocalService::Program^ p in ps)
			{
				Program program;

				Convert(p, program);

				programs.push_back(program);
			}

			return true;
		}

		END_CALL
		
		programs.clear();

		return false;
	}

    bool Service::GetEpisode(int episodeId, Episode& episode)
	{
		BEGIN_CALL

		Convert((*m_service)->GetEpisode(episodeId), episode);

		return true;

		END_CALL
		
		return false;
	}

    bool Service::GetMovie(int movieId, Movie& movie)
	{
		BEGIN_CALL

		Convert((*m_service)->GetMovie(movieId), movie);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::SearchPrograms(LPCWSTR query, ProgramSearch::Type type, std::list<ProgramSearchResult>& results)
	{
		results.clear();

		BEGIN_CALL

		array<LocalService::ProgramSearchResult^>^ psrs = (*m_service)->SearchPrograms(Convert(query), (LocalService::ProgramSearchType)type);

		if(psrs == nullptr)
		{
			return false;
		}

		for each(LocalService::ProgramSearchResult^ prs in psrs)
		{
			ProgramSearchResult result;

			Convert(prs->program, result.program);

			for each(String^ match in prs->matches)
			{
				result.matches.push_back(Convert(match));
			}

			results.push_back(result);
		}

		return true;

		END_CALL
		
		results.clear();

		return false;
	}

    bool Service::SearchProgramsByMovie(int movieId, std::list<Program>& programs)
	{
		programs.clear();

		BEGIN_CALL

		array<LocalService::Program^>^ ps = (*m_service)->SearchProgramsByMovie(movieId);

		if(ps == nullptr)
		{
			return false;
		}

		for each(LocalService::Program^ p in ps)
		{
			Program program;

			Convert(p, program);

			programs.push_back(program);
		}

		return true;

		END_CALL
		
		programs.clear();

		return false;
	}

	bool Service::GetRecordings(RecordingState::Type state, std::list<Recording>& recordings)
	{
		recordings.clear();

		BEGIN_CALL

		array<LocalService::Recording^>^ rs = (*m_service)->GetRecordings((LocalService::RecordingState)state);

		if(rs == nullptr)
		{
			return false;
		}

		for each(LocalService::Recording^ src in rs)
		{
			Recording dst;

			dst.id = Convert(src->id);
			dst.webId = src->webId;
			dst.name = Convert(src->name);
			dst.start = Convert(src->start);
			dst.stop = Convert(src->stop);
			dst.startDelay = Convert(src->startDelay);
			dst.stopDelay = Convert(src->stopDelay);
			dst.state = (RecordingState::Type)src->state;
			dst.channelId = src->channelId;
			dst.path = Convert(src->path);
			dst.rating = src->rating;

			if(src->program)
			{
				Convert(src->program, dst.program);
			}

			if(src->preset)
			{
				Convert(src->preset, dst.preset);
			}

			recordings.push_back(dst);
		}

		return true;

		END_CALL

		recordings.clear();

		return false;
	}

    bool Service::GetRecording(const GUID& recordingId, Recording& recording)
	{
		BEGIN_CALL

		LocalService::Recording^ r = (*m_service)->GetRecording(Convert(recordingId));

		recording.id = Convert(r->id);
		recording.webId = r->webId;
		recording.name = Convert(r->name);
		recording.start = Convert(r->start);
		recording.stop = Convert(r->stop);
		recording.startDelay = Convert(r->startDelay);
		recording.stopDelay = Convert(r->stopDelay);
		recording.state = (RecordingState::Type)r->state;
		recording.channelId = r->channelId;
		recording.path = Convert(r->path);
		recording.rating = r->rating;

		if(r->program)
		{
			Convert(r->program, recording.program);
		}

		if(r->preset)
		{
			Convert(r->preset, recording.preset);
		}

		return true;

		END_CALL
		
		return false;
	}

    bool Service::UpdateRecording(const Recording& recording)
	{
		BEGIN_CALL

		LocalService::Recording^ r = gcnew LocalService::Recording();

		r->id = Convert(recording.id);
		// r->webId = recording.webId;
		r->name = Convert(recording.name);
		r->start = Convert(recording.start);
		r->stop = Convert(recording.stop);
		r->startDelay = Convert(recording.startDelay);
		r->stopDelay = Convert(recording.stopDelay);
		// r->state = (LocalService::RecordingState)recording.state;
		// r->path = recording.path;

		if(recording.program.id)
		{
			r->program = gcnew LocalService::Program();
			r->program->id = recording.program.id;
		}

		if(recording.preset.id)
		{
			r->preset = gcnew LocalService::Preset();
			r->preset->id = recording.preset.id;
		}

		(*m_service)->UpdateRecording(r);

		return true;

		END_CALL
		
		return false;
	}

    bool Service::DeleteRecording(const GUID& recordingId)
	{
		BEGIN_CALL

		(*m_service)->DeleteRecording(Convert(recordingId));

		return true;

		END_CALL
		
		return false;
	}

    bool Service::DeleteMovieRecording(int movieId)
	{
		BEGIN_CALL

		(*m_service)->DeleteMovieRecording(movieId);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::GetProgramsForWishlist(const WishlistRecording& recording, std::list<Program>& programs)
	{
		programs.clear();

		BEGIN_CALL

		LocalService::WishlistRecording^ r = gcnew LocalService::WishlistRecording();

		Convert(recording, r);

		for each(LocalService::Program^ p in (*m_service)->GetProgramsForWishlist(r))
		{
			Program program;

			Convert(p, program);

			programs.push_back(program);
		}

		return true;

		END_CALL

		programs.clear();
		
		return false;
	}

	bool Service::GetWishlistRecording(const GUID& wishlistRecordingId, WishlistRecording& recording)
	{
		BEGIN_CALL

		LocalService::WishlistRecording^ r = (*m_service)->GetWishlistRecording(Convert(wishlistRecordingId));

		Convert(r, recording);

		return true;

		END_CALL
		
		return false;
	}

	bool Service::DeleteWishlistRecording(const GUID& wishlistRecordingId)
	{
		BEGIN_CALL

		(*m_service)->DeleteWishlistRecording(Convert(wishlistRecordingId));

		return true;

		END_CALL
		
		return false;
	}

    bool Service::RecordByProgram(int programId, bool warnOnly, GUID& recordingId)
	{
		BEGIN_CALL

		recordingId = Convert((*m_service)->RecordByProgram(programId, warnOnly));

		return !(recordingId == GUID_NULL);

		END_CALL
		
		return false;
	}

    bool Service::RecordByPreset(int presetId, const CTime& start, const CTime& stop, GUID& recordingId)
	{
		BEGIN_CALL

		recordingId = Convert((*m_service)->RecordByPreset(presetId, Convert(start), Convert(stop)));

		return !(recordingId == GUID_NULL);

		END_CALL
		
		return false;
	}

    bool Service::RecordByMovie(int movieId, int channelId, GUID& movieRecordingId)
	{
		BEGIN_CALL

		movieRecordingId = Convert((*m_service)->RecordByMovie(movieId, channelId));

		return !(movieRecordingId == GUID_NULL);

		END_CALL
		
		return false;
	}

	bool Service::RecordByWishlist(const WishlistRecording& recording, GUID& wishlistRecordingId)
	{
		BEGIN_CALL

		LocalService::WishlistRecording^ r = gcnew LocalService::WishlistRecording();

		Convert(recording, r);

		wishlistRecordingId = Convert((*m_service)->RecordByWishlist(r));

		return !(wishlistRecordingId == GUID_NULL);

		END_CALL
		
		return false;
	}

    bool Service::Record(const GUID& tunerId, GUID& recordingId)
	{
		BEGIN_CALL

		recordingId = Convert((*m_service)->Record(Convert(tunerId)));

		return !(recordingId == GUID_NULL);

		END_CALL
		
		return false;
	}

	bool Service::Record(const GUID& tunerId)
	{
		GUID recordingId;

		return Record(tunerId, recordingId);
	}

    bool Service::IsRecording(const GUID& tunerId, GUID& recordingId)
	{
		BEGIN_CALL

		recordingId = Convert((*m_service)->IsRecording(Convert(tunerId)));

		return !(recordingId == GUID_NULL);

		END_CALL
		
		return false;
	}

	bool Service::IsRecording(const GUID& tunerId)
	{
		GUID recordingId;

		return IsRecording(tunerId, recordingId);
	}

    bool Service::StopRecording(const GUID& tunerId)
	{
		BEGIN_CALL

		(*m_service)->StopRecording(Convert(tunerId));

		return true;

		END_CALL

		return false;
	}

    bool Service::Activate(const GUID& tunerId, TimeshiftFile& tf)
	{
		BEGIN_CALL

		LocalService::TimeshiftFile^ src = (*m_service)->Activate(Convert(tunerId));

		tf.path = Convert(src->path);
		tf.port = src->port;
		tf.version = src->version;

		return true;

		END_CALL
		
		return false;
	}

    bool Service::Deactivate(const GUID& tunerId)
	{
		BEGIN_CALL

		(*m_service)->Deactivate(Convert(tunerId));

		return true;

		END_CALL
		
		return false;
	}

    bool Service::DeactivateAll()
	{
		BEGIN_CALL

		(*m_service)->DeactivateAll();

		return true;

		END_CALL
		
		return false;
	}

    bool Service::SetPrimaryTuner(const GUID& tunerId, bool active)
	{
		BEGIN_CALL

		(*m_service)->SetPrimaryTuner(Convert(tunerId), active);

		return true;

		END_CALL
		
		return false;
	}

    bool Service::UpdateGuideNow(bool reset)
	{
		BEGIN_CALL

		(*m_service)->UpdateGuideNow(reset);

		return true;

		END_CALL
		
		return false;
	}

    bool Service::PopServiceMessage(ServiceMessage& msg)
	{
		msg = ServiceMessage();

		BEGIN_CALL

		LocalService::ServiceMessage^ src = (*m_service)->PopServiceMessage();

		if(src != nullptr)
		{
			msg.id = src->id;
			msg.message = Convert(src->message);

			return true;
		}

		END_CALL
		
		return false;
	}

    bool Service::GetTuneRequestPackages(const GUID& tunerId, std::list<TuneRequestPackage>& packages)
	{
		BEGIN_CALL

		for each(LocalService::TuneRequestPackage^ src in (*m_service)->GetTuneRequestPackages(Convert(tunerId)))
		{
			TuneRequestPackage dst;

			dst.id = src->id;
			dst.provider = Convert(src->provider);
			dst.name = Convert(src->name);

			packages.push_back(dst);
		}

		return true;

		END_CALL

		return false;
	}

	bool Service::StartTunerScanByPackage(const GUID& tunerId, int id)
	{
		BEGIN_CALL

		(*m_service)->StartTunerScanByPackage(Convert(tunerId), id);

		return true;

		END_CALL

		return false;
	}

	bool Service::StartTunerScan(const GUID& tunerId, const std::list<TuneRequest>& trs, LPCWSTR provider)
	{
		BEGIN_CALL

		array<LocalService::TuneRequest^>^ requests = gcnew array<LocalService::TuneRequest^>(trs.size());

		int index = 0;

		for(auto i = trs.begin(); i != trs.end(); i++, index++)
		{
			LocalService::TuneRequest^ r = gcnew LocalService::TuneRequest();

			Convert(*i, r);

			requests[index] = r;
		}

		(*m_service)->StartTunerScan(Convert(tunerId), requests, Convert(provider));

		return true;

		END_CALL
		
		return false;
	}

    bool Service::StopTunerScan()
	{
		BEGIN_CALL

		(*m_service)->StopTunerScan();

		return true;

		END_CALL
		
		return false;
	}

    bool Service::GetTunerScanResult(std::list<Preset>& presets)
	{
		presets.clear();

		BEGIN_CALL

		array<LocalService::Preset^>^ ps = (*m_service)->GetTunerScanResult();

		for each(LocalService::Preset^ p in ps)
		{
			Preset preset;
				
			Convert(p, preset);

			presets.push_back(preset);
		}

		return true;

		END_CALL

		presets.clear();
		
		return false;
	}

    bool Service::IsTunerScanDone()
	{
		BEGIN_CALL

		return (*m_service)->IsTunerScanDone();

		END_CALL
		
		return false;
	}

	bool Service::SaveTunerScanResult(const GUID& tunerId, std::list<Preset>& presets, bool append)
	{
		BEGIN_CALL_WITH_TIMEOUT(300000)

		array<LocalService::Preset^>^ ps = gcnew array<LocalService::Preset^>(presets.size());

		int index = 0;

		for(auto i = presets.begin(); i != presets.end(); i++, index++)
		{
			LocalService::Preset^ p = gcnew LocalService::Preset();

			Convert(*i, p);

			ps[index] = p;
		}

		(*m_service)->InnerChannel->OperationTimeout = TimeSpan(0, 5, 0);

		(*m_service)->SaveTunerScanResult(Convert(tunerId), ps, append);

		return true;

		END_CALL
		
		return false;
	}

	static DWORD WINAPI ThreadProcOpenHTML(LPVOID param)
	{
		LPCWSTR str = (LPCWSTR)param;

		HRESULT hr;

		hr = OleInitialize(0);
		
		{
			CComPtr<IWebBrowser2> browser;

			hr = browser.CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_SERVER);

			if(SUCCEEDED(hr) && browser != NULL)
			{
				hr = browser->put_Visible(VARIANT_FALSE);
				hr = browser->put_Silent(VARIANT_TRUE);

				CComVariant url(str);
				CComVariant empty;

				hr = browser->Navigate2(&url, &empty, &empty, &empty, &empty);

				if(SUCCEEDED(hr))
				{
					for(int i = 0; i < 10; i++)
					{
						VARIANT_BOOL busy;

						hr = browser->get_Busy(&busy);

						READYSTATE state;

						hr = browser->get_ReadyState(&state);

						if(busy == VARIANT_FALSE) break;

						Sleep(500);
					}
				}

				browser->Quit();
			}
		}

		OleUninitialize();

		delete [] str;

		return 0;
	}

	//

	ref class CallbackServiceClient : public LocalService::ICallbackServiceCallback
	{
		LocalService::CallbackServiceClient^ m_inner;
		ICallback* m_cb;

	public:
		CallbackServiceClient()
			: m_cb(NULL)
		{
			InstanceContext^ context = gcnew InstanceContext(this);
			NetNamedPipeBinding^ binding = gcnew NetNamedPipeBinding();
			EndpointAddress^ endpointAddress = gcnew EndpointAddress("net.pipe://localhost/homesys/service/callback");

			binding->MaxReceivedMessageSize = Int32::MaxValue / 2;
			binding->ReaderQuotas->MaxArrayLength = Int32::MaxValue / 2;

			m_inner = gcnew LocalService::CallbackServiceClient(context, binding, endpointAddress);
		}

		void Register(ICallback* cb)
		{
			m_cb = cb;

			m_inner->Register((int)GetCurrentProcessId());
		}

		void Unregister()
		{
			m_inner->Unregister((int)GetCurrentProcessId());
		}

		// ICallbackServiceCallback

		virtual void OnUserInput(int type, int param)
		{
			if(m_cb != NULL)
			{
				m_cb->OnUserInput((UserInputType)type, param);
			}
		}

        virtual void OnCurrentPresetChanged(Guid tunerId)
		{
			if(m_cb != NULL)
			{
				m_cb->OnCurrentPresetChanged(Convert(tunerId));
			}
		}
	};

	class CallbackService::ManagedCallbackServicePtr : public managed_ptr<CallbackServiceClient>
	{
	};

	CallbackService::CallbackService(ICallback* cb)
	{
		try
		{
			m_service = new ManagedCallbackServicePtr();

			(*m_service)->Register(cb);
		}
		catch(Exception^ e)
		{
			Debug::WriteLine(e);
		}
	}

	CallbackService::~CallbackService()
	{
		try
		{
			(*m_service)->Unregister();

			delete m_service;
		}
		catch(Exception^ e)
		{
			Debug::WriteLine(e);
		}
	}

	// TODO

	// HttpMediaDetector

	HttpMediaDetector::HttpMediaDetector()
	{
	}

	HttpMediaDetector::~HttpMediaDetector()
	{
	}

	bool HttpMediaDetector::Open(LPCWSTR url, HttpMedia& m)
	{
		try
		{
			HttpWebRequest^ req = (HttpWebRequest^)WebRequest::Create(Convert(url));

			req->Proxy = gcnew Web::EmptyWebProxy();

			HttpWebResponse^ res = (HttpWebResponse^)req->GetResponse();

			m.url = url;
			m.type = Convert(res->ContentType);
			m.length = res->ContentLength;

			{
				std::wstring::size_type i = m.type.find_last_of(';');
	
				if(i != std::wstring::npos)
				{
					m.type = m.type.substr(0, i);
				}
			}

			if(res->ContentType->Contains("video/x-ms-asf"))
			{
				try
				{
					XmlSerializer^ serializer = gcnew XmlSerializer(XML::ASX::ASX::typeid);

					XML::ASX::ASX^ asx = (XML::ASX::ASX^)serializer->Deserialize(res->GetResponseStream());

					if(asx->entries != nullptr)
					{
						for each(XML::ASX::Entry^ e in asx->entries)
						{
							if(e->refs != nullptr)
							{
								for each(XML::ASX::Ref^ r in e->refs)
								{
									try
									{
										HttpWebRequest^ req = (HttpWebRequest^)WebRequest::Create(r->href);

										req->Proxy = gcnew Web::EmptyWebProxy();
						
										HttpWebResponse^ res = (HttpWebResponse^)req->GetResponse();

										m.url = Convert(r->href) + L"?MSWMExt=.asf";

										break;									
									}
									catch(Exception^ e)
									{
										Console::WriteLine(e);
									}
								}
							}

							break;
						}
					}
				}
				catch(Exception^ e)
				{
					Console::WriteLine(e);
				}
			}
			else if(res->ContentType->Contains("video/x-ms-w"))
			{
				m.url += L"?MSWMExt=.asf"; // FIXME
			}
			else if(res->ContentType == "application/octet-stream")
			{
				if(m.length < 0)
				{
					Stream^ s = res->GetResponseStream();

					array<BYTE>^ buff = gcnew array<BYTE>(192);

					if(s->Read(buff, 0, buff->Length) == buff->Length)
					{
						if(buff[0] == 0x47 && buff[188] == 0x47)
						{
							m.type = L"video/x-homesys-ts";
						}
					}
				}
			}

			return true;
		}
		catch(Exception^ e)
		{
			Console::WriteLine(e);
		}

		return false;
	}
}