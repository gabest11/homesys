#pragma once

#include "TunerDevice.h"

using namespace System;
using namespace System::ServiceModel;
using namespace System::Runtime::Serialization;

namespace Homesys { namespace Local
{
    [DataContract]
	public ref class TuneRequest
	{
	public:
        [DataMember]
        int freq;

        [DataMember]
        int standard;

        [DataMember]
        TunerConnector^ connector;

		[DataMember]
		int sid;

		[DataMember]
		int ifec;

		[DataMember]
		int ifecRate;

		[DataMember]
		int ofec;

		[DataMember]
		int ofecRate;

		[DataMember]
		int modulation;

		[DataMember]
		int symbolRate;

		[DataMember]
		int polarisation;

		[DataMember]
		bool west;

		[DataMember]
		int orbitalPosition;

		[DataMember]
		int azimuth;

		[DataMember]
		int elevation;

		[DataMember]
		int lnbSource;

		[DataMember]
		int lowOscillator;

		[DataMember]
		int highOscillator;

		[DataMember]
		int lnbSwitch;

		[DataMember]
		int spectralInversion;

		[DataMember]
		int bandwidth;

		[DataMember]
		int lpifec;

		[DataMember]
		int lpifecRate;

		[DataMember]
		int halpha;

		[DataMember]
		int guard;

		[DataMember]
		int transmissionMode;

		[DataMember]
		bool otherFreqInUse;

		[DataMember]
		String^ path;

		TuneRequest^ Clone() {return (TuneRequest^)MemberwiseClone();}
	};
}}