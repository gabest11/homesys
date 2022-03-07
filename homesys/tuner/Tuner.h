// tuner.h

#pragma once

#include "TunerDevice.h"
#include "TuneRequest.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace Homesys::Local;

namespace Homesys
{
	public ref class TunerProgram
	{
	public:
		String^ provider;
		String^ name;
		long sid;
		bool scrambled;
	};

	public ref class SmartCardServer
	{
	public:
		ISmartCardServer* m_obj;

	public:
		SmartCardServer();
		~SmartCardServer();
		!SmartCardServer();

		List<Homesys::Local::SmartCardDevice^>^ GetSmartCardDevices();
	};

	public ref class Tuner
	{
		Guid m_id;
		GenericTuner* m_tuner;
		String^ m_fn;
		int m_version;
		DateTime m_idle;
		bool m_recording;
		Guid m_recordingId;
		String^ m_recordingPath;
		int m_presetId;
		bool m_dummy;
		
		static unsigned int s_counter = 0;
		static int GetCounter();

		GenericPresetParam* Convert(Local::TuneRequest^ tr);

	public:
		Tuner();
		~Tuner();
		!Tuner();

		static IEnumerable<TunerDevice^>^ EnumTuners();
		static IEnumerable<TunerDevice^>^ EnumTunersNoType();

		void Open(TunerDevice^ td, Guid id, SmartCardServer^ scs);
		void Close();

		bool Timeshift(String^ dir);
		bool Timeshift(String^ dir, Local::TuneRequest^ tr, int presetId);
		bool IsTimeshiftDummy() {return m_dummy;}

		String^ GetFileName();
		int GetFileVersion();

		IEnumerable<int>^ GetConnectorTypes();
		static String^ GetConnectorName(int type);
		static bool IsTunerConnectorType(int type);

		TunerStat^ GetStat();

		void Tune(Local::TuneRequest^ tr, int presetId);
		IEnumerable<TunerProgram^>^ GetPrograms();

		void ResetIdleTime();
		TimeSpan GetIdleTime();

		void StartRecording(String^ path, String^ format, String^ title, Guid recordingId);
		void StopRecording();
		bool IsRecording();
		Guid GetRecordingId();
		String^ GetRecordingPath();
		int GetPresetId();

		int GetPort();
	};

	public ref class CoInitSec
	{
	public:
		CoInitSec();
	};
}
