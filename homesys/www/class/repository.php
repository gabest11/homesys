<?php

require_once('MDB2.php');
require_once('Mail.php');
require_once('File/Archive.php');
require_once(dirname(__FILE__).'/../config.php');
require_once(dirname(__FILE__).'/../util.php');
require_once(dirname(__FILE__).'/guid.php');
require_once(dirname(__FILE__).'/data.php');
require_once(dirname(__FILE__).'/soapserverex.php');
require_once(dirname(__FILE__).'/unicable.php');

class Repository
{
	/**
	 * @var MDB2_Driver_Common
	 */

	protected $db;

	public function __construct($dsn = '')
	{
		global $g_config;
		
		if(empty($dsn))
		{
			$dsn = $g_config['db']['dsn'];
		}
		
		$this->db =& $this->ConnectDB($dsn);

		if(!empty($_SERVER['HTTP_USER_AGENT']))
		{
			$this->CreateUserAgent($_SERVER['HTTP_USER_AGENT']);
		}
	}

	function ConnectDB($dsn)
	{
		$db =& MDB2::factory($dsn);

		$this->IsError($db);

		$db->setOption('portability', $db->getOption('portability') & ~MDB2_PORTABILITY_EMPTY_TO_NULL);
		$db->setOption('persistent', true);

		$res = $db->loadModule('Extended');

		$this->IsError($res);

		$res = $db->query("SET NAMES 'utf8'");

		$this->IsError($res);

		return $db;
	}

	public function BeginTransaction()
	{
		$this->db->beginTransaction();
	}

	public function Commit()
	{
		$this->db->commit();
	}

	public function Rollback()
	{
		$this->db->rollback();
	}

	public function IsError($res, $exit = true)
	{
		if(PEAR::isError($res))
		{
			if($exit)
			{
				echo $res->getMessage().' - '.$res->getUserinfo();

				if(isset($res->backtrace)) foreach($res->backtrace as $row)
				{
					if(isset($row['file']) && isset($row['line']))
					{
						$location = sprintf("%s (%d)", $row['file'], $row['line']);
					}
					else
					{
						$location = "(unknown location)";
					}

					printf("%s: %s::%s\r\n", $location, !empty($row['class']) ? $row['class'] : '', $row['function']);
				}

				exit;
			}

			return true;
		}

		return false;
	}

	public function IsPresent($table, $where, $value, $type, $field = null)
	{
		if(empty($field)) $field = 'COUNT(*)';

		$res = $this->db->queryOne("SELECT $field FROM $table WHERE $where = ".$this->db->quote($value, $type));

		$this->IsError($res);

		return $res;
	}

	public function Insert($table, $fields, $types)
	{
		$res = $this->db->autoExecute($table, $fields, MDB2_AUTOQUERY_INSERT, null, $types);

		$this->IsError($res);
	}

	public function CalcFoundRows($req, $start, $limit, &$maxpages, $exit = true)
	{
		if(empty($limit)) $limit = 10;

		$req =
			//'SELECT SQL_CALC_FOUND_ROWS * FROM ('.$req.') AS dummy '.
			preg_replace('/SELECT /i', 'SELECT SQL_CALC_FOUND_ROWS ', $req, 1).' '.
			' LIMIT '.($start * $limit).', '.$limit;

		$res = $this->db->query($req);

		if($exit) $this->IsError($res);

		$maxpages = (int)(($this->db->queryOne('SELECT FOUND_ROWS()') + ($limit - 1)) / $limit);

		return $res;
	}

	public function &GetDB()
	{
		return $this->db;
	}

	public function GetChannelLogo($id)
	{
		return $this->db->queryOne('SELECT Logo FROM Channel WHERE Id = '.(int)$id);
	}

	public function Register($username, $password, $email, $guest = false)
	{
		$username = trim($username);
		$password = trim($password);
		$email = trim($email);

		if(empty($username) || empty($password))
		{
			return null;
		}

		if(!preg_match('/^[a-zA-Z0-9\-\.\_]+\@(\[?)[a-zA-Z0-9\-\.]+\.([a-zA-Z]{2,4}|[0-9]{1,3})(\]?)$/', $email)
		|| !preg_match('/^[a-zA-Z0-9_]+$/', $username))
		{
			return null;
		}

		$this->BeginTransaction();

		// homesys

		$res = $this->db->queryOne('SELECT COUNT(*) FROM User WHERE Username = '.$this->db->quote($username, 'text'));

		if($this->IsError($res) || $res > 0) return null;

		$id = Guid::Generate();

		$req =
			'INSERT INTO User (Id, Username, Password, Email, Created, Guest) VALUES ('.
			$this->db->quote($id).', '.
			$this->db->quote($username).', '.
			$this->db->quote(md5($username.$password)).', '.
			$this->db->quote($email).', '.
			'NOW(), '.
			($guest ? '1' : '0').' '.
			') ';

		$res = $this->db->query($req);

		$this->IsError($res);

		$this->Commit();
		
		return $id;
	}

	public function Login($username, $password)
	{
		$req = 'SELECT Id, Password FROM User WHERE Username = '.$this->db->quote($username);

		$res = $this->db->queryRow($req);

		if($this->IsError($res)) return null;

		if($res[1] != md5($username.$password)) return null;

		return $res[0];
	}

	public function HasMachine($userId)
	{
    		$req = 'SELECT COUNT(*) FROM Machine WHERE UserId = '.$this->db->quote($userId);

		$res = $this->db->queryOne($req);

		$this->IsError($res);

		return !empty($res);
	}

	public function SearchMovie($str)
	{
		$str = trim($str);

		if(empty($str) || strlen($str) < 3)
		{
			return null;
		}

		$str = str_replace(
			array('á', 'é', 'í', 'ó', 'ö', 'ő', 'ú', 'ü', 'ű', '_', '.'),
			array('a', 'e', 'i', 'o', 'o', 'o', 'u', 'u', 'u', ' ', ' '),
			$str);

		$str = preg_replace('/\[[^\]]*\]/', ' ', $str);
		$str = preg_replace('/\([^\)]*\)/', ' ', $str);
		$str = preg_replace('/\{[^\}]*\}/', ' ', $str);
		$str = preg_replace('/ {2,}/', ' ', $str);

		$res = $this->db->queryRow(
			'SELECT UNIX_TIMESTAMP(At), UNIX_TIMESTAMP(DATE_ADD(NOW(), INTERVAL -1 DAY)) '.
			'FROM MovieSearch '.
			'WHERE Str = '.$this->db->quote($str));

		$this->IsError($res);

		if(empty($res) || $res[0] < $res[1])
		{
			// TODO: search port.hu, insert result

			$url = 'http://www.port.hu/pls/po/portal.out_main?i_where=323&i_link_l2=1&i_p2='.urlencode($str);

			$xml = simplexml_load_string(file_get_contents($url));

			$res = $this->db->query(
				'INSERT INTO MovieSearch (Str, At) '.
				'VALUES ('.$this->db->quote($str).', NOW())');

			$this->IsError($res);
		}

		$sl = explode(' ', $str);

		foreach($sl as $i => $str)
		{
			$sl[$i] = 'Title LIKE \'%'.$this->db->escape($str, true).'%\'';
		}

		$res = $this->db->query(
			'SELECT Id, MovieTypeId, DVBCategoryId, Title, Description, Rating, Year, EpisodeCount, PorthuId, PictureUrl IS NOT NULL '.
			'FROM Movie '.
			'WHERE '.implode(' AND ', $sl).' '.
			'ORDER BY Title ASC ');

		$this->IsError($res);

		$movies = array();

		while($row = $res->fetchRow())
		{
			$movies[] = new Movie($row[0], $row[1], $row[2], $row[3], $row[4], $row[5], $row[6], $row[7], $row[8] == 0, $row[9] != 0);
		}

		return $movies;
	}

	public function UpdateTuneRequestPackages()
	{
		// MinDig

		$packages = array();
		$name = '';
		$freq = 0;

		$str = file_get_contents('http://frekvencia.hu/dvb-t-hng.htm');
		// $str = file_get_contents('http://frekvencia.hu/dvb-t-hng-amux.htm');
		// $str .= file_get_contents('http://frekvencia.hu/dvb-t-hng-cmux.htm');

		$str = str_replace("\n", "", $str);
		$str = str_replace("\t", "", $str);
		$str = str_replace("\r", "", $str);

		preg_match_all('/<tr>(.+?)<\\/tr>/i', $str, $matches);

		for($i = 0; $i < count($matches[0]); $i++)
		{
			if(!preg_match_all('/<td class="(h|g[0-9])"[^>]*>(.*?)<\\/td>/i', $matches[1][$i], $matches2))
			{
				continue;
			}

			$k = count($matches2[0]);

			if($k == 7)
			{
				$freq = (int)$matches2[2][1] * 1000;
			}

			if(!preg_match('/<a[^>]+fmdx\\.hu[^>]+rel="external"[^>]+title="([^"]+)"[^>]*>(.+?)<\\/a>/i', $matches[1][$i], $matches3))
			{
				if($k == 5 && $matches2[2][3] == 'H') 
				{
					echo $matches2[2][2]."*\r\n";

					$matches3 = array(2 => $matches2[2][2]);
				}
				else
				{
					continue;
				}
			}

			$name = trim($matches3[2]);
			$name = strip_tags(str_replace('<br />', ' ', $name));

			if(empty($packages[$name]))
			{
				$packages[$name] = array();
			}

			$packages[$name][] = $freq;
		}

		if(empty($packages)) die('UpdateTuneRequestPackages ???');

		$this->BeginTransaction();

		$req =
			'DELETE FROM TuneRequestRegion '.
			'WHERE PackageId IN (SELECT Id FROM TuneRequestPackage '.
			'WHERE Type = '.$this->db->quote('DVB-T', 'text').
			'  AND Provider = '.$this->db->quote('MinDig', 'text').')';

		$res = $this->db->query($req);

		$this->IsError($res);

		$req =
			'DELETE FROM TuneRequest '.
			'WHERE PackageId IN (SELECT Id FROM TuneRequestPackage '.
			'WHERE Type = '.$this->db->quote('DVB-T', 'text').
			'  AND Provider = '.$this->db->quote('MinDig', 'text').')';

		$res = $this->db->query($req);

		$this->IsError($res);

		$req =
			'DELETE FROM TuneRequestPackage '.
			'WHERE Type = '.$this->db->quote('DVB-T', 'text').
			'  AND Provider = '.$this->db->quote('MinDig', 'text');

		$res = $this->db->query($req);

		$this->IsError($res);

		$ids = array();

		foreach($packages as $name => $freqs)
		{
			echo $name."\r\n";

			$req =
				'INSERT INTO TuneRequestPackage (Type, Provider, Name) '.
				'VALUES ('.
				$this->db->quote('DVB-T', 'text').', '.
				$this->db->quote('MinDig', 'text').', '.
				$this->db->quote($name, 'text').') ';

			echo $req."\r\n";

			$res = $this->db->query($req);

			$this->IsError($res);

			$id = $this->db->lastInsertId();

			foreach($freqs as $freq)
			{
				$req =
					"INSERT INTO TuneRequest (PackageId, Frequency, IFECRate, Modulation, Guard) ".
					"VALUES (".$id.", ".$freq.", 3, '64QAM', 4)";

				echo $req."\r\n";

				$res = $this->db->query($req);

				$this->IsError($res);
			}

			$ids[] = $id;
		}

		foreach($ids as $id)
		{
			$req = 'INSERT INTO TuneRequestRegion (PackageId, RegionId) VALUES ('.$id.', '.$this->db->quote('hun').')';

			$res = $this->db->query($req);

			echo $req."\r\n";

			$this->IsError($res);
		}

		$this->Commit();
	}

	public function GetEPG($region)
	{
		$this->BeginTransaction();

		$now = $this->db->queryOne("SELECT UNIX_TIMESTAMP(NOW())");

		$epg = new EPG();

		$epg->Date = SoapServerEx::FixDateTime($now);

		// TuneRequestPackages

		$epg->TuneRequestPackages = array();

		$req =
			'SELECT t1.Id, t1.Type, t1.Provider, t1.Name FROM TuneRequestPackage t1 '.
			'LEFT OUTER JOIN TuneRequestRegion TRR ON TRR.PackageId = t1.Id '.
			'WHERE (TRR.RegionId IS NULL OR TRR.RegionId = '.$this->db->quote($region, 'text').') '.
			'ORDER BY 2 ';

		$res = $this->db->query($req);

		$this->IsError($res);

		$packages = array();

		while($row = $res->fetchRow())
		{
			$packages[(int)$row[0]] = new TuneRequestPackage($row[1], $row[2], $row[3]);
		}

		foreach($packages as $id => $p)
		{
			$req =
				'SELECT '.
				' t1.Frequency, '.
				' CAST(t1.VideoStandard AS UNSIGNED INTEGER), '.
				' CAST(t1.IFECMethod AS UNSIGNED INTEGER), '.
				' CAST(t1.IFECRate AS UNSIGNED INTEGER), '.
				' CAST(t1.OFECMethod AS UNSIGNED INTEGER), '.
				' CAST(t1.OFECRate AS UNSIGNED INTEGER), '.
				' CAST(t1.Modulation AS UNSIGNED INTEGER), '.
				' t1.SymbolRate, '.
				' CAST(t1.Polarisation AS UNSIGNED INTEGER), '.
				' t1.LNBSource, '.
				' CAST(t1.SpectralInversion AS UNSIGNED INTEGER), '.
				' t1.Bandwidth, '.
				' CAST(t1.LPIFECMethod AS UNSIGNED INTEGER), '.
				' CAST(t1.LPIFECRate AS UNSIGNED INTEGER), '.
				' t1.HAlpha, '.
				' t1.Guard, '.
				' t1.TransmissionMode, '.
				' t1.OtherFrequencyInUse, '.
				' t1.Path, '.
				' t1.ChannelId '.
				'FROM TuneRequest t1 '.
				'LEFT OUTER JOIN Channel t2 ON t1.ChannelId = t2.Id '.
				'WHERE t1.PackageId = '.$id.' '.
				'ORDER BY t1.LNBSource, t2.Radio, t1.Frequency, t1.Modulation ';

			$res = $this->db->query($req);

			$this->IsError($res);

			while($row = $res->fetchRow())
			{
				$tr = new TuneRequest();

				$tr->Frequency = $row[0];
				$tr->VideoStandard = (int)$row[1];
				$tr->IFECMethod = (int)$row[2];
				$tr->IFECRate = (int)$row[3];
				$tr->OFECMethod = (int)$row[4];
				$tr->OFECRate = (int)$row[5];
				$tr->Modulation = (int)$row[6];
				$tr->SymbolRate = $row[7];
				$tr->Polarisation = (int)$row[8];
				$tr->LNBSource = $row[9];
				$tr->SpectralInversion = (int)$row[10];
				$tr->Bandwidth = $row[11];
				$tr->LPIFECMethod = (int)$row[12];
				$tr->LPIFECRate = (int)$row[13];
				$tr->HAlpha = $row[14];
				$tr->Guard = $row[15];
				$tr->TransmissionMode = $row[16];
				$tr->OtherFrequencyInUse = $row[17];
				$tr->Path = $row[18];
				$tr->ChannelId = $row[19];

				$packages[$id]->TuneRequests[] = $tr;
			}
		}

		$epg->TuneRequestPackages = array_values($packages);

		// Channels

		$epg->Channels = array();

		$aliasas = array();

		$res = $this->db->query('SELECT ChannelId, Name FROM ChannelAlias');

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			$id = (int)$row[0];

			if(!isset($aliases[$id])) $aliases[$id] = array();

			$aliases[$id][] = $row[1];
		}

		$res = $this->db->query(
			'SELECT t1.Id, t1.Name, t1.Url, t1.Logo, t1.Radio '.
			'FROM Channel t1 '.
			'LEFT OUTER JOIN ChannelRegion CR ON CR.ChannelId = t1.Id '.
			'WHERE t1.HasProgram <> 0 '.
			'  AND (CR.RegionId IS NULL OR CR.RegionId = '.$this->db->quote($region, 'text').') '.
			'ORDER BY 5, 1 '
			);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			$id = (int)$row[0];

			$channel = new Channel($id, $row[1], $row[2], $row[3], $row[4] == 1);

			if(!empty($aliases[$id])) $channel->Aliases = implode('|', $aliases[$id]);

			$epg->Channels[] = $channel;
		}

		// Programs

		$epg->Programs = array();

		$req =
			'SELECT t1.Id, t1.ChannelId, t1.EpisodeId, t2.MovieId, t1.StartDate, t1.StopDate, t1.IsRepeat, t1.IsLive, t1.IsRecommended, t1.Deleted '.
			'FROM Program t1 '.
			'JOIN Episode t2 ON t1.EpisodeId = t2.Id '.
			'LEFT OUTER JOIN ChannelRegion CR ON CR.ChannelId = t1.ChannelId '.
			'WHERE t1.Deleted = 0 AND t1.StopDate > CAST(CURDATE() AS datetime) '.
			'  AND (CR.RegionId IS NULL OR CR.RegionId = '.$this->db->quote($region, 'text').') ';

		$res = $this->db->query($req);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			$epg->Programs[] = new Program($row[0], $row[1], $row[2], $row[3], SoapServerEx::FixDateTime($row[4]), SoapServerEx::FixDateTime($row[5]), $row[6], $row[7], $row[8], $row[9]);
		}

		// Episodes

		$episodes = array();

		$req =
			'SELECT DISTINCT t1.Id, t1.MovieId, t1.Title, t1.Description, t1.EpisodeNumber, t1.PorthuId, t3.Description '.
			'FROM Episode t1 '.
			'JOIN Program t2 ON t2.EpisodeId = t1.Id '.
			'JOIN Movie t3 ON t3.Id = t1.MovieId '.
			'LEFT OUTER JOIN ChannelRegion CR ON CR.ChannelId = t2.ChannelId '.
			'WHERE t2.Deleted = 0 AND t2.StopDate > CAST(CURDATE() AS datetime) '.
			'  AND (CR.RegionId IS NULL OR CR.RegionId = '.$this->db->quote($region, 'text').') ';

		$res = $this->db->query($req);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			if($row[3] == $row[6]) $row[3] = '';
			if(mb_strlen($row[2], 'UTF-8') > 64) $row[2] = mb_substr($row[2], 0, 64, 'UTF-8');

			$episodes[(int)$row[0]] = new Episode($row[0], $row[1], $row[2], $row[3], $row[4], $row[5] == 0);
		}

		$req =
			'SELECT DISTINCT t1.EpisodeId, t1.CastId, t1.RoleId, t1.MovieName '.
			'FROM EpisodeCast t1 '.
			'JOIN Episode t2 ON t2.Id = t1.EpisodeId '.
			'JOIN Program t3 ON t3.EpisodeId = t2.Id '.
			'LEFT OUTER JOIN ChannelRegion CR ON CR.ChannelId = t3.ChannelId '.
			'WHERE t3.Deleted = 0 AND t3.StopDate > CAST(CURDATE() AS datetime) '.
			'  AND (CR.RegionId IS NULL OR CR.RegionId = '.$this->db->quote($region, 'text').') ';

		$res = $this->db->query($req);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			if(isset($episodes[(int)$row[0]]))
			{
				if(mb_strlen($row[3], 'UTF-8') > 64) $row[3] = mb_substr($row[3], 0, 64, 'UTF-8');

				$episodes[(int)$row[0]]->EpisodeCastList[] = new EpisodeCast($row[1], $row[2], $row[3]);
			}
		}

		$epg->Episodes = array_values($episodes);

		// Movies

		$epg->Movies = array();

		$req =
			'SELECT DISTINCT t1.Id, t1.MovieTypeId, t1.DVBCategoryId, t1.Title, t1.Description, t1.Rating, t1.Year, t1.EpisodeCount, t1.PorthuId, t1.PictureUrl IS NOT NULL '.
			'FROM Movie t1 '.
			'JOIN Episode t2 ON t2.MovieId = t1.Id '.
			'JOIN Program t3 ON t3.EpisodeId = t2.Id '.
			'LEFT OUTER JOIN ChannelRegion CR ON CR.ChannelId = t3.ChannelId '.
			'WHERE t3.Deleted = 0 AND t3.StopDate > CAST(CURDATE() AS datetime) '.
			'  AND (CR.RegionId IS NULL OR CR.RegionId = '.$this->db->quote($region, 'text').') ';

		$res = $this->db->query($req);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			if(mb_strlen($row[3], 'UTF-8') > 64) $row[3] = mb_substr($row[3], 0, 64, 'UTF-8');

			$epg->Movies[] = new Movie($row[0], $row[1], $row[2], $row[3], $row[4], $row[5], $row[6], $row[7], $row[8] == 0, $row[9] != 0);
		}

		// Casts

		$epg->Casts = array();

		$req =
			'SELECT DISTINCT t1.Id, t1.Name '.
			'FROM Cast t1 '.
			'JOIN EpisodeCast t2 ON t2.CastId = t1.Id '.
			'JOIN Program t3 ON t3.EpisodeId = t2.EpisodeId '.
			'LEFT OUTER JOIN ChannelRegion CR ON CR.ChannelId = t3.ChannelId '.
			'WHERE t3.Deleted = 0 AND t3.StopDate > CAST(CURDATE() AS datetime) '.
			'  AND (CR.RegionId IS NULL OR CR.RegionId = '.$this->db->quote($region, 'text').') ';

		$res = $this->db->query($req);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			if(mb_strlen($row[1], 'UTF-8') > 64) $row[1] = mb_substr($row[1], 0, 64, 'UTF-8');

			$epg->Casts[] = new Cast($row[0], $row[1]);
		}

		// MovieTypes

		$epg->MovieTypes = array();

		$req =
			'SELECT DISTINCT t1.Id, t1.NameShort, t1.NameLong '.
			'FROM MovieType t1 '; // TODO: region

		$res = $this->db->query($req);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			$epg->MovieTypes[] = new MovieType($row[0], $row[1], $row[2]);
		}

		// DVBCategories

		$epg->DVBCategories = array();

		$req =
			'SELECT DISTINCT t1.Id, t1.Name '.
			'FROM DVBCategory t1 '; // TODO: region

		$res = $this->db->query($req);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			$epg->DVBCategories[] = new DVBCategory($row[0], $row[1]);
		}

		//

		$this->Commit();

		return $epg;
	}

	public function CleanUpGuide()
	{
		$this->BeginTransaction();

		printf("Removing old entries...\r\n");


		$req = 'DELETE FROM TvStat WHERE Start < DATE_ADD(NOW(), INTERVAL -6 MONTH)';

		$res = $this->db->exec($req);

		$this->IsError($res);

		printf("TvStat: %d\r\n", $res);


		$req = 'DELETE FROM Program WHERE StartDate < DATE_ADD(NOW(), INTERVAL -6 MONTH) OR StartDate < DATE_ADD(NOW(), INTERVAL -1 WEEK) AND Id NOT IN (SELECT ProgramId FROM TvStat) AND Id NOT IN (SELECT ProgramId FROM Recording)';

		$res = $this->db->exec($req);

		$this->IsError($res);

		printf("Program: %d\r\n", $res);


		$req = 'DELETE FROM Episode WHERE Id NOT IN (SELECT EpisodeId FROM Program) ';

		$res = $this->db->exec($req);

		$this->IsError($res);

		printf("Episode: %d\r\n", $res);


		$req = 'DELETE FROM Movie WHERE Id NOT IN (SELECT MovieId FROM Episode) ';

		$res = $this->db->exec($req);

		$this->IsError($res);

		printf("Movie: %d\r\n", $res);


		$req = 'DELETE FROM EpisodeCast WHERE EpisodeId NOT IN (SELECT Id FROM Episode) ';

		$res = $this->db->exec($req);

		$this->IsError($res);

		printf("EpisodeCast: %d\r\n", $res);


		$req = 'DELETE FROM Cast WHERE Id NOT IN (SELECT CastId FROM EpisodeCast) ';

		$res = $this->db->exec($req);

		$this->IsError($res);

		printf("Cast: %d\r\n", $res);


		$req = 'DELETE FROM ProgramRelated WHERE ProgramId NOT IN (SELECT Id FROM Program) ';

		$res = $this->db->exec($req);

		$this->IsError($res);

		printf("ProgramRelated: %d\r\n", $res);


		$this->Commit();

		printf("Done!\r\n\r\n");
	}

	public function UpdateGuide()
	{
		$this->BeginTransaction();

		// HasProgram

		$channelIds = $this->db->queryCol('SELECT DISTINCT ChannelId FROM Program');

		$this->IsError($channelIds);

		$this->db->query('UPDATE Channel SET HasProgram = 0');

		foreach($channelIds as $channelId)
		{
			$res = $this->db->query('UPDATE Channel SET HasProgram = 1 WHERE Id = '.$channelId);

			$this->IsError($res);
		}

		// IsRepeat

		$programIds = $this->db->queryCol(
			'SELECT DISTINCT t2.Id '.
			'FROM Program t1 '.
			'JOIN Program t2 ON t2.Id <> t1.Id '.
			'AND t2.EpisodeId = t1.EpisodeId '.
			'AND t2.ChannelId = t1.ChannelId '.
			'AND t2.StartDate > t1.StartDate '.
			'AND t2.StartDate < DATE_ADD(t1.StartDate, INTERVAL 1 MONTH) '.
			'JOIN Channel t3 ON t3.Id = t1.ChannelId '.
			'JOIN Episode t4 ON t4.Id = t1.EpisodeId '.
			'WHERE t1.IsRepeat = 0 AND t2.IsRepeat = 0 ');

		$this->IsError($programIds);

		foreach($programIds as $programId)
		{
			$res = $this->db->query('UPDATE Program SET IsRepeat = 1 WHERE Id = '.$programId);

			$this->IsError($res);
		}

		echo 'Marked '.count($programIds).' programs repeating'."\r\n";

		$this->Commit();
	}

	public function CreateDevice($brand, $model, $serial, $system, $networkId)
	{
		$brand = trim($brand);
		$model = trim($model);
		$serial = trim($serial);
		$system = trim($system);
		$networkId = trim($networkId);

		// TODO: if(strpos($serial, 'ASHWID') === 0) {} // handle hw drift, find similar $system with similar ashwid and update $serial

		if($brand == 'browser')
		{
			if(empty($model) && !empty($_SERVER['HTTP_USER_AGENT']))
			{
				$model = substr($_SERVER['HTTP_USER_AGENT'], 0, 192);
			}
		}

		$req =
			'SELECT Id, System, NetworkId, Model '.
			'FROM Device '.
			'WHERE Brand = '.$this->db->quote($brand).' '.
			'  AND Model = '.$this->db->quote($model).' '.
			'  AND Serial = '.$this->db->quote($serial);

		$res = $this->db->queryRow($req);

		$this->IsError($res);

		if(empty($res))
		{
			$req =
				'INSERT INTO Device (Brand, Model, Serial, System, NetworkId) '.
				'VALUES ('.
				$this->db->quote($brand).', '.
				$this->db->quote($model).', '.
				$this->db->quote($serial).', '.
				$this->db->quote($system).', '.
				$this->db->quote($networkId).')';

			$res = $this->db->query($req);

			$this->IsError($res);

			$id = $this->db->lastInsertId();
		}
		else
		{
			$id = $res[0];

			if(!empty($system) && (empty($res[1]) || $res[1] != $system))
			{
				$req = 'UPDATE Device SET System = '.$this->db->quote($system).' WHERE Id = '.$id;

				$res2 = $this->db->query($req);

				$this->IsError($res2);
			}

			if(!empty($networkId) && (empty($res[2]) || $res[2] != $networkId))
			{
				$req = 'UPDATE Device SET NetworkId = '.$this->db->quote($networkId).' WHERE Id = '.$id;

				$res2 = $this->db->query($req);

				$this->IsError($res2);
			}
		}

		return $id;
	}

	public function CreateUserAgent($name)
	{
		if(empty($name)) $name = '(empty)';

		$name = trim($name);
		$name = trim($name, ';');

		$hash = md5($name);

		$encoding = !empty($_SERVER['HTTP_ACCEPT_ENCODING']) ? $_SERVER['HTTP_ACCEPT_ENCODING'] : null;

		$req = 'SELECT Id, AcceptEncoding FROM UserAgent WHERE NameHash = '.$this->db->quote($hash);

		$res = $this->db->queryRow($req);

		$this->IsError($res);

		if(empty($res))
		{
			$req =
				'INSERT INTO UserAgent (Name, NameHash) '.
				'VALUES ('.$this->db->quote($name).', '.$this->db->quote($hash).')';

			$res = $this->db->exec($req);

			$this->IsError($res);

			$id = $this->db->lastInsertId();
		}
		else
		{
			$id = $res[0];
		}

		$req = 'UPDATE UserAgent SET RequestCount = RequestCount + 1 ';

		if(!empty($encoding))
		{
			$req .= ', AcceptEncoding = '.$this->db->quote($encoding).' ';
		}

		$req .= 'WHERE Id = '.$id;

		$res = $this->db->exec($req);

		return $id;
	}
}

class PorthuRepository extends Repository
{

	private $channelInsert;
	private $programInsert;
	private $episodeInsert;
	private $movieInsert;
	private $castInsert;
	private $episodeCastInsert;
	private $movieTypeInsert;
	private $dvbCategoryInsert;
	private $porthuMap = array();
	private $movieTypeMap = array();
	private $dvbCategoryMap = array();
	private $episodeCastMap = array();
	public $stats = array();

	public function __construct()
	{
		parent::__construct();

		$res = $this->channelInsert = $this->db->autoPrepare(
			'Channel', array('Name', 'Url', 'Logo', 'PorthuId', 'PorthuName'), MDB2_AUTOQUERY_INSERT, false, array('text', 'text', 'blob', 'text', 'text'));

		$this->IsError($res);

		$res = $this->programInsert = $this->db->autoPrepare(
			'Program', array('ChannelId', 'EpisodeId', 'StartDate', 'StopDate', 'IsRepeat', 'IsLive', 'PorthuId'), MDB2_AUTOQUERY_INSERT, false, array('integer', 'integer', 'timestamp', 'timestamp', 'boolean', 'boolean', 'text'));

		$this->IsError($res);

		$res = $this->episodeInsert = $this->db->autoPrepare(
			'Episode', array('MovieId', 'Title', 'Description', 'EpisodeNumber', 'PorthuId'), MDB2_AUTOQUERY_INSERT, false, array('integer', 'text', 'clob', 'integer', 'text'));

		$this->IsError($res);

		$res = $this->movieInsert = $this->db->autoPrepare(
			'Movie', array('MovieTypeId', 'DVBCategoryId', 'Title', 'Rating', 'Year', 'EpisodeCount', 'PorthuId'), MDB2_AUTOQUERY_INSERT, false, array('integer', 'integer', 'text', 'integer', 'integer', 'integer', 'text'));

		$this->IsError($res);

		$res = $this->castInsert = $this->db->autoPrepare(
			'Cast', array('Name', 'MovieName', 'PorthuId'), MDB2_AUTOQUERY_INSERT, false, array('text', 'text', 'text'));

		$this->IsError($res);

		$res = $this->episodeCastInsert = $this->db->autoPrepare(
			'EpisodeCast', array('EpisodeId', 'CastId', 'RoleId', 'MovieName'), MDB2_AUTOQUERY_INSERT, false, array('integer', 'integer', 'integer', 'text'));

		$this->IsError($res);

		$res = $this->movieTypeInsert = $this->db->autoPrepare(
						'MovieType', array('NameShort', 'NameLong'), MDB2_AUTOQUERY_INSERT, false, array('text', 'text'));

		$this->IsError($res);

		$res = $this->dvbCategoryInsert = $this->db->autoPrepare(
			'DVBCategory', array('Name'), MDB2_AUTOQUERY_INSERT, false, array('text'));

		$this->IsError($res);
	}

	private function BuildPorthuMap($table)
	{
		$res = $this->db->query('SELECT Id, PorthuId FROM '.$table);

		$this->IsError($res);

		while($row = $res->fetchRow())
		{
			$this->porthuMap[$table.$row[1]] = $row[0];
		}
	}

	private function BuildMaps()
	{
		$this->porthuMap = array();

		$this->BuildPorthuMap('Program');
		$this->BuildPorthuMap('Episode');
		$this->BuildPorthuMap('Movie');
		$this->BuildPorthuMap('Cast');

		$this->movieTypeMap = array();

		$res = $this->db->queryAll("SELECT * FROM MovieType", null, MDB2_FETCHMODE_ASSOC);

		$this->IsError($res);

		foreach($res as $row)
		{
			$hash = $this->MakeMovieTypeHash($row['nameshort'], $row['namelong']);

			$this->movieTypeMap[$hash] = $row['id'];
		}

		$this->dvbCategoryMap = array();

		$res = $this->db->queryAll("SELECT * FROM DVBCategory", null, MDB2_FETCHMODE_ASSOC);

		$this->IsError($res);

		foreach($res as $row)
		{
			$this->dvbCategoryMap[$row['name']] = $row['id'];
		}

		$this->episodeCastMap = array();

		$res = $this->db->queryAll("SELECT EpisodeId, CastId, RoleId FROM EpisodeCast", null, MDB2_FETCHMODE_ASSOC);

		$this->IsError($res);

		foreach($res as $row)
		{
			$hash = $this->MakeEpisodeCastHash($row['episodeid'], $row['castid'], $row['roleid']);

			$this->episodeCastMap[$hash] = true;
		}
	}

	public function Import($archive)
	{
		echo $archive."\r\n";

		$source = File_Archive::read($archive.'/*.xml*');

		if(!$source)
		{
			echo "ERROR: Cannot read $archive\r\n";
			return;
		}

		$this->CleanUpGuide(); // TODO

		$this->BeginTransaction();

		// maps

		$this->BuildMaps();

		$dvb2mtMap = array(
			'Oktatás / Tudomány / Ismeretterjesztés' => array('ismerett. film', 'ismeretterjesztő film'),
			'Hírek / Aktualitások' => array('hírek', 'hírek'),
			'Film/Dráma' => array('film', 'film'),
			'Zene/ Balett / Tánc' => array('zene', 'zene'),
			'Gyermek- / Ifjúsági programok' => array('gyermekf.', 'gyermekfilm'),
		);

		// channels

		$channelMap = array();

		$source->close();

		while($source->next())
		{
			$fileName = trim($source->getFilename());

			echo $fileName."\r\n";

			if(eregi('^base_channels\.xml$', $fileName))
			{
				echo $fileName."\r\n";

				$xml = new SimpleXMLElement($source->getData());

				foreach($xml->Channels->Channel as $channel)
				{
					$porthuId = (string)$channel['id'];
					$porthuName = str_replace('...', '', (string)$channel->Title);
					$porthuLogo = (string)$channel->Logo;

					$id = $this->FindChannel($porthuId, $porthuName);

					if(empty($id))
					{
						$id = $this->CreateChannel($porthuId, $porthuName);
					}
					
					if(!empty($porthuLogo))
					{
						$logo = file_get_contents($porthuLogo);

						if(!empty($logo))
						{
							$res = $this->db->query('UPDATE Channel SET Logo = '.$this->db->quote($logo, 'blob').' WHERE Id = '.$id);

							$this->IsError($res);
						}
					}

					$channelMap[$porthuId] = $id;
				}

				echo "\r\n";
			}
		}

		// programs

		$source->close();

		while($source->next())
		{
			$fileName = $source->getFilename();

			if(!eregi('^n_events_([0-9]+)\.xml$', $fileName, $matches))
			{
				continue;
			}

			echo $fileName."\r\n";

			$xml = new SimpleXMLElement($source->getData());

			$porthuChannelId = (string)$xml->Channel['Id'];

			if(!isset($channelMap[$porthuChannelId]))
			{
				echo "WARNING: porthuChannelId = $porthuChannelId is unknown\r\n";
				continue;
			}

			$channelId = $channelMap[$porthuChannelId];

			$newProgramIds = array();

			foreach($xml->Channel->EventList->EventDate as $eventDate)
			{
				foreach($eventDate->Event as $event)
				{
					$programId = $this->AddProgram($event, $channelId);

					if($programId > 0)
					{
						$newProgramIds[$programId] = true;
					}
				}
			}

			if(!empty($newProgramIds))
			{
				$res = $this->db->query('SELECT Id FROM Program WHERE StartDate >= NOW() AND ChannelId = '.$channelId);

				while($row = $res->fetchRow())
				{
					$id = $row[0];

					if(empty($newProgramIds[$id]))
					{
						$newProgramIds[$id] = false;
					}
				}

				foreach($newProgramIds as $id => $exists)
				{
					if(!$exists)
					{
						$this->db->query('UPDATE Program SET Deleted = 1 WHERE Id = '.$id);

						echo 'Existing Program not found, marking as Deleted (Id = '.$id.' ChannelId = '.$channelId.')'."\r\n";
					}
				}
			}

			echo "\r\n";
		}

		$this->Commit();

		$this->UpdateGuide();
	}

	private function AddProgram($event, $channelId)
	{
		$porthuProgramId = (string)$event['Id'];
		$porthuMovieId = (string)$event->Filmid;
		$porthuEpisodeId = (string)$event->Episodeid;

		if(empty($porthuProgramId)) $porthuProgramId = 0;
		if(empty($porthuMovieId)) $porthuMovieId = 0;
		if(empty($porthuEpisodeId)) $porthuEpisodeId = 0;

		$startDate = strftime('%Y-%m-%d %H:%M:%S', strtotime($event->StartDate));
		$stopDate = strftime('%Y-%m-%d %H:%M:%S', strtotime($event->StopDate));
		$isRepeat = (int)$event->Isrepeat;
		$isLive = false;
		$episodeTitle = substr((string)$event->Episodetitle, 0, 192);
		$episodeNumber = 0;
		$episodeCount = 0;
		$directors = array();
		$actors = array();
		$year = 0;
		$rating = (int)$event->Rating;
		$title = substr((string)$event->Title, 0, 192);
		$description = $this->ExtractDescription((string)$event->Shortdescription, (string)$event->Longdescription, $isLive, $episodeNumber, $episodeCount, $year, $directors, $actors);
		$movieTypeNameShort = (string)$event->Filmtypenameshort;
		$movieTypeNameLong = (string)$event->Filmtypenamelong;
		$dvbCategoryName = (string)$event->DVBCategoryName;
		// TODO: $categoryNibbleLevel1 = (int)$event->CategoryNibbleLevel1;

		if($porthuProgramId < 0)
		{
			return 0;
		}

		$programId = $this->FindByPorthuId('Program', $porthuProgramId);

		if(empty($title))
		{
			echo "WARNING: Event $porthuProgramId has no Title\r\n";

			$title = '?';
		}
		
		if($rating < 0 || $rating > 18) $rating = 0;

		{

			$movieTypeNameShort = trim($movieTypeNameShort);

			$movieTypeNameLong = trim($movieTypeNameLong);

			$movieTypeId = $this->FindMovieType($movieTypeNameShort, $movieTypeNameLong);

			if(!$movieTypeId)
			{
				$movieTypeId = $this->CreateMovieType($movieTypeNameShort, $movieTypeNameLong);
			}

			//

			$dvbCategoryName = trim($dvbCategoryName);

			$dvbCategoryId = $this->FindDVBCategory($dvbCategoryName);

			if(!$dvbCategoryId)
			{
				$dvbCategoryId = $this->CreateDVBCategory($dvbCategoryName);
			}

			// HACK

			if(!$movieTypeId && $dvbCategoryId && !empty($dvb2mtMap[$dvbCategoryName]))
			{
				$mt = $dvb2mtMap[$dvbCategoryName];

				$movieTypeId = $this->FindMovieType($mt[0], $mt[1]);
			}
		}

		$movieId = $this->FindMovie($porthuMovieId, $porthuEpisodeId, $porthuProgramId, $title);

		if(empty($movieId))
		{
			$movieId = $this->CreateMovie($porthuMovieId, $movieTypeId, $dvbCategoryId, $title, $rating, $year, $episodeCount);
		}
		else
		{
			// if($title != '?')
			{
				// echo 'Movie Title ['.$movieId.'] => '.$title."\r\n";

				$res = $this->db->query(
					'UPDATE Movie '.
					'SET '.
					' MovieTypeId = '.$this->db->quote($movieTypeId, 'integer').', '.
					' DVBCategoryId = '.$this->db->quote($dvbCategoryId, 'integer').', '.
					' Title = '.$this->db->quote($title, 'text').', '.
					' Rating = '.$this->db->quote($rating, 'integer').', '.
					' Year = '.$this->db->quote($year, 'integer').', '.
					' EpisodeCount = '.$this->db->quote($episodeCount, 'integer').' '.
					'WHERE Id = '.$this->db->quote($movieId, 'integer'));

				if($this->IsError($res, false))
				{
					@mail2('gabest11@gmail.com', 'epg import hiba', print_r($res, true));
				}
			}
		}

		$episodeId = $this->FindEpisode($porthuEpisodeId, $porthuProgramId, $title);

		if(empty($episodeId))
		{
			$episodeId = $this->CreateEpisode($porthuEpisodeId, $movieId, $episodeTitle, $description, $episodeNumber);
		}
		else
		{
			// if(!empty($episodeTitle))
			{
				// echo 'Episode Title ['.$episodeId.'] => '.$episodeTitle."\r\n";

				$res = $this->db->query(
					'UPDATE Episode '.
					'SET '.
					' Title = '.$this->db->quote($episodeTitle, 'text').', '.
					' Description = '.$this->db->quote($description, 'text').', '.
					' EpisodeNumber = '.$this->db->quote($episodeNumber, 'integer').' '.
					'WHERE Id = '.$this->db->quote($episodeId, 'integer'));
			}
		}

		foreach(array(1 => $directors, 2 => $actors) as $roleId => $persons)
		{
			foreach($persons as $porthuCastId => $names)
			{
				$castId = $this->FindByPorthuId('Cast', $porthuCastId);

				if(!$castId)
				{
					$castId = $this->CreateCast($porthuCastId, $names);
				}

				$this->CreateEpisodeCast($episodeId, $castId, $roleId, $names);
			}
		}

		// TODO: insert into Picture

		foreach($event->Pictures->Picture as $picture)
		{
			$pictureId = (string)$picture->id;
			$pictureUrl = (string)$picture->url;

			$req =
				'UPDATE Movie SET PictureUrl = '.$this->db->quote($pictureUrl).' '.
				'WHERE Id = '.$movieId;

			$this->db->query($req);

			break; // csak egy kep van ugyis benne...
		}

		if(empty($programId))
		{
			$programId = $this->CreateProgram($porthuProgramId, $channelId, $episodeId, $startDate, $stopDate, $isRepeat, $isLive);
		}
		else
		{
			$this->UpdateProgram($programId, $episodeId, $startDate, $stopDate, $isRepeat, $isLive);
		}

		return $programId;
	}

	private function AddCast(&$dst, $str)
	{
		$array = explode(',', $str);

		foreach($array as $str)
		{
			if(preg_match('/ *(.+?) *\[ *([0-9]+) *\]/', $str, $matches))
			{
				$id = $matches[2];

				$dst[$id] = array();

				$names = explode(':', $matches[1], 2);

				foreach($names as $name)
				{
					$dst[$id][] = trim($name);
				}
			}
		}
	}

	private function StartsWith($haystack, $needles)
	{
		foreach($needles as $needle)
		{
			if(strpos($haystack, $needle) === 0)
			{
				return true;
			}
		}

		return false;
	}

	private function ExtractDescription($short, $long, &$live, &$episodeNumber, &$episodeCount, &$year, &$directors, &$actors)
	{
		$short = trim($short);
		$long = trim($long);
		$live = strpos($short, '(élő)') !== false;
		$episodeNumber = 0;
		$episodeCount = 0;
		$year = 0;
		$directors = array();
		$actors = array();

		if(preg_match('/^\((.+?)\) *(.+)/', $short, $matches))
		{
			$short = $matches[2];

			$array = explode(',', $matches[1]);

			foreach($array as $str)
			{
				$str = trim($str);

				if(ereg('^[0-9]{4}$', $str))
				{
					$year = (int)$str;
				}
				else if(ereg('^([0-9]+)\. rész$', $str, $matches))
				{
					$episodeNumber = $matches[1];
				}
				else if(ereg('^([0-9]+)/([0-9]+)\. rész$', $str, $matches))
				{
					$episodeNumber = $matches[2];
					$episodeCount = $matches[1];
				}
			}
		}

		$pos1 = strrpos($long, 'rendező:');
		$pos2 = strrpos($long, 'szereplő(k)');

		$minpos = false;

		if($pos1 !== false && ($minpos == false || $pos1 < $minpos))
		{
			$minpos = $pos1;
		}

		if($pos2 !== false && ($minpos === false || $pos2 < $minpos))
		{
			$minpos = $pos2;
		}

		if($minpos !== false)
		{
			$long_director = null;
			$long_actor = null;

			if($pos1 !== false)
			{
				$pos1start = $pos1 + strlen('rendező:');
				$pos1end = $pos2 !== false && $pos2 > $pos1 ? $pos2 : strlen($long);

				$this->AddCast($directors, trim(substr($long, $pos1start, $pos1end - $pos1start)));
			}

			if($pos2 !== false)
			{
				$pos2start = $pos2 + strlen('szereplő(k)');
				$pos2end = $pos1 !== false && $pos1 > $pos2 ? $pos1 : strlen($long);

				$this->AddCast($actors, trim(substr($long, $pos2start, $pos2end - $pos2start)));
			}

			$long = trim(substr($long, 0, $minpos));
		}

		if($short == '-')
		{
			$short = '';
		}

		$description = $short;

		if(!empty($long))
		{
			$separator = " - ";

			$morechars = array('á', 'é', 'ó', 'ö', 'ő', 'ú', 'ü', 'ű', '.', ',');

			if($long[0] >= 'a' && $long[0] <= 'z' || $this->StartsWith($long, $morechars))
			{
				$separator = ' ';
			}

			if(!empty($description))
			{
				$description .= $separator;
			}

			$description .= $long;
		}

		if(ereg('^([0-9]+)\. rész(.*)', $description, $matches))
		{
			$episodeNumber = $matches[1];
			$description = $matches[2];
		}
		else if(ereg('^([0-9]+)/([0-9]+)\. rész(.*)', $description, $matches))
		{
			$episodeNumber = $matches[2];
			$episodeCount = $matches[1];
			$description = $matches[3];
		}

		$description = ltrim($description, ' -');

		return $description;
	}

	private function IncStats($param)
	{
		if(!isset($this->stats[$param]))
		{
			$this->stats[$param] = 0;
		}

		$this->stats[$param]++;
	}

	private function FindByPorthuId($table, $porthuId)
	{
		$id = 0;

		if($porthuId > 0)
		{
			if(isset($this->porthuMap[$table.$porthuId]))
			{
				return $this->porthuMap[$table.$porthuId];
			}

			if($row = $this->db->queryRow("SELECT Id FROM `$table` WHERE PorthuId = ".$this->db->quote($porthuId)))
			{
				$this->IsError($row);

				$id = $row[0];
			}
		}

		return $id;
	}

	private function FindChannel($porthuId, $porthuName)
	{
		$id = 0;

		$row = $this->db->queryRow("SELECT Id, PorthuName FROM Channel WHERE PorthuId = ".$this->db->quote($porthuId)." AND PorthuId > 0");

		$this->IsError($row);

		if(!empty($row))
		{
			if($row[1] != $porthuName)
			{
				$err = "ERROR: PorthuName '{$row[1]}' != '$porthuName' (PorthuChannelId = $porthuId) ";
				echo $err;
				@mail2('gabest11@gmail.com', 'epg import hiba', $err);
				//exit;

				$req =
					'UPDATE Channel SET '.
					' Name = '.$this->db->quote($porthuName, 'text').', '.
					' PorthuName = '.$this->db->quote($porthuName, 'text').' '.
					'WHERE Id = '.$row[0];

				$res = $this->db->query($req);

				$this->IsError($res);
			}

			$id = $row[0];
		}
		else
		{
			$req = 'SELECT Id FROM Channel WHERE (PorthuId <> 0 OR PorthuId = 0) AND PorthuName = '.$this->db->quote($porthuName, 'text');

			$res = $this->db->queryCol($req);

			$this->IsError($res);

			if(count($res) == 1)
			{
				$id = $res[0];

				$req = 'UPDATE Channel SET PorthuId = '.$this->db->quote($porthuId).' WHERE Id = '.$id;

				$res = $this->db->query($req);

				$this->IsError($res);
			}
		}

		return $id;
	}

	private function CreateChannel($porthuId, $porthuName)
	{
		$params = array($porthuName, null, null, $porthuId, $porthuName);

		$res = $this->channelInsert->execute($params);

		$this->IsError($res);

		$id = $this->db->lastInsertId();

		echo "Channel: $porthuName ($porthuId => $id)\r\n";

		$this->IncStats('Channel');

		return $id;
	}

	private function CreateProgram($porthuId, $channelId, $episodeId, $startDate, $stopDate, $isRepeat, $isLive)
	{
		$params = array($channelId, $episodeId, $startDate, $stopDate, $isRepeat, $isLive, $porthuId);

		$res = $this->programInsert->execute($params);

		$this->IsError($res);

		$id = $this->db->lastInsertId();

		$this->porthuMap['Program'.$porthuId] = $id;

		echo "Program: {$startDate} - {$stopDate} [$channelId, $episodeId] ($porthuId => $id)\r\n";

		$this->IncStats('Program');

		return $id;
	}

	private function UpdateProgram($programId, $episodeId, $startDate, $stopDate, $isRepeat, $isLive)
	{
		$res = $this->db->query(
			'UPDATE Program SET '.
			' EpisodeId = '.$episodeId.', '.
			' StartDate = \''.$startDate.'\', '.
			' StopDate = \''.$stopDate.'\', '.
			' IsRepeat = '.($isRepeat ? 1 : 0).', '.
			' IsLive = '.($isLive ? 1 : 0).' '.
			'WHERE Id = '.$programId.' AND (EpisodeId <> '.$episodeId.' OR StartDate <> \''.$startDate.'\' OR StopDate <> \''.$stopDate.'\')');

		$this->IsError($res);
	}

	private function FindEpisode($porthuId, $porthuProgramId, $title)
	{
		if($porthuId > 0)
		{
			return $this->FindByPorthuId('Episode', $porthuId);
		}
		else if($porthuProgramId > 0 && !empty($title))
		{
			$res = $this->db->queryOne(
				'SELECT t2.Id FROM Movie t1 '.
				'JOIN Episode t2 ON t2.MovieId = t1.Id '.
				'JOIN Program t3 ON t3.EpisodeId = t2.Id '.
				'WHERE t3.PorthuId = '.$this->db->quote($porthuProgramId).' '.
				'  AND t1.Title = '.$this->db->quote($title, 'text'));

			$this->IsError($res);

			return $res;
		}

		return 0;
	}

	private function CreateEpisode($porthuId, $movieId, $title, $description, $episodeNumber)
	{
		$params = array($movieId, $title, $description, $episodeNumber, $porthuId);

		$res = $this->episodeInsert->execute($params);

		$this->IsError($res);

		$id = $this->db->lastInsertId();

		$this->porthuMap['Episode'.$porthuId] = $id;

		echo "Episode: $title [$episodeNumber] ($porthuId => $id)\r\n";

		$this->IncStats('Episode');

		return $id;
	}

	private function FindMovie($porthuId, $porthuEpisodeId, $porthuProgramId, $title)
	{
		if(!empty($porthuId))
		{
			return $this->FindByPorthuId('Movie', $porthuId);
		}
		else if(!empty($porthuEpisodeId))
		{
			$res = $this->db->queryOne(
				'SELECT t1.Id FROM Movie t1 '.
				'JOIN Episode t2 ON t2.MovieId = t1.Id '.
				'WHERE t2.PorthuId = '.$this->db->quote($porthuEpisodeId));

			$this->IsError($res);

			return $res;
		}
		else if(!empty($porthuProgramId) && !empty($title))
		{
			$res = $this->db->queryOne(
				'SELECT t1.Id FROM Movie t1 '.
				'JOIN Episode t2 ON t2.MovieId = t1.Id '.
				'JOIN Program t3 ON t3.EpisodeId = t2.Id '.
				'WHERE t3.PorthuId = '.$this->db->quote($porthuProgramId).' '.
				'  AND t1.Title = '.$this->db->quote($title));

			$this->IsError($res);

			return $res;
		}

		return 0;
	}

	private function CreateMovie($porthuId, $movieTypeId, $dvbCategoryId, $title, $rating, $year, $episodeCount)
	{
		$params = array($movieTypeId, $dvbCategoryId, $title, $rating, $year, $episodeCount, $porthuId);

		$res = $this->movieInsert->execute($params);

		$this->IsError($res);

		$id = $this->db->lastInsertId();

		$this->porthuMap['Movie'.$porthuId] = $id;

		echo "Movie: $title [$year, $rating, $episodeCount] ($porthuId => $id)\r\n";

		$this->IncStats('Movie');

		return $id;
	}

	private function MakeMovieTypeHash($nameShort, $nameLong)
	{
		return $nameShort.'|'.$nameLong;
	}

	private function FindMovieType($nameShort, $nameLong)
	{
		$id = 0;

		$hash = $this->MakeMovieTypeHash($nameShort, $nameLong);

		if(isset($this->movieTypeMap[$hash]))
		{
			$id = $this->movieTypeMap[$hash];
		}

		return $id;
	}

	private function CreateMovieType($nameShort, $nameLong)
	{
		if(empty($nameShort))
			return 0;

		if(empty($nameLong))
			return 0;

		$params = array($nameShort, $nameLong);

		$res = $this->movieTypeInsert->execute($params);

		$this->IsError($res);

		$id = $this->db->lastInsertId();

		echo "MovieType: $nameShort, $nameLong ($id)\r\n";

		$this->IncStats('MovieType');

		$movieTypeHash = $this->MakeMovieTypeHash($nameShort, $nameLong);

		$this->movieTypeMap[$movieTypeHash] = $id;

		return $id;
	}

	private function FindDVBCategory($name)
	{
		$id = 0;

		if(isset($this->dvbCategoryMap[$name]))
		{
			$id = $this->dvbCategoryMap[$name];
		}

		return $id;
	}

	private function CreateDVBCategory($name)
	{
		if(empty($name))
			return 0;

		$params = array($name);

		$res = $this->dvbCategoryInsert->execute($params);

		$this->IsError($res);

		$id = $this->db->lastInsertId();

		echo "DVBCategory: $name ($id)\r\n";

		$this->IncStats('DVBCategory');

		$this->dvbCategoryMap[$name] = $id;

		return $id;
	}

	private function CreateCast($porthuId, $names)
	{
		$params = array(
			count($names) == 2 ? $names[1] : $names[0],
			count($names) == 2 ? $names[0] : '',
			$porthuId);

		$res = $this->castInsert->execute($params);

		$this->IsError($res);

		$id = $this->db->lastInsertId();

		$this->porthuMap['Cast'.$porthuId] = $id;

		echo "Cast: ".implode(': ', $names)." ($porthuId => $id)\r\n";

		$this->IncStats('Cast');

		return $id;
	}

	private function MakeEpisodeCastHash($episodeId, $castId, $roleId)
	{
		return $episodeId.' '.$castId.' '.$roleId;
	}

	private function CreateEpisodeCast($episodeId, $castId, $roleId, $names)
	{
		$hash = $this->MakeEpisodeCastHash($episodeId, $castId, $roleId);

		$movieName = null;

		if(isset($this->episodeCastMap[$hash]))
		{
			if(count($names) == 2) 
			{
				$movieName = $names[0];
			}

			if(!empty($movieName))
			{
				$req =
					'UPDATE EpisodeCast '.
					'SET MovieName = '.$this->db->quote($movieName).' '.
					'WHERE EpisodeId = '.intval($episodeId).
					'  AND CastId = '.intval($castId).
					'  AND RoleId = '.intval($roleId);

				echo $req."\r\n";

				$this->db->query($req);
			}

			return;
		}

		$params = array($episodeId, $castId, $roleId, $movieName);

		$res = $this->episodeCastInsert->execute($params);

		$this->IsError($res);

		$this->episodeCastMap[$hash] = true;

		echo "EpisodeCast: [$episodeId, $castId, $roleId, $movieName]\r\n";

		$this->IncStats('EpisodeCast');
	}
}

?>