<?php

class Channel
{
	/** @var int Id */

	public $Id;

	/** @var string Name */

	public $Name;

	/** @var string Url */

	public $Url;

	/** @var base64Binary Logo */

	public $Logo;

	/** @var boolean Radio */

	public $Radio;

	/** @var string Aliases */

	public $Aliases;

	public function __construct($id, $name, $url, $logo, $radio)
	{
		$this->Id = $id;
		$this->Name = $name;
		$this->Url = $url;
		$this->Logo = $logo;
		$this->Radio = !!$radio;
		$this->Aliases = array();
	}
}

class Program
{
	/** @var int Id */

	public $Id;

	/** @var int ChannelId */

	public $ChannelId;

	/** @var int EpisodeId */

	public $EpisodeId;

	/** @var int MovieId */

	public $MovieId;

	/** @var dateTime StartDate */

	public $StartDate;

	/** @var dateTime StopDate */

	public $StopDate;

	/** @var boolean IsRepeat */

	public $IsRepeat;

	/** @var boolean IsLive */

	public $IsLive;

	/** @var boolean IsLive */


	public $IsRecommended;

	/** @var boolean IsRecommended */

	public $Deleted;

	public function __construct($id, $channelId, $episodeId, $movieId, $startDate, $stopDate, $isRepeat, $isLive, $isRecommended, $deleted)
	{
		$this->Id = $id;
		$this->ChannelId = $channelId;
		$this->EpisodeId = $episodeId;
		$this->MovieId = $movieId;
		$this->StartDate = $startDate;
		$this->StopDate = $stopDate;
		$this->IsRepeat = $isRepeat;
		$this->IsLive = $isLive;
		$this->IsRecommended = $isRecommended;
		$this->Deleted = $deleted;
	}
}

class RelatedProgram
{
	/** @var int Id */

	public $Id;

	/** @var int OtherId */

	public $OtherId;

	/** @var int Type */

	public $Type;
}

class Episode
{
	/** @var int Id */

	public $Id;

	/** @var int MovieId */

	public $MovieId;

	/** @var string Title */

	public $Title;

	/** @var string Description */

	public $Description;

	/** @var int EpisodeNumber */

	public $EpisodeNumber;

	/** @var boolean Temp */

	public $Temp;

	/** @var EpisodeCast[] EpisodeCastList */

	public $EpisodeCastList;

	public function __construct($id, $movieId, $title, $description, $episodeNumber, $temp)
	{
		$this->Id = $id;
		$this->MovieId = $movieId;
		$this->Title = $title;
		$this->Description = $description;
		$this->EpisodeNumber = $episodeNumber;
		$this->Temp = $temp;
	}
}

class Movie
{
	/** @var int Id */

	public $Id;

	/** @var int MovieTypeId */

	public $MovieTypeId;

	/** @var int DVBCategoryId */

	public $DVBCategoryId;

	/** @var string Title */

	public $Title;

	/** @var string Description */

	public $Description;

	/** @var int Rating */

	public $Rating;

	/** @var int Year */

	public $Year;

	/** @var int EpisodeCount */

	public $EpisodeCount;

	/** @var boolean Temp */

	public $Temp;

	/** @var boolean HasPicture */

	public $HasPicture;

	public function __construct($id, $movieTypeId, $dvbCategoryId, $title, $description, $rating, $year, $episodeCount, $temp, $hasPicture)
	{
		$this->Id = $id;
		$this->MovieTypeId = $movieTypeId;
		$this->DVBCategoryId = $dvbCategoryId;
		$this->Title = $title;
		$this->Description = $description;
		$this->Rating = $rating;
		$this->Year = $year;
		$this->EpisodeCount = $episodeCount;
		$this->Temp = $temp;
		$this->HasPicture = $hasPicture;
	}
}

class EpisodeCast
{
	/** @var int CastId */

	public $CastId;

	/** @var int RoleId */

	public $RoleId;

	/** @var string Name */

	public $Name;

	public function __construct($castId, $roleId, $name)
	{
		$this->CastId = $castId;
		$this->RoleId = $roleId;
		$this->Name = !empty($name) ? $name : null;
	}
}

class Cast
{
	/** @var int Id */

	public $Id;

	/** @var string Name */

	public $Name;

	/** @var string MovieName */

	public $MovieName;

	public function __construct($id, $name)
	{
		$this->Id = $id;
		$this->Name = $name;
		$this->MovieName = '';
	}
}

class MovieType
{
	/** @var int Id */

	public $Id;

	/** @var string NameShort */

	public $NameShort;

	/** @var string NameLong */

	public $NameLong;

	public function __construct($id, $nameShort, $nameLong)
	{
		$this->Id = $id;
		$this->NameShort = $nameShort;
		$this->NameLong = $nameLong;
	}
}

class DVBCategory
{
	/** @var int Id */

	public $Id;

	/** @var string Name */

	public $Name;

	public function __construct($id, $name)
	{
		$this->Id = $id;
		$this->Name = $name;
	}
}

class TuneRequest
{
	/** @var int Frequency */

	public $Frequency;

	/** @var int VideoStandard */

	public $VideoStandard;

	/** @var int IFECMethod */

	public $IFECMethod;

	/** @var int IFECRate */

	public $IFECRate;

	/** @var int OFECMethod */

	public $OFECMethod;

	/** @var int OFECRate */

	public $OFECRate;

	/** @var int Modulation */

	public $Modulation;

	/** @var int SymbolRate */

	public $SymbolRate;

	/** @var int Polarisation */

	public $Polarisation;

	/** @var int LNBSource */

	public $LNBSource;

	/** @var int SpectralInversion */

	public $SpectralInversion;

	/** @var int Bandwidth */

	public $Bandwidth;

	/** @var int LPIFECMethod */

	public $LPIFECMethod;

	/** @var int LPIFECRate */

	public $LPIFECRate;

	/** @var int HAlpha */

	public $HAlpha;

	/** @var int Guard */

	public $Guard;

	/** @var int TransmissionMode */

	public $TransmissionMode;

	/** @var boolean OtherFrequencyInUse */

	public $OtherFrequencyInUse;

	/** @var string Path */

	public $Path;

	/** @var int ChannelId */

	public $ChannelId;
}

class TuneRequestPackage
{
	/** @var string Type */

	public $Type;

	/** @var string Provider */

	public $Provider;

	/** @var string Name */

	public $Name;

	/** @var TuneRequest[] TuneRequests; */

	public $TuneRequests;

	public function __construct($type, $provider, $name)
	{
		$this->Type = $type;
		$this->Provider = $provider;
		$this->Name = $name;
		$this->TuneRequests = array();
	}
}

class TunerData
{
	/** @var int SequenceNumber */

	public $SequenceNumber;

	/** @var string ClientTunerData */

	public $ClientTunerData;

	/** @var string FriendlyName */

	public $FriendlyName;

	/** @var string DisplayName */

	public $DisplayName;

	public function TunerData($sequenceNumber, $clientTunerData, $friendlyName, $displayName)
	{
		$this->SequenceNumber = $sequenceNumber;
		$this->ClientTunerData = $clientTunerData;
		$this->FriendlyName = $friendlyName;
		$this->DisplayName = $displayName;
	}
}

/** @internal soapenum RecordingState */

class RecordingState
{
	const NotScheduled = 'NotScheduled';
	const Scheduled = 'Scheduled';
	const Aborted = 'Aborted';
	const Error = 'Error';
	const Deleted = 'Deleted';
	const InProgress = 'InProgress';
	const Finished = 'Finished';
	const Warning = 'Warning';
}

class Recording
{
	/** @var int Id */

	public $Id;

	/** @var int ProgramId */

	public $ProgramId;

	/** @var string RecordingState */

	public $RecordingState;

	/** @var string StateText */

	public $StateText;

	/** @var guid MachineId */

	public $MachineId;

	/** @var int ChannelId */

	public $ChannelId;

	/** @var dateTime StartDate */

	public $StartDate;

	/** @var dateTime StopDate */

	public $StopDate;

	/** @var guid TunerId */

	public $TunerId;
}

class Machine
{
	/** @var guid TunerId */

	public $Id;

	/** @var string Name */

	public $Name;

	public function __construct($id, $name)
	{
		$this->Id = $id;
		$this->Name = $name;
	}
}

class Tuner
{
	/** @var int SequenceNumber */

	public $SequenceNumber;

	/** @var guid Id */

	public $Id;

	public function __construct($sequenceNumber, $id)
	{
		$this->SequenceNumber = $sequenceNumber;
		$this->Id = $id;
	}
}

class MachineRegistrationData
{
	/** @var guid MachineId */

	public $MachineId;

	/** @var Tuner[] TunerList */

	public $TunerList;

	public function __construct($machineId)
	{
		$this->MachineId = $machineId;
		$this->TunerList = array();
	}
}

class Subscription
{
	/** @var string Id */

	public $Id;

	/** @var string Level */

	public $Level;

	/** @var dateTime StartDate */

	public $StartDate;

	/** @var dateTime StopDate */

	public $StopDate;

	public function __construct($id, $level, $startDate, $stopDate)
	{
		$this->Id = $id;
		$this->Level = $level;
		$this->StartDate = $startDate;
		$this->StopDate = $stopDate;
	}
}

class Guide
{
	/** @var int Page */

	public $Page;

	/** @var int MaxPage */

	public $MaxPage;

	/** @var dateTime MinDate */

	public $MinDate;

	/** @var dateTime MaxDate */

	public $MaxDate;

	/** @var GuideChannel[] Channels */

	public $Channels;
}

class GuideChannel extends Channel
{
	public function __construct($id, $name, $url, $radio)
	{
		parent::__construct($id, $name, $url, null, $radio);
	}
}

class GuideProgram
{
	/** @var int Id */

	public $Id;

	/** @var dateTime StartDate */

	public $StartDate;

	/** @var dateTime StopDate */

	public $StopDate;

	/** @var string Title */

	public $Title;

	/** @var string EpisodeTitle */

	public $EpisodeTitle;

	/** @var string Description */

	public $Description;

	/** @var int EpisodeNumber */

	public $EpisodeNumber;

	/** @var int EpisodeCount */

	public $EpisodeCount;

	/** @var boolean IsRepeat */

	public $IsRepeat;

	/** @var boolean IsLive */

	public $IsLive;

	/** @var int Rating */

	public $Rating;

	/** @var int Year */

	public $Year;

	/** @var string MovieType */

	public $MovieType;

	public function __construct($id)
	{
		$this->Id = $id;
	}
}

class GuideSearchResult
{
	/** @var int Page */

	public $Page;

	/** @var int MaxPage */

	public $MaxPage;

	/** @var string ChannelImageUrl */

	public $ChannelImageUrl;

	/** @var string ProgramImageUrl */

	public $ProgramImageUrl;

	/** @var GuideSearchItem[] Item */

	public $Item;

	public function __construct()
	{
		$this->Item = array();
		$this->ChannelImageUrl = 'http://'.$_SERVER['HTTP_HOST'].dirname($_SERVER['PHP_SELF']).'/channelimg.php?id=';
		$this->ProgramImageUrl = 'http://'.$_SERVER['HTTP_HOST'].dirname($_SERVER['PHP_SELF']).'/programimg.php?id=';
	}
}

class GuideSearchItem
{
	/** @var string Type */

	public $Type; // channel, program

	/** @var GuideChannel Channel */

	public $Channel;

	/** @var GuideProgram Program */

	public $Program;

	public function __construct($type)
	{
		$this->Type = $type;
	}
}

class GuideProgramRecStat
{
	/** @var int ProgramId */

	public $ProgramId;

	/** @var dateTime StartDate */

	public $StartDate;

	/** @var string Title */

	public $Title;

	/** @var int Count */

	public $Count;
}

class GuideMovieRecStat
{
	/** @var int MovieId */

	public $MovieId;

	/** @var string Title */

	public $Title;

	/** @var int Count */

	public $Count;
}

class AppVersion
{
	/** @var string Version */

	public $Version;

	/** @var string Url */

	public $Url;

	/** @var boolean Required */

	public $Required;

	public static function VersionToString($a, $b, $c, $d)
	{
		return sprintf("%04x%04x%04x%04x", $a, $b, $c, $d);
	}

	public static function StringToVersion($v)
	{
		$v = trim($v);

		if(eregi('^([0-9a-fA-F]{4})([0-9a-fA-F]{4})([0-9a-fA-F]{4})([0-9a-fA-F]{4})$', $v, $m)
		|| eregi('^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$', $v, $m))
		{
			$v = array();
			for($i = 1; $i <= 4; $i++) $v[] = hexdec($m[$i]);
		}
		else
		{
			$v = null;
		}

		return $v;
	}
}

class EPG
{
	/** @var dateTime Date */

	public $Date;

	/** @var TuneRequestPackage[] TuneRequestPackages */

	public $TuneRequestPackages;

	/** @var Channel[] Channels */

	public $Channels;

	/** @var Program[] Programs */

	public $Programs;

	/** @var Episode[] Episodes */

	public $Episodes;

	/** @var Movie[] Movies */

	public $Movies;

	/** @var Cast[] Casts */

	public $Casts;

	/** @var MovieType[] MovieTypes */

	public $MovieTypes;

	/** @var DVBCategory[] DVBCategories */

	public $DVBCategories;
};

class EPGLocation
{
	/** @var dateTime Date */

	public $Date;

	/** @var string Url */

	public $Url;

	/** @var string JsonUrl */

	public $JsonUrl;

	/** @var string BZip2Url */

	public $BZip2Url;

	/** @var string JsonBZip2Url */

	public $JsonBZip2Url;
}

class TvStat
{
	/** @var int ChannelId */

	public $ChannelId;

	/** @var int ProgramId */

	public $ProgramId;

	/** @var dateTime Start */

	public $Start;

	/** @var dateTime Stop */

	public $Stop;
}

class UserTvStat
{
	/** @var string Username */

	public $Username;

	/** @var TvStat[] WatchList */

	public $WatchList;
}

class UserInfo
{
	/** @var string Email */

	public $Email;

	/** @var string FirstName */

	public $FirstName;

	/** @var string LastName */

	public $LastName;

	/** @var string Country */

	public $Country;

	/** @var string Address */

	public $Address;

	/** @var string PostalCode */

	public $PostalCode;

	/** @var string PhoneNumber */

	public $PhoneNumber;

	/** @var string Gender */

	public $Gender;

	/** @var int ParentalAge */

	public $ParentalAge;
}

class DeviceDescription
{
	/** @var int Id */

	public $Id;

	/** @var string Brand */

	public $Brand;

	/** @var string Model */

	public $Model;

	/** @var string Serial */

	public $Serial;

	/** @var string System */

	public $System;
}

class Device extends DeviceDescription
{
	/** @var dateTime VodAuthAt */

	public $VodAuthAt;
}

class VersionInfo
{
	/** @var string Name */

	public $Name;

	/** @var boolean Required */

	public $Required;

	/** @var int MajorVersion */

	public $MajorVersion;

	/** @var int MinorVersion */

	public $MinorVersion;

	/** @var int Revision */

	public $Revision;

	/** @var string[] Description */

	public $Description;

	/** @var string Url */

	public $Url;

	/** @var dateTime ReleasedAt */

	public $ReleasedAt;
}

?>