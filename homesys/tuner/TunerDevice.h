#pragma once

using namespace System;
using namespace System::ServiceModel;
using namespace System::Runtime::Serialization;
using namespace System::Collections::Generic;

namespace Homesys { namespace Local
{
	[DataContract]
	public enum class TunerDeviceType
	{
	    [EnumMember]
	    None = -1,

	    [EnumMember]
	    Analog = 0,

	    [EnumMember]
	    DVBS = 1,

	    [EnumMember]
	    DVBT = 2,

	    [EnumMember]
	    DVBC = 3,

	    [EnumMember]
	    DVBF = 4,
	};

    [DataContract]
    public ref class TunerConnector
    {
	public:
		[DataMember]
		int id;

        [DataMember]
        int num;

        [DataMember]
        int type;

        [DataMember]
        String^ name;
    };

	[DataContract]
	public ref class TunerDevice
	{
	public:
	    [DataMember]
	    String^ dispname;

	    [DataMember]
	    String^ name;

	    [DataMember]
	    TunerDeviceType type;
	};

    [DataContract]
    public ref class TunerStat
    {
	public:
        [DataMember]
        int frequency;

        [DataMember]
		int present;

		[DataMember]
		int locked;

		[DataMember]
		int strength;

		[DataMember]
		int quality;

		[DataMember]
		__int64 received;

		[DataMember]
		bool scrambled;
    };

	[DataContract]
	public ref class SmartCardSubscription
	{
	public:
		[DataMember]
		unsigned short id;

		[DataMember]
		String^ name;

		[DataMember]
		List<DateTime>^ date;
	};

    [DataContract]
    public ref class SmartCardDevice
    {
	public:
		[DataMember]
		String^ name;

		[DataMember]
		bool inserted;

        [DataMember]
        String^ serial;

        [DataMember]
		unsigned short systemId;

        [DataMember]
		String^ systemName;

		[DataMember]
		List<SmartCardSubscription^>^ subscriptions;
    };
}}