<?php

require_once(dirname(__FILE__).'/../../class/guid.php');
require_once(dirname(__FILE__).'/../../class/data.php');
require_once(dirname(__FILE__).'/../../config.php');
require_once(dirname(__FILE__).'/../../util.php');

class Service
{
	/**
	 * @var MDB2_Driver_Common
	 */

	private $db;

	/**
	 * @var Repository
	 */

	private $repository;

	public function __construct()
	{
		$this->repository = new Repository();
		$this->db =& $this->repository->GetDB();
	}

	private function LookupUserId($sessionId)
	{
		//$this->repository->BeginTransaction();

		$req = 'UPDATE Session SET AccessedAt = NOW() WHERE Id = '.$this->db->quote($sessionId);

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		$req = 'SELECT UserId FROM Session WHERE Id = '.$this->db->quote($sessionId);

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		//$this->repository->Commit();

		return $res;
	}

	private function LookupMachineId($id)
	{
		$req = 'SELECT MachineId FROM Tuner WHERE Id = '.$this->db->quote($id, 'text');

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if(!empty($res)) return $res;

		$req = 'SELECT Id FROM Machine WHERE Id = '.$this->db->quote($id, 'text').' OR UserId = '.$this->db->quote($id, 'text');

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if(!empty($res)) return $res;

		return $id;
	}

	private function CheckSession($sessionId, $access_list = null)
	{
		$userId = $this->LookupUserId($sessionId);

		if($userId == null)
		{
			throw new SoapFault('Server', 'Invalid sessionId');
		}

		if(!empty($access_list))
		{
			if(!is_array($access_list))
			{
				$access_list = array($access_list);
			}

			$res = $this->db->queryOne('SELECT Username FROM User WHERE Id = '.$this->db->quote($userId, 'text'));

			$this->repository->IsError($res);

			if(array_search($res, $access_list) === false)
			{
				throw new SoapFault('Server', 'Access denied');
			}
		}

		if(!empty($_SERVER['REMOTE_ADDR']))
		{
			$this->db->query(
				'UPDATE User '.
				'SET LastSeenBefore = LastSeen '.
				'WHERE Id = '.$this->db->quote($userId).' '.
				'AND LastSeen IS NOT NULL '.
				'AND TIMESTAMPDIFF(SECOND, LastSeen, NOW()) >= 3600 '
				);

			$this->db->query(
				'UPDATE User SET '.
				' LastSeen = NOW(), '.
				' LastIP = '.$this->db->quote($_SERVER['REMOTE_ADDR']).' '.
				'WHERE Id = '.$this->db->quote($userId)
				);
		}

		return $userId;
	}

	private function WhereModified($userId, $timestamp)
	{
		// TODO: timestamp override

		if(!is_int($timestamp))
		{
			$timestamp = strtotime($timestamp);
		}

		if(empty($timestamp))
		{
			$timestamp = 0;
		}
/*
		if($userId)
		{
			$res = $this->db->queryOne(
				'SELECT COUNT(*) FROM User '.
				'WHERE Id = '.$this->db->quote($userId, 'text').' '.
				'  AND HomesysUpdate IS NULL ');

			$this->repository->IsError($res);

			if($res > 0) $timestamp = 0;
		}
*/
		return 'Modified > FROM_UNIXTIME('.$timestamp.')';
	}

	/**
	 * @param string $username .
	 * @param string $password .
	 * @return string .
	 */

	public function InitiateSession($username, $password)
	{
		$userId = $this->repository->Login($username, $password);

		if(empty($userId))
		{
			// throw new SoapFault('Server', 'Invalid login');

			if(0)
			//if(!empty($username))
			{
				if($fp = fopen('c:/temp/login.txt', 'a'))
				{
					fprintf($fp, "%s, %s, %s, %s\r\n", date('c'), $_SERVER['REMOTE_ADDR'], $username, $password);

					fclose($fp);
				}
			}

			return '';
		}

		if(empty($userId))
		{
			throw new SoapFault('Server', 'Invalid login');
		}

		//$this->repository->BeginTransaction();

		$sessionId = Guid::Generate();

		$req =
			'INSERT INTO Session (Id, UserId, CreatedAt, AccessedAt) VALUES ('.
			$this->db->quote($sessionId).', '.
			$this->db->quote($userId).', '.
			'NOW(), NOW() '.
			')';

		$res = $this->db->query($req);

		$this->repository->IsError($res);

		//$this->repository->Commit();

		return $sessionId;
	}

	/**
	 * @param string $sessionId .
	 * @param string $lang .
	 */

	public function SetLanguage($sessionId, $lang)
	{
		$userId = $this->CheckSession($sessionId);

		$req =
			'UPDATE User '.
			'SET Lang = '.$this->db->quote($lang, 'text').' '.
			'WHERE Id = '.$this->db->quote($userId, 'text');

		$res = $this->db->query($req);

		$this->repository->IsError($res);
	}

	/**
	 * @param string $username .
	 * @param string $password .
	 * @param string $lang .
	 * @return string .
	 */

	public function InitiateSessionWithLang($username, $password, $lang)
	{
		$sessionId = $this->InitiateSession($username, $password);

		if($sessionId != null)
		{
			$this->SetLanguage($sessionId, $lang);
		}

		return $sessionId;
	}

	/**
	 * @param string $username .
	 * @return boolean .
	 */

	public function UserExists($username)
	{
		// homesys

		$req = 'SELECT COUNT(*) FROM User WHERE Username = '.$this->db->quote($username, 'text');

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if($res > 0) return true;

		//
		
		return false;
	}

	/**
	 * @param string $username .
	 * @param string $password .
	 * @return boolean .
	 */

	public function LoginExists($username, $password)
	{
		$where = 'Username = '.$this->db->quote($username, 'text').' AND Password = '.$this->db->quote(md5($username.$password), 'text');

		$res = $this->db->queryOne('SELECT COUNT(*) FROM User WHERE '.$where);

		$this->repository->IsError($res);

		return $res > 0;
	}

	/**
	 * @param string $username .
	 * @param string $password .
	 * @param string $email .
	 * @return string .
	 */

	public function RegisterUser($username, $password, $email)
	{
		$id = $this->repository->Register($username, $password, $email);

		if($id == null) return null; // TODO: SoapFault

		return $id;
	}

	/**
	 * @param string $sessionId .
	 * @param string $username .
	 * @param string $password .
	 * @param string $email .
	 * @return string .
	 */

	public function Register($sessionId, $username, $password, $email)
	{
		$this->CheckSession($sessionId, 'admin');

		$id = $this->repository->Register($username, $password, $email);

		if($id == null) return null; // TODO: SoapFault

		return $id;
	}

	/**
	 * @param string $sessionId .
	 * @param UserInfo $userInfo .
	 */

	public function SetUserInfo($sessionId, $userInfo)
	{
		$userId = $this->CheckSession($sessionId);

		if($userInfo->Email != null)
		{
			$req =
				'UPDATE User SET '.
				' Email = '.$this->db->quote($userInfo->Email, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}

		if($userInfo->FirstName != null)
		{
			$req =
				'UPDATE User SET '.
				' FirstName = '.$this->db->quote($userInfo->FirstName, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}

		if($userInfo->LastName != null)
		{
			$req =
				'UPDATE User SET '.
				' LastName = '.$this->db->quote($userInfo->LastName, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}

		if($userInfo->Country != null)
		{
			$req =
				'UPDATE User SET '.
				' Country = '.$this->db->quote($userInfo->Country, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}

		if($userInfo->Address != null)
		{
			$req =
				'UPDATE User SET '.
				' Address = '.$this->db->quote($userInfo->Address, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}

		if($userInfo->PostalCode != null)
		{
			$req =
				'UPDATE User SET '.
				' PostalCode = '.$this->db->quote($userInfo->PostalCode, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}

		if($userInfo->PhoneNumber != null)
		{
			$req =
				'UPDATE User SET '.
				' PhoneNumber = '.$this->db->quote($userInfo->PhoneNumber, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}

		if($userInfo->Gender != null)
		{
			$req =
				'UPDATE User SET '.
				' Gender = '.$this->db->quote($userInfo->Gender, 'text').' '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$this->db->query($req);

			$this->repository->IsError($res);
		}
	}

	/**
	 * @param string $sessionId .
	 * @return UserInfo .
	 */

	public function GetUserInfo($sessionId)
	{
		$userId = $this->CheckSession($sessionId);

		$userInfo = new UserInfo();

		$req = 'SELECT '.
			' Email, '.
			' FirstName, '.
			' LastName, '.
			' Country, '.
			' Address, '.
			' PostalCode, '.
			' PhoneNumber, '.
			' Gender '.
			'FROM User '.
			'WHERE Id = '.$this->db->quote($userId, 'text');

		$res = $this->db->queryRow($req);

		$this->repository->IsError($res);

		$userInfo->Email = $res[0];
		$userInfo->FirstName = $res[1];
		$userInfo->LastName = $res[2];
		$userInfo->Country = $res[3];
		$userInfo->Address = $res[4];
		$userInfo->PostalCode = $res[5];
		$userInfo->PhoneNumber = $res[6];
		$userInfo->Gender = $res[7];

		return $userInfo;
	}


	/**
	 * @param string $sessionId .
	 * @param string $machineName .
	 * @param TunerData[] $tunerDataList .
	 * @return MachineRegistrationData .
	 */

	public function RegisterMachine($sessionId, $machineName, $tunerDataList)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$this->repository->BeginTransaction();

		//

		$req = 'SELECT Id FROM Machine WHERE UserId = '.$this->db->quote($userId, 'text').' AND Name = '.$this->db->quote($machineName, 'text');

		$res = $this->db->queryCol($req);

		$this->repository->IsError($res);

		foreach($res as $machineId)
		{
			$id = $this->db->quote($machineId, 'text');

			$this->db->query('DELETE FROM Recording WHERE MachineId = '.$id);
			$this->db->query('DELETE FROM Tuner WHERE MachineId = '.$id);
			$this->db->query('DELETE FROM Machine WHERE Id = '.$id);
		}

		//

		$mrd = new MachineRegistrationData(Guid::Generate());

		//

		$data = array('Id' => $mrd->MachineId, 'UserId' => $userId, 'Name' => $machineName);

		$types = array('text', 'text', 'text');

		$res = $this->db->autoExecute('Machine', $data, MDB2_AUTOQUERY_INSERT, null, $types);

		$this->repository->IsError($res);

		//

		foreach($tunerDataList as $tunerData)
		{
			$tuner = new Tuner($tunerData->SequenceNumber, Guid::Generate());

			//

			$data = array(
				'Id' => $tuner->Id,
				'MachineId' => $mrd->MachineId,
				'ClientTunerData' => $tunerData->ClientTunerData,
				'FriendlyName' => $tunerData->FriendlyName,
				'DisplayName' => $tunerData->DisplayName);

			$types = array('text', 'text', 'blob', 'text', 'text');

			$res = $this->db->autoExecute('Tuner', $data, MDB2_AUTOQUERY_INSERT, null, $types);

			$this->repository->IsError($res);

			//

			$mrd->TunerList[] = $tuner;
		}

		//

		$this->repository->Commit();

		//

		return $mrd;
	}

	/**
	 * @param string $sessionId .
	 * @param guid $machineId .
	 */

	public function UnregisterMachine($sessionId, $machineId)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$this->repository->BeginTransaction();

		//

		$where = 'Id = '.$this->db->quote($machineId, 'text').' AND UserId = '.$this->db->quote($userId, 'text');

		$res = $this->db->autoExecute('Machine', null, MDB2_AUTOQUERY_DELETE, $where);

		$this->repository->IsError($res);

		//

		$where = 'MachineId = '.$this->db->quote($machineId, 'text');

		$res = $this->db->autoExecute('Tuner', null, MDB2_AUTOQUERY_DELETE, $where);

		$this->repository->IsError($res);

		//

		$res = $this->db->autoExecute('Recording', null, MDB2_AUTOQUERY_DELETE, $where);

		$this->repository->IsError($res);

		//

		$this->repository->Commit();
	}

	/**
	 * @param string $sessionId .
	 * @return Machine[] .
	 */

	public function ListMachines($sessionId)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$machines = array();

		$where = 'UserId = '.$this->db->quote($userId, 'text');

		$res = $this->db->query('SELECT Id, Name FROM Machine WHERE '.$where);

		$this->repository->IsError($res);

		while($row = $res->fetchRow())
		{
			$machines[] = new Machine($row[0], $row[1]);;
		}

		return $machines;
	}

	/**
	 * @param string $sessionId .
	 * @param guid $machineId .
	 * @param int $programId .
	 * @return int .
	 */

	public function AddRecording($sessionId, $machineId, $programId)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$req = 'SELECT COUNT(*) FROM Program WHERE Id = '.(int)$programId;

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if($res == 0) return -1;

		//

		$machineId = $this->LookupMachineId($machineId);

		// TODO: check if the user really owns this machine?

		$req = 'SELECT Id FROM Recording WHERE ProgramId = '.(int)$programId.' AND MachineId = '.$this->db->quote($machineId, 'text');

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if(!empty($res))
		{
			return $res;
		}

		$req = 'DELETE FROM Recording WHERE ProgramId = '.(int)$programId.' AND MachineId = '.$this->db->quote($machineId, 'text');

		$res = $this->db->query($req);

		$this->repository->IsError($res);

		$data = array(
			'ProgramId' => $programId,
			'UserId' => $userId,
			'MachineId' => $machineId,
			'RecordingState' => RecordingState::Scheduled
			);

		$types = array('integer', 'text', 'text', 'text');

		$res = $this->db->autoExecute('Recording', $data, MDB2_AUTOQUERY_INSERT, null, $types);

		$this->repository->IsError($res);

		//

		$id = $this->db->lastInsertID('Recording');

		return $id;
	}

	/**
	 * @param string $sessionId .
	 * @param int $recordingId .
	 * @param string $recordingState .
	 */

	public function UpdateRecording($sessionId, $recordingId, $recordingState)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$data = array('RecordingState' => $recordingState);

		$types = array('text');

		$where = 'Id = '.(int)$recordingId.' AND UserId = '.$this->db->quote($userId, 'text');

		$res = $this->db->autoExecute('Recording', $data, MDB2_AUTOQUERY_UPDATE, $where, $types);

		$this->repository->IsError($res);
	}

	/**
	 * @param string $sessionId .
	 * @param int $recordingId .
	 */

	public function DeleteRecording($sessionId, $recordingId)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$where = 'Id = '.(int)$recordingId.' AND UserId = '.$this->db->quote($userId, 'text');

		$res = $this->db->autoExecute('Recording', null, MDB2_AUTOQUERY_DELETE, $where);

		$this->repository->IsError($res);
	}

	/**
	 * @param string $sessionId .
	 * @param boolean $futureOnly .
	 * @return Recording[] .
	 */

	public function ListRecordings($sessionId, $futureOnly)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$where = 't1.UserId = '.$this->db->quote($userId, 'text');

		if($futureOnly) $where .= ' AND t2.StartDate >= NOW()';

		$req =
			'SELECT '.
			' t1.Id, t1.ProgramId, t1.RecordingState, t1.StateText, t1.MachineId, '.
			' t2.ChannelId, t2.StartDate, t2.StopDate '.
			'FROM Recording t1 '.
			'JOIN Program t2 ON t2.Id = t1.ProgramId '.
			'WHERE '.$where;

		$res = $this->db->query($req);

		$this->repository->IsError($res);

		$recordings = array();

		while($row = $res->fetchRow())
		{
			$r = new Recording();

			$r->Id = $row[0];
			$r->ProgramId = $row[1];
			$r->RecordingState = $row[2];
			$r->StateText = $row[3];
			$r->MachineId = $row[4];
			$r->TunerId = null; // not used
			$r->ChannelId = $row[5];
			$r->StartDate = SoapServerEx::FixDateTime($row[6]);
			$r->StopDate = SoapServerEx::FixDateTime($row[7]);

			$recordings[] = $r;
		}

		return $recordings;
	}

	/**
	 * @param string $sessionId .
	 * @param int $programId .
	 * @return int .
	 */

	public function AddRecordingByPortId($sessionId, $programId)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$req = 'SELECT Id FROM Program WHERE PorthuId = '.(int)$programId;

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if(empty($res)) return 0;

		$programId = $res;

		//

		$req = 'SELECT Id FROM Machine WHERE UserId = '.$this->db->quote($userId, 'text');

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if(empty($res)) $res = ''; // return 0;

		$machineId = $res;

		//

		$req =
			'SELECT Id FROM Recording '.
			'WHERE UserId = '.$this->db->quote($userId, 'text').' AND ProgramId = '.$programId;

		$res = $this->db->queryOne($req);

		$this->repository->IsError($res);

		if(!empty($res))
		{
			$id = $res;

			$req =
				'UPDATE Recording '.
				'SET RecordingState = '.$this->db->quote(RecordingState::Scheduled, 'text').' '.
				'WHERE Id = '.$id;

			$res = $this->db->query($req);

			$this->repository->IsError($res);
		}
		else
		{
			$req =
				'INSERT INTO Recording (ProgramId, UserId, MachineId, RecordingState) '.
				'VALUES ('.
				$programId.', '.
				$this->db->quote($userId, 'text').', '.
				$this->db->quote($machineId, 'text').', '.
				$this->db->quote(RecordingState::Scheduled, 'text').
				')';

			$res = $this->db->query($req);

			$this->repository->IsError($res);

			$id = $this->db->lastInsertId('Recording');
		}

		return $id;
	}

	/**
	 * @param string $sessionId .
	 * @param int $programId .
	 */

	public function DeleteRecordingByPortId($sessionId, $programId)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$subreq = 'SELECT Id FROM Program WHERE PorthuId = '.(int)$programId;

		$req =
			'UPDATE Recording SET RecordingState = \'Deleted\' '.
			'WHERE UserId = '.$this->db->quote($userId, 'text').' AND ProgramId = ('.$subreq.')';

		$res = $this->db->exec($req);

		$this->repository->IsError($res);
	}

	/**
	 * @param string $sessionId .
	 * @param boolean $futureOnly .
	 * @return Recording[] .
	 */

	public function ListRecordingsByPortId($sessionId, $futureOnly)
	{
		$userId = $this->CheckSession($sessionId);

		//

		$where = 't1.UserId = '.$this->db->quote($userId, 'text');

		if($futureOnly) $where .= ' AND t2.StartDate >= NOW()';

		$res = $this->db->query(
			'SELECT '.
			' t1.Id, t2.PorthuId, t1.RecordingState, t1.StateText, t1.MachineId, '.
			' t2.ChannelId, t2.StartDate, t2.StopDate '.
			'FROM Recording t1 '.
			'JOIN Program t2 ON t2.Id = t1.ProgramId '.
			'WHERE '.$where);

		$this->repository->IsError($res);

		$recordings = array();

		while($row = $res->fetchRow())
		{
			$r = new Recording();

			$r->Id = $row[0];
			$r->ProgramId = $row[1];
			$r->RecordingState = $row[2];
			$r->StateText = $row[3];
			$r->MachineId = $row[4];
			$r->TunerId = null; // not used
			$r->ChannelId = $row[5];
			$r->StartDate = SoapServerEx::FixDateTime($row[6]);
			$r->StopDate = SoapServerEx::FixDateTime($row[7]);

			$recordings[] = $r;
		}

		return $recordings;
	}

	/**
	 * @param string $name .
	 * @param string $version .
	 * @return AppVersion .
	 */

	public function GetAppVersion($name, $version)
	{
		global $g_config;
		
		if(!empty($_SERVER['REMOTE_ADDR']))
		{
			$req =
				'SELECT Id, Username FROM User '.
				'WHERE LastIP = '.$this->db->quote($_SERVER['REMOTE_ADDR'], 'text').' '.
				'ORDER BY LastSeen DESC ';

			$res = $this->db->queryRow($req);
			/*
			if($fp = fopen('c:/version.txt', 'a'))
			{
				fprintf($fp, "%s | %s | %s | %s | %s\r\n", strftime('%Y-%m-%d %H:%M:%S'), $_SERVER['REMOTE_ADDR'], $name, $version, !empty($res) ? $res[1] : '????');

				fclose($fp);
			}
			*/
			if(!empty($res))
			{
				$req =
					'UPDATE User '.
					'SET HomesysVersion = '.$this->db->quote($version, 'text').' '.
					'WHERE Id = '.$this->db->quote($res[0], 'text');

				$res = $this->db->query($req);
			}
		}

		// $version = AppVersion::StringToVersion($version);

		$baseurl = !empty($_SERVER['HTTP_HOST']) ? 'http://'.$_SERVER['HTTP_HOST'].'/' : $g_config['site']['url'];

		$av = new AppVersion();

		$sl = explode('/', $name, 2);

		$appname = $sl[0];
		$subname = !empty($sl[1]) ? $sl[1] : null;

		$files = array();

		if($dh = opendir(dirname(__FILE__).'/../download/'.$name))
		{
			while(($file = readdir($dh)) !== false)
			{
				if(!preg_match('/'.$appname.'_([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)\.zip/', $file, $matches))
				{
					continue;
				}

				$files[] = array($file, (int)$matches[1], (int)$matches[2], (int)$matches[3], (int)$matches[4]);
			}

			closedir($dh);
		}

		usort($files, array('Service', 'cmp_appversion'));

		if(!empty($files))
		{
			$f = array_shift($files);

			$av->Version = AppVersion::VersionToString($f[1], $f[2], $f[3], $f[4]);
			$av->Url = $baseurl.'download/'.$name.'/'.$f[0];
			$av->Required = false;

			return $av;
		}

		return $av;
	}

	static private function cmp_appversion($a, $b)
	{
		for($i = 1; $i <= 4; $i++)
		{
			if($a[$i] < $b[$i]) return +1;
			if($a[$i] > $b[$i]) return -1;
		}

		return 0;
	}

	/**
	 * @return string .
	 * @todo deprecated
	 */

	public function VersionCheck()
	{
		// TODO

		return '';
	}

	/**
	 * @param string $sessionId .
	 * @return dateTime .
	 */

	public function BeginUpdate($sessionId)
	{
		$now = $this->db->queryOne("SELECT UNIX_TIMESTAMP(NOW())");

		if(!empty($sessionId))
		{
			$userId = $this->CheckSession($sessionId);

			$req =
				'SELECT HomesysVersion, Username, UNIX_TIMESTAMP(HomesysUpdate), TIMESTAMPDIFF(DAY, HomesysUpdate, NOW()) '.
				'FROM User '.
				'WHERE Id = '.$this->db->quote($userId, 'text');

			$res = $this->db->QueryRow($req);

			if(!empty($res))
			{
				$version = AppVersion::StringToVersion($res[0]);

				if(!empty($version) && $version[0] == '2')
				{
					if($fp = fopen('c:/version.txt', 'a'))
					{
						fprintf($fp, "*** %s | %s | %s | %s\r\n", strftime('%Y-%m-%d %H:%M:%S'), $_SERVER['REMOTE_ADDR'], $res[0], $res[1]);

						fclose($fp);
					}

					throw new SoapFault('Server', 'Version number too low, please upgrade.');
				}

				if((int)$res[3] <= 3 && (int)$res[2] > 0)
				{
					// $now = $res[2];
				}
			}

			$res = $this->db->query('UPDATE User SET HomesysBeginUpdate = FROM_UNIXTIME('.$now.') WHERE Id = '.$this->db->quote($userId, 'text'));
		}

		return SoapServerEx::FixDateTime($now);
	}

	/**
	 * @param string $sessionId .
	 */

	public function EndUpdate($sessionId)
	{
		if(!empty($sessionId))
		{
			$userId = $this->CheckSession($sessionId);

			$res = $this->db->queryOne('UPDATE User SET HomesysUpdate = HomesysBeginUpdate WHERE Id = '.$this->db->quote($userId, 'text'));

			$this->repository->IsError($res);
		}
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return TuneRequestPackage[] .
	 */

	public function GetTuneRequestPackages($sessionId, $timestamp)
	{
		return $this->GetTuneRequestPackagesForRegion($sessionId, 'hun', $timestamp);
	}

	/**
	 * @param string $sessionId .
	 * @param string $region .
	 * @param dateTime $timestamp .
	 * @return TuneRequestPackage[] .
	 */

	public function GetTuneRequestPackagesForRegion($sessionId, $region, $timestamp)
	{
		$packages = array();

		$userId = $this->LookupUserId($sessionId);

		// no subscription required

		$whereModified = $this->WhereModified($userId, $timestamp);

		$res = $this->db->queryOne('SELECT COUNT(*) FROM TuneRequestPackage WHERE '.$whereModified);

		$this->repository->IsError($res);

		$username = $this->db->queryOne('SELECT Username FROM User WHERE Id = '.$this->db->quote($userId, 'text'));

		if(empty($res) || $username != 'admin' && $username != 'admin_silver')
		{
			return null;
		}

		$req =
			"SELECT t1.Id, t1.Type, t1.Provider, t1.Name FROM TuneRequestPackage t1 ".
			"LEFT OUTER JOIN TuneRequestRegion t2 ON t2.PackageId = t1.Id ".
			"WHERE t2.RegionId IS NULL OR t2.RegionId = ".$this->db->quote($region, 'text')." ".
			"ORDER BY 2 ";

		$res = $this->db->query($req);

		$this->repository->IsError($res);

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

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				$tr = new TuneRequest();

				$tr->Frequency = $row[0];
				$tr->VideoStandard = $row[1];
				$tr->IFECMethod = $row[2];
				$tr->IFECRate = $row[3];
				$tr->OFECMethod = $row[4];
				$tr->OFECRate = $row[5];
				$tr->Modulation = $row[6];
				$tr->SymbolRate = $row[7];
				$tr->Polarisation = $row[8];
				$tr->LNBSource = $row[9];
				$tr->SpectralInversion = $row[10];
				$tr->Bandwidth = $row[11];
				$tr->LPIFECMethod = $row[12];
				$tr->LPIFECRate = $row[13];
				$tr->HAlpha = $row[14];
				$tr->Guard = $row[15];
				$tr->TransmissionMode = $row[16];
				$tr->OtherFrequencyInUse = $row[17];
				$tr->Path = $row[18];
				$tr->ChannelId = $row[19];

				$packages[$id]->TuneRequests[] = $tr;
			}
		}

		return $packages;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return Channel[] .
	 */

	public function GetChannels($sessionId, $timestamp)
	{
		$channels = array();

		$userId = $this->LookupUserId($sessionId);

		// no subscription required
		{
			$whereModified = $this->WhereModified($userId, $timestamp);

			$res = $this->db->query(
				'SELECT Id, Name, Url, Logo, Radio '.
				'FROM Channel '.
				'WHERE '.$whereModified.' '.
				'ORDER BY 5, 1 ');

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				$channels[] = new Channel($row[0], $row[1], $row[2], $row[3], $row[4] == 1);
			}
		}

		return $channels;
	}

	/**
	 * @param string $sessionId .
	 * @return int[] .
	 */

	public function GetChannelRank($sessionId)
	{
		$userId = $this->LookupUserId($sessionId);

		$channelIds = array();

		if(!empty($userId))
		{
			$req =
				'SELECT t2.Id, SUM(TIMESTAMPDIFF(SECOND, t1.Start, t1.Stop)) '.
				'FROM TvStat t1 '.
				'JOIN Channel t2 ON t2.id = t1.ChannelId '.
				'WHERE t1.UserId = '.$this->db->quote($userId, 'text').' AND TIMESTAMPDIFF(SECOND, t1.Start, t1.Stop) > 10 '.
				'AND t1.Start > DATE_ADD(NOW(), INTERVAL -6 MONTH) '.
				'GROUP BY 1 '.
				'ORDER BY 2 DESC ';

			$res = $this->db->query($req);

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				$channelIds[] = $row[0];
			}
		}

		return $channelIds;
	}

	/**
	 * @param string $sessionId .
	 * @param string $regionId .
	 * @param string[] $names .
	 */

	public function SetUnknownChannels($sessionId, $regionId, $names)
	{
		$userId = $this->LookupUserId($sessionId);

		$this->repository->BeginTransaction();

		if(!empty($userId))
		{
			$req = 'DELETE FROM ChannelUnknown WHERE UserId = '.$this->db->quote($userId, 'text'); // TODO: $regionId

			$res = $this->db->query($req);

			$this->repository->IsError($res);
		}

		foreach($names as $name)
		{
			if(strpos($name, 'Hz') !== false || strpos($name, 'SID=') !== false)
			{
				continue;
			}

			$req = 'SELECT COUNT(*) FROM ChannelAlias WHERE Name = '.$this->db->quote($name, 'text'); // TODO: regionId

			$res = $this->db->queryOne($req);

			$this->repository->IsError($res);

			if((int)$res == 0)
			{
				$req = 'SELECT COUNT(*) FROM ChannelUnknown WHERE Name = '.$this->db->quote($name, 'text');
				$req .= ' AND RegionId '.(!empty($regionId) ? ' = '.$this->db->quote($regionId, 'text') : 'IS NULL');
				$req .= ' AND UserId '.(!empty($userId) ? ' = '.$this->db->quote($userId, 'text') : 'IS NULL');

				$res = $this->db->queryOne($req);

				$this->repository->IsError($res);

				if((int)$res == 0)
				{
					$req = 'INSERT INTO ChannelUnknown (Name, RegionId, UserId) VALUES ('.$this->db->quote($name, 'text').', ';
					$req .= (!empty($regionId) ? $this->db->quote($regionId, 'text') : 'NULL').', ';
					$req .= (!empty($userId) ? $this->db->quote($userId, 'text') : 'NULL').' ';
					$req .= ')';

					$res = $this->db->query($req);

					$this->repository->IsError($res);
				}
			}
		}

		$this->repository->Commit();
	}

	/**
	 * @param string $sessionId .
	 * @return int[] .
	 */

	public function GetMovieRank($sessionId)
	{
		$userId = $this->LookupUserId($sessionId);

		$movieIds = array();

		if(!empty($userId))
		{
			$req =
				'SELECT DISTINCT Id FROM ( '.
				'SELECT t4.Id, t4.Title, t5.Name, COUNT(DISTINCT t2.Id) As Num, SUM(TIMESTAMPDIFF(SECOND, t1.Start, t1.Stop)) AS Dur '.
				'FROM TvStat t1 '.
				'JOIN Program t2 ON t2.Id = t1.ProgramId '.
				'JOIN Episode t3 ON t3.Id = t2.EpisodeId '.
				'JOIN Movie t4 ON t4.Id = t3.MovieId  '.
				'JOIN Channel t5 ON t5.Id = t2.ChannelId '.
				'WHERE t1.UserId = '.$this->db->quote($userId, 'text').' '.
				' AND TIMESTAMPDIFF(SECOND, t1.Start, t1.Stop) >= 600 '.
				' AND t1.Start > DATE_ADD(NOW(), INTERVAL -6 MONTH) '.
				' AND t4.Title NOT IN ('.$this->db->quote('Musorszunet', 'text').', '.$this->db->quote('Nincs Információ', 'text').') '.
				'GROUP BY 1, 2, 3 '.
				'ORDER BY t2.StartDate ASC '.
				') AS Res '.
				'WHERE Num >= 3 AND Dur >= 3600 ';

			$res = $this->db->query($req);

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				$movieIds[] = $row[0];
			}

			/*
			if($userId == '2b0fbf92-84d8-4cc2-93a7-b5d9ac2a99da'
			|| $userId == '2e47acb1-36a1-4fdb-bb19-44a84e41352e')
			{
				file_put_contents('c:/temp/movierank.'.$userId.'.txt', print_r(array($userId, $req, $movieIds), true));
			}
			*/
		}

		return $movieIds;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return Program[] .
	 */

	public function GetPrograms($sessionId, $timestamp)
	{
		$programs = array();

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			$whereModified = $this->WhereModified($userId, $timestamp);
/*
			$subscription = $this->GetSubscription($sessionId);

			if($subscription->Level != 'G')
			{
				$whereModified .= ' AND t1.ChannelId IN (1, 2, 3, 5) ';
			}
*/
			$req =
				'SELECT t1.Id, t1.ChannelId, t1.EpisodeId, t2.MovieId, t1.StartDate, t1.StopDate, t1.IsRepeat, t1.IsLive, t1.IsRecommended, t1.Deleted '.
				'FROM Program t1 '.
				'JOIN Episode t2 ON t1.EpisodeId = t2.Id '.
				'WHERE (t1.'.$whereModified.' OR t2.'.$whereModified.') '.
				'  AND t1.StopDate > CAST(CURDATE() AS datetime) ';

			$res = $this->db->query($req);

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				$programs[] = new Program($row[0], $row[1], $row[2], $row[3], SoapServerEx::FixDateTime($row[4]), SoapServerEx::FixDateTime($row[5]), $row[6], $row[7], $row[8], $row[9]);
			}
		}

		return $programs;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return Episode[] .
	 */

	public function GetEpisodes($sessionId, $timestamp)
	{
		$episodes = array();

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			$whereModified = $this->WhereModified($userId, $timestamp);

			$res = $this->db->query(
				'SELECT DISTINCT t1.Id, t1.MovieId, t1.Title, t1.Description, t1.EpisodeNumber, t1.PorthuId, t3.Description '.
				'FROM Episode t1 '.
				'JOIN Program t2 ON t2.EpisodeId = t1.Id '.
				'JOIN Movie t3 ON t3.Id = t1.MovieId '.
				'WHERE (t1.'.$whereModified.' OR t2.'.$whereModified.') '.
				'  AND t2.StopDate > CAST(CURDATE() AS datetime) ');

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				if($row[3] == $row[6]) $row[3] = '';
				if(mb_strlen($row[2], 'UTF-8') > 64) $row[2] = mb_substr($row[2], 0, 64, 'UTF-8');

				$episodes[(int)$row[0]] = new Episode($row[0], $row[1], $row[2], $row[3], $row[4], $row[5] == 0);
			}

			$res = $this->db->query(
				'SELECT DISTINCT t1.EpisodeId, t1.CastId, t1.RoleId, t1.MovieName '.
				'FROM EpisodeCast t1 '.
				'JOIN Episode t2 ON t2.Id = t1.EpisodeId '.
				'JOIN Program t3 ON t3.EpisodeId = t2.Id '.
				'WHERE (t2.'.$whereModified.' OR t3.'.$whereModified.') '.
				'  AND t3.StopDate > CAST(CURDATE() AS datetime) ');

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				if(isset($episodes[(int)$row[0]]))
				{
					if(mb_strlen($row[3], 'UTF-8') > 64) $row[3] = mb_substr($row[3], 0, 64, 'UTF-8');

					$episodes[(int)$row[0]]->EpisodeCastList[] = new EpisodeCast($row[1], $row[2], $row[3]);
				}
				else
				{
					$str = sprintf("%d\r\n%s", (int)$row[0], print_r($episodes, true));

					file_put_contents('c:\err.txt', $str);

					exit;
				}
			}
		}

		return $episodes;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return Movie[] .
	 */

	public function GetMovies($sessionId, $timestamp)
	{
		$movies = array();

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			$whereModified = $this->WhereModified($userId, $timestamp);

			$res = $this->db->query(
				'SELECT DISTINCT t1.Id, t1.MovieTypeId, t1.DVBCategoryId, t1.Title, t1.Description, t1.Rating, t1.Year, t1.EpisodeCount, t1.PorthuId, t1.PictureUrl IS NOT NULL '.
				'FROM Movie t1 '.
				'JOIN Episode t2 ON t2.MovieId = t1.Id '.
				'JOIN Program t3 ON t3.EpisodeId = t2.Id '.
				'WHERE (t1.'.$whereModified.' OR t2.'.$whereModified.' OR t3.'.$whereModified.') '.
				'  AND t3.StopDate > CAST(CURDATE() AS datetime) ');

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				if(mb_strlen($row[3], 'UTF-8') > 64) $row[3] = mb_substr($row[3], 0, 64, 'UTF-8');

				$movies[] = new Movie($row[0], $row[1], $row[2], $row[3], $row[4], $row[5], $row[6], $row[7], $row[8] == 0, $row[9] != 0);
			}
		}

		return $movies;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return Cast[] .
	 */

	public function GetCast($sessionId, $timestamp)
	{
		$cast = array();

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			$whereModified = $this->WhereModified($userId, $timestamp);

			$res = $this->db->query(
				'SELECT DISTINCT t1.Id, t1.Name '.
				'FROM Cast t1 '.
				'JOIN EpisodeCast t2 ON t2.CastId = t1.Id '.
				'JOIN Program t3 ON t3.EpisodeId = t2.EpisodeId '.
				'WHERE (t1.'.$whereModified.' OR t3.'.$whereModified.') '.
				'  AND t3.StopDate > CAST(CURDATE() AS datetime) ');

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				if(mb_strlen($row[1], 'UTF-8') > 64) $row[1] = mb_substr($row[1], 0, 64, 'UTF-8');

				$cast[] = new Cast($row[0], $row[1]);
			}
		}

		return $cast;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return MovieType[] .
	 */

	public function GetMovieTypes($sessionId, $timestamp)
	{
		$movieTypes = array();

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			$whereModified = $this->WhereModified($userId, $timestamp);

			$res = $this->db->query(
				'SELECT Id, NameShort, NameLong '.
				'FROM MovieType '.
				'WHERE '.$whereModified);

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				$movieTypes[] = new MovieType($row[0], $row[1], $row[2]);
			}
		}

		return $movieTypes;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return DVBCategory[] .
	 */

	public function GetDVBCategory($sessionId, $timestamp)
	{
		$dvbCategories = array();

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			$whereModified = $this->WhereModified($userId, $timestamp);

			$res = $this->db->query(
				'SELECT Id, Name '.
				'FROM DVBCategory '.
				'WHERE '.$whereModified);

			$this->repository->IsError($res);

			while($row = $res->fetchRow())
			{
				$dvbCategories[] = new DVBCategory($row[0], $row[1]);
			}
		}

		return $dvbCategories;
	}

	/**
	* @param string $sessionId .
	* @return RelatedProgram[] .
	*/

	public function GetRelatedPrograms($sessionId)
	{
		$rps = array();
/*
		$req =
			'SELECT t1.ProgramId, t1.OtherProgramId, t1.Type '.
			'FROM ProgramRelated t1 '.
			'JOIN Program t2 ON t2.Id = t1.ProgramId '.
			'WHERE t2.StopDate > NOW() ';

		$res = $this->db->query($req);

		while($row = $res->fetchRow())
		{
			$rp = new RelatedProgram();

			$rp->Id = $row[0];
			$rp->OtherId = $row[1];
			$rp->Type = $row[2];

			$rps[] = $rp;
		}
*/
		return $rps;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @return EPG .
	 */

	public function GetEPG($sessionId, $timestamp)
	{
		$this->CheckSession($sessionId, array('admin', 'admin_silver'));

		$this->repository->BeginTransaction();

		$epg = new EPG();

		$epg->Date = $this->BeginUpdate($sessionId);

		$epg->TuneRequestPackages = $this->GetTuneRequestPackages($sessionId, $timestamp);

		$epg->Channels = $this->GetChannels($sessionId, $timestamp);

		$epg->Programs = $this->GetPrograms($sessionId, $timestamp);

		$epg->Episodes = $this->GetEpisodes($sessionId, $timestamp);

		$epg->Movies = $this->GetMovies($sessionId, $timestamp);

		$epg->Casts = $this->GetCast($sessionId, $timestamp);

		$epg->MovieTypes = $this->GetMovieTypes($sessionId, $timestamp);

		$epg->DVBCategories = $this->GetDVBCategory($sessionId, $timestamp);

		$this->EndUpdate($sessionId);

		$this->repository->Commit();

		return $epg;
	}

	/**
	 * @param string $sessionId .
	 * @param dateTime $timestamp .
	 * @param string $region .
	 * @return EPGLocation .
	 */

	public function GetEPGLocation($sessionId, $timestamp, $region)
	{
		$loc = null;

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			if(empty($region) || !is_dir(dirname(__FILE__).'/epg/'.$region))
			{
				$region = 'hun';
			}

			$region = strtolower($region);

			$dir = 'epg/'.$region;

			$tmax = 0;

			$files = @scandir(dirname(__FILE__).'/'.$dir);

			if(!empty($files))
			foreach($files as $file)
			{
				if(preg_match('/([0-9]+)\.xml/i', $file, $matches))
				{
					$t = (int)$matches[1];

					if($t > $tmax)
					{
						$tmax = $t;
					}
				}
			}

			if($tmax > strtotime($timestamp))
			{
				$baseurl = 'http://'.$_SERVER['HTTP_HOST'].'/service/'.$dir.'/';
				$xml = $tmax.'.xml';
				$xml_bz2 = $xml.'.bz2';
				$json = $tmax.'.json';
				$json_bz2 = $json.'.bz2';

				$loc = new EPGLocation();

				$loc->Date = SoapServerEx::FixDateTime($tmax);
				$loc->Url = $baseurl.$xml;

				if(file_exists(dirname(__FILE__).'/'.$dir.'/'.$xml_bz2)) $loc->BZip2Url = $baseurl.$xml_bz2;
				if(file_exists(dirname(__FILE__).'/'.$dir.'/'.$json)) $loc->JsonUrl = $baseurl.$json;
				if(file_exists(dirname(__FILE__).'/'.$dir.'/'.$json_bz2)) $loc->JsonBZip2Url = $baseurl.$json_bz2;
			}
		}

		return $loc;
	}

	/**
	 * @return dateTime .
	 * @todo deprecated
	 */

	public function GetNow()
	{
		return SoapServerEx::FixDateTime($this->db->queryOne("SELECT NOW()"));
	}

	/**
	 * @param string $sessionId .
	 * @param string $str .
	 * @return Movie[] .
	 */

	public function SearchMovie($sessionId, $str)
	{
		$movies = array();

		$userId = $this->LookupUserId($sessionId);

//		if($this->repository->HasSubscription($userId, null))
		{
			$movies = $this->repository->SearchMovie($str);
		}

		return $movies;
	}

	/**
	 * @param string $sessionId .
	 * @return Guide .
	 */

	public function GetGuide($sessionId)
	{
		$userId = $this->CheckSession($sessionId, 'admin');

		$g = new Guide();

		$row = $this->db->queryRow('SELECT UNIX_TIMESTAMP(MIN(StartDate)), UNIX_TIMESTAMP(MAX(StopDate)) FROM Program ');

		$this->repository->IsError($row);

		$g->MinDate = SoapServerEx::FixDateTime($row[0]);
		$g->MaxDate = SoapServerEx::FixDateTime($row[1]);
		$g->Channels = array();

		$res = $this->db->query(
			'SELECT Id, Name, Url, Radio FROM Channel '.
			'WHERE HasProgram = 1 '.
			'ORDER BY 4, 2');

		$this->repository->IsError($res);

		while($row = $res->fetchRow())
		{
			$g->Channels[(int)$row[0]] = new Channel($row[0], $row[1], $row[2], null, (int)$row[3] == 1);
		}

		return $g;
	}

	/**
	 * @param string $sessionId .
	 * @param int $channelId .
	 * @param dateTime $startDate .
	 * @param dateTime $stopDate .
	 * @return GuideProgram[] .
	 */

	public function GetGuideProgram($sessionId, $channelId, $startDate, $stopDate)
	{
		$userId = $this->CheckSession($sessionId, 'admin');

		$startDate = strtotime($startDate);
		$stopDate = strtotime($stopDate);

		$gps = array();

		$res = $this->db->query(
			'SELECT '.
			' t1.Id, t1.StartDate, t1.StopDate, t1.IsRepeat, t1.IsLive, '.
			' t2.Title as EpisodeTitle, t2.Description, t2.EpisodeNumber, '.
			' t3.Rating, t3.Year, t3.EpisodeCount, '.
			' t4.NameLong, t3.Title '.
			'FROM Program t1 '.
			'JOIN Episode t2 ON t2.Id = t1.EpisodeId '.
			'JOIN Movie t3 ON t3.Id = t2.MovieId '.
			'LEFT OUTER JOIN MovieType t4 ON t4.Id = t3.MovieTypeId '.
			'WHERE t1.ChannelId = '.$this->db->quote($channelId, 'integer').' '.
			'  AND t1.StopDate > FROM_UNIXTIME('.$startDate.') '.
			'  AND t1.StartDate < FROM_UNIXTIME('.$stopDate.') '.
			'  AND t1.Deleted = 0 '.
			'ORDER BY 1 ');

		$this->repository->IsError($res);

		while($row = $res->fetchRow())
		{
			$gp = new GuideProgram();

			$gp->ProgramId = $row[0];
			$gp->StartDate = SoapServerEx::FixDateTime($row[1]);
			$gp->StopDate = SoapServerEx::FixDateTime($row[2]);
			$gp->Title = $row[12];
			$gp->EpisodeTitle = $row[5];
			$gp->Description = $row[6];
			$gp->EpisodeNumber = $row[7];
			$gp->EpisodeCount = $row[10];
			$gp->IsRepeat = $row[3];
			$gp->IsLive = $row[4];
			$gp->Rating = $row[8];
			$gp->Year = $row[9];
			$gp->MovieType = $row[11];

			$gps[] = $gp;
		}

		return $gps;
	}

	/**
	 * @param string $sessionId .
	 * @param int $channelId .
	 * @return GuideProgramRecStat[] .
	 */

	public function GetGuideProgramRecStat($sessionId, $channelId)
	{
		$userId = $this->CheckSession($sessionId, 'admin');

		$gprs = array();

		$res = $this->db->query(
			'SELECT t1.Id, t1.StartDate, t4.Title, count(t3.Id) '.
			'FROM Program t1 '.
			'JOIN Episode t2 ON t2.Id = t1.EpisodeId '.
			'JOIN Movie t4 ON t4.Id = t2.MovieId '.
			'JOIN Recording t3 ON t3.ProgramId = t1.Id '.
			'WHERE (t1.ChannelId = '.$this->db->quote($channelId, 'integer').' OR 0 = '.$this->db->quote($channelId, 'integer').') '.
			'  AND t1.StartDate > DATE_ADD(CURDATE(), INTERVAL -7 day) '.
			'GROUP BY 1, 2 '.
			'ORDER BY 4 DESC, 2 ASC, 3 ASC '.
			'LIMIT 5 ');

		$this->repository->IsError($res);

		while($row = $res->fetchRow())
		{
			$gpr = new GuideProgramRecStat();

			$gpr->ProgramId = $row[0];
			$gpr->StartDate = SoapServerEx::FixDateTime($row[1]);
			$gpr->Title = $row[2];
			$gpr->Count = $row[3];

			$gprs[] = $gpr;
		}

		return $gprs;
	}

	/**
	 * @param string $sessionId .
	 * @param int $channelId .
	 * @return GuideMovieRecStat[] .
	 */

	public function GetGuideMovieRecStat($sessionId, $channelId)
	{
		$userId = $this->CheckSession($sessionId, 'admin');

		$gmrs = array();

		$res = $this->db->query(
			'SELECT t3.Id, t3.Title, count(DISTINCT t4.UserId) '.
			'FROM Program t1 '.
			'JOIN Episode t2 ON t2.Id = t1.EpisodeId '.
			'JOIN Movie t3 ON t3.Id = t2.MovieId '.
			'JOIN Recording t4 ON t4.ProgramId = t1.Id '.
			'WHERE (t1.ChannelId = '.$this->db->quote($channelId, 'integer').' OR 0 = '.$this->db->quote($channelId, 'integer').') '.
			'  AND t1.StartDate > DATE_ADD(CURDATE(), INTERVAL -7 day) '.
			'GROUP BY 1 '.
			'ORDER BY 3 DESC, t1.StartDate ASC, 2 ASC '.
			'LIMIT 5 ');

		$this->repository->IsError($res);

		while($row = $res->fetchRow())
		{
			$gmr = new GuideMovieRecStat();

			$gmr->MovieId = $row[0];
			$gmr->Title = $row[1];
			$gmr->Count = $row[2];

			$gmrs[] = $gmr;
		}

		return $gmrs;
	}

	/**
	 * @param string $sessionId .
	 * @param TvStat[] $stat .
	 */

	public function SendTvStat($sessionId, $stat)
	{
		$userId = $this->LookupUserId($sessionId);

		if(empty($userId)) return;

		foreach($stat as $s)
		{
			$start = strtotime($s->Start);
			$stop = strtotime($s->Stop);

			if($start == $stop) continue;

			$req =
				'INSERT INTO TvStat (UserId, ChannelId, ProgramId, Start, Stop) '.
				'VALUES ('.
				$this->db->quote($userId, 'text').', '.
				(int)$s->ChannelId.', '.
				(int)$s->ProgramId.', '.
				'FROM_UNIXTIME('.$start.'), '.
				'FROM_UNIXTIME('.$stop.')) ';

			$res = $this->db->query($req);

			$this->repository->IsError($res);
		}
	}


	/**
	 * @param string $sessionId .
	 * @param dateTime $date .
	 * @return UserTvStat[] .
	 */

	public function GetTvStatByPortId($sessionId, $date)
	{
		$userId = $this->CheckSession($sessionId, array('admin', 'pie'));

		$date = strtotime($date);

		$ret = array();

		$req =
			'SELECT t4.Username, t3.PorthuId, t2.PorthuId, UNIX_TIMESTAMP(t1.Start), UNIX_TIMESTAMP(t1.Stop) '.
			'FROM TvStat t1 '.
			'JOIN Program t2 ON t2.Id = t1.ProgramId '.
			'JOIN Channel t3 ON t3.Id = t1.ChannelId '.
			'JOIN User t4 ON t4.Id = t1.UserId '.
			'WHERE TIMESTAMPDIFF(SECOND, t1.Start, t1.Stop) > 10 '.
			'AND CAST(t1.Start AS DATE) = CAST(FROM_UNIXTIME('.$date.') AS DATE) '.
			'ORDER BY t1.Start, t4.Username ';

		$res = $this->db->query($req);

		$this->repository->IsError($res);

		while($row = $res->fetchRow())
		{
			$username = $row[0];

			if(empty($ret[$username]))
			{
				$uts = new UserTvStat();

				$uts->Username = $username;
				$uts->WatchList = array();

				$ret[$username] = $uts;
			}
			else
			{
				$uts = $ret[$username];
			}

			$s = new TvStat();

			$s->ChannelId = $row[1];
			$s->ProgramId = $row[2];
			$s->Start = SoapServerEx::FixDateTime($row[3]);
			$s->Stop = SoapServerEx::FixDateTime($row[4]);

			$uts->WatchList[] = $s;
		}

		return $ret;
	}
}

?>