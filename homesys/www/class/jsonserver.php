<?php

class JSONServer
{
	private static function GetVarTypeFromComment($comment)
	{
		if(empty($comment))
		{
			return null;
		}

		foreach(explode("\n", $comment) as $row)
		{
			$row = trim($row);

			$tokens = preg_split('/\\s/', $row);

			for($i = 0; $i < count($tokens); $i++)
			{
				if($tokens[$i] == '@var' && $i < count($tokens) - 1)
				{
					return $tokens[$i + 1];
				}
			}
		}

		return null;
	}

	private static function GetTypesFromComment($comment, &$types)
	{
		if(empty($comment))
		{
			return;
		}

		foreach(explode("\n", $comment) as $row)
		{
			$row = trim($row);

			$tokens = preg_split('/\\s/', $row);

			for($i = 0; $i < count($tokens); $i++)
			{
				if(($tokens[$i] == '@param' || $tokens[$i] == '@return' || $tokens[$i] == '@var') && $i < count($tokens) - 1)
				{
					$type = trim($tokens[$i + 1], '[]');

					if($type == 'string'
					|| $type == 'int'
					|| $type == 'float'
					|| $type == 'boolean'
					|| $type == 'dateTime'
					|| $type == 'base64Binary')
					{
						continue;
					}

					if(!isset($types[$type]))
					{
						$types[$type] = true;

						$rctype = new ReflectionClass($type);

						$props = $rctype->getProperties(ReflectionProperty::IS_PUBLIC);

						foreach($props as $prop)
						{
							$comment = $prop->getDocComment();

							if(!empty($comment))
							{
								JSONServer::GetTypesFromComment($comment, $types);
							}
						}
					}
				}
			}
		}
	}

	private static function GetParamsFromComment($comment, &$params)
	{
		$ret = 'object';
		$params = array();

		if(empty($comment))
		{
			return $ret;
		}

		foreach(explode("\n", $comment) as $row)
		{
			$row = trim($row);

			$tokens = preg_split('/\\s/', $row);

			for($i = 0; $i < count($tokens); $i++)
			{
				if(($tokens[$i] == '@return') && $i < count($tokens) - 1)
				{
					$ret = $tokens[$i + 1];
				}
				else if(($tokens[$i] == '@param') && $i < count($tokens) - 2)
				{
					$params[trim($tokens[$i + 2], '$')] = $tokens[$i + 1];
				}
			}
		}

		return $ret;
	}

	private static function GetCSType($type)
	{
		switch($type)
		{
			case 'boolean': $type = 'bool?'; break;
			case 'int': $type = 'int?'; break;
			case 'dateTime': $type = 'DateTime?'; break;
			case 'base64Binary': $type = 'byte[]'; break;
		}

		return $type;
	}

	public static function Run($name, $class)
	{
		if(isset($_REQUEST['dumpmethods']))
		{
			$rc = new ReflectionClass($class);

			foreach($rc->getMethods() as $method)
			{
				if(!empty($method) && $method->isPublic() && !$method->isStatic() && !$method->isConstructor())
				{
					echo '<p><b>'.$method->name.'</b></p>';

					echo '<p>'.$method->getDocComment().'</p>';

					echo '<p>';

					foreach($method->getParameters() as $param)
					{
						echo $param->name.'<br/>';
					}

					echo '</p>';
				}
			}
		}
		else if(!empty($_REQUEST['method']))
		{
			$start = microtime(true);

			try
			{
				$rc = new ReflectionClass($class);

				$method = $rc->hasMethod($_REQUEST['method']) ? $rc->getMethod($_REQUEST['method']) : null;

				if(!empty($method) && $method->isPublic() && !$method->isStatic() && !$method->isConstructor())
				{
					$params = array();

					foreach($method->getParameters() as $param)
					{
						if(isset($_REQUEST[$param->name]))
						{
							$value = $_REQUEST[$param->name];

							if($value == 'undefined') $value = null;
							else if($value == 'true' || $value == 'True') $value = true;
							else if($value == 'false' || $value == 'False') $value = false;
							else if($value == '' && $param->isDefaultValueAvailable() && $param->getDefaultValue() == null) $value = null; // HACKish

							$params[] = $value; // TODO: convert associative arrays to object if needed
						}
						else if($param->isDefaultValueAvailable())
						{
							$params[] = $param->getDefaultValue();
						}
						else
						{
							throw new JsonFault(JF_MISSING_PARAMETER, $param->name);
						}
					}

					$svc = new Service();

					$logdir = sys_get_temp_dir().'/';

					if(!empty($svc->_logdir)) {$logdir = strpos($svc->_logdir, ':') !== false ? $logdir = $svc->_logdir.'/' : $logdir .= $svc->_logdir.'/'; @mkdir($logdir);}

					$today = date('YmdH');

					$jsonlog = isset($_REQUEST['jsonlog']) || !empty($svc->_log);/*
						|| strcasecmp($_REQUEST['method'], 'GetPodcast') === 0
						|| strcasecmp($_REQUEST['method'], 'FeedView') === 0
						|| strcasecmp($_REQUEST['method'], 'FeedViewInProgress') === 0
						|| strcasecmp($_REQUEST['method'], 'FeedViewEnd') === 0;*/

					if($jsonlog) @mkdir($logdir.$today);
					
					$fn = $logdir.$today.'/'.time().'_'.rand().'_'.$_REQUEST['method'].'.txt';

					$log = array(
						gethostbyaddr($_SERVER['REMOTE_ADDR']),
						'ip' => $_SERVER['REMOTE_ADDR'],
						'method' => $_REQUEST['method'],
						'params' => $params,
						'GET' => $_GET,
						'POST' => $_POST,
						'COOKIE' => $_COOKIE,
						apache_request_headers());

					if($jsonlog) @file_put_contents($fn, print_r($log, true));

					$res = $method->invokeArgs($svc, $params);

					if(isset($_REQUEST['__DOTNET__']))
					{
						$tmp = new stdClass();
						$tmp->Obj = $res;
						$res = $tmp;

						walk_recursive_remove_null($res, function(&$item, $key) {return $item == null;});
					}

					if(is_object($res)) $res->_exectime = microtime(true) - $start;

					$json = empty($svc->returnAsIs) ? json_encode($res) : $res;

					if(empty($svc->returnAsIs) && $json === false)
					{
						// throw new JsonFault('Server', 'json_encode error: '.json_last_error_msg());
					}

					if($jsonlog || empty($svc->returnAsIs) && $json === false) @file_put_contents($fn, print_r(array('print_r' => $res, 'json' => $json), true), FILE_APPEND | LOCK_EX);
				}
				else
				{
					throw new JsonFault(JF_INVALID_METHOD, $_REQUEST['method']);
				}
			}
			catch(Exception $e)
			{
				if(!empty($logdir)) file_put_contents($logdir.time().'_'.rand(), print_r(array($_REQUEST, $_SERVER['REMOTE_ADDR'], $e), true));

				if(isset($_REQUEST['__DOTNET__']))
				{
					$res = new stdClass();
					$res->Error = $e->getMessage();

					if($e instanceof JsonFault)
					{
						$res->Error = $e->getMessage();
						$res->ErrorCode = $e->getCode();
					}
					else
					{
						$res->Error = $e->getMessage();
						$res->ErrorCode = JF_ERROR;
					}

					$json = json_encode($res);
				}
				else
				{
					header($_SERVER['SERVER_PROTOCOL'].' 500 Internal Server Error', true, 500);

					die($e->getMessage());
				}
			}
			
			if(!empty($_SERVER['HTTP_ACCEPT']) && strpos($_SERVER['HTTP_ACCEPT'], 'application/json+bzip') !== false)
			{
				header('Content-Type: application/json+bzip');

				$json = @bzcompress($json, 9);
			}
			else
			{
				header('Content-Type: application/json');

				if(!empty($_SERVER['HTTP_ACCEPT_ENCODING']) && strpos($_SERVER['HTTP_ACCEPT_ENCODING'], 'bzip2') !== false)
				{
					$json = @bzcompress($json, 9);

					header('Content-Encoding: bzip2');
					header('Content-Length: '.strlen($json));
				}
			}

			echo $json;
		}
		else if(isset($_GET['JavaScript']))
		{
			header('Content-Type: text/javascript');

			if($name == 'HTPC') $name = 'Homesys'; // FIXME

			echo "function $name$class()\r\n{\r\n\tthis.url = ".'"http://'.$_SERVER['HTTP_HOST'].$_SERVER['PHP_SELF'].'"'.";\r\n";

			$types = array();

			$rc = new ReflectionClass($class);

			foreach($rc->getMethods() as $method)
			{
				if(!empty($method) && $method->isPublic() && !$method->isStatic() && !$method->isConstructor())
				{
					$comment = $method->getDocComment();

					if(!empty($comment))
					{
						JSONServer::GetTypesFromComment($comment, $types);

						echo "\r\n";

						foreach(explode("\n", $comment) as $row)
						{
							echo "\t".trim($row)."\r\n";
						}
					}

					$params = array();

					foreach($method->getParameters() as $param)
					{
						$params[] = $param->name;
					}

					echo "\r\n";

					echo "\tthis.".$method->name.' = function(cb_success, cb_failure';
					if(!empty($params)) echo ', '.implode(', ', $params);
					echo ")\r\n";

					foreach($params as $i => $param)
					{
						$params[$i] = "'".$param."': ".$param;
					}

					echo "\t{\r\n";
					echo "\t\t$.ajax({\r\n";
					echo "\t\t\ttype: 'POST',\r\n";
					echo "\t\t\turl: this.url,\r\n";
					echo "\t\t\tdata: {'method': '".$method->name."'";
					if(!empty($params)) echo ", ".implode(', ', $params);
					if(!empty($_GET['profile'])) echo ", 'XDEBUG_PROFILE': 1";
					echo "},\r\n";
					echo "\t\t\tdataType: 'json',\r\n";
					echo "\t\t\tsuccess: cb_success,\r\n";
					echo "\t\t\terror: cb_failure\r\n";
					//echo "\t\t\tfailure: function() {alert('".$method->name." failed');}\r\n";
					echo "\t\t});\r\n";
					echo "\t}\r\n";

					echo "\r\n";
				}
			}

			{
				echo "\t/**\r\n";

				foreach($types as $type => $dummy)
				{
					if($type == 'string'
					|| $type == 'int'
					|| $type == 'float'
					|| $type == 'boolean'
					|| $type == 'dateTime'
					|| $type == 'base64Binary')
					{
						continue;
					}

					echo "\t* class $type\r\n\t* {\r\n";

					$rctype = new ReflectionClass($type);

					$props = $rctype->getProperties(ReflectionProperty::IS_PUBLIC);

					foreach($props as $prop)
					{
						$type = null;

						$comment = $prop->getDocComment();

						if(!empty($comment)) $type = JSONServer::GetVarTypeFromComment($comment);

						if(empty($type)) $type = 'var';

						echo "\t*\t$type \$".$prop->name.";\r\n";
					}

					echo "\t* }\r\n\t*\r\n";
				}

				echo "\t*/\r\n";
			}

			echo "}\r\n";
		}
		else if(isset($_GET['PHP']))
		{
			header('Content-Type: text/plain');

			$tpl = file_get_contents(dirname(__FILE__).'/jsonserver.php.tpl');
			$tpl = str_replace('%URL%', 'http://'.$_SERVER['HTTP_HOST'].$_SERVER['PHP_SELF'], $tpl);
			$tpl = str_replace('%SERVICENAME%', $_GET['name'], $tpl);
			$tpl = str_replace('%CLIENTNAME%', isset($_GET['client']) ? $_GET['client'] : 'unknown', $tpl);
			$tpl = str_replace('%MEMBERS%', '', $tpl); // TODO

			echo $tpl;
		}
		else if(isset($_GET['CS']))
		{
			header('Content-Type: text/plain');

			$bindable = isset($_REQUEST['BINDABLE']);

			$tpl = file_get_contents(dirname(__FILE__).'/jsonserver.cs.tpl');
			$tpl = str_replace('%URL%', 'http://'.$_SERVER['HTTP_HOST'].$_SERVER['PHP_SELF'], $tpl);

			$decl = array();
			$defs = array();

			$types = array();

			$rc = new ReflectionClass($class);

			foreach($rc->getMethods() as $method)
			{
				if(!empty($method) && $method->isPublic() && !$method->isStatic() && !$method->isConstructor())
				{
					$comment = $method->getDocComment();

					JSONServer::GetTypesFromComment($comment, $types);

					$params = array();
					$params2 = array();

					$return = JSONServer::GetParamsFromComment($comment, $params);
					$return = JSONServer::GetCSType($return);

					$funct = 'public Task<'.$return.'> '.$method->name.'Async(';

					if(!empty($params)) 
					{
						if($method->name != 'SessionExists') 
						{
							unset($params['sessionId']);
						}

						foreach($params as $name => $type) 
						{
							$params2[$name] = JSONServer::GetCSType($type).' '.$name;
						}

						if(!empty($params2))
						{
							$funct .= implode(', ', $params2).', ';
						}
					}

					$funct .= 'CancellationToken? ct = null, ServiceParameter param = null)';

					$defs[] = $funct;
					$defs[] = '{';
					$defs[] = "\treturn Call<".$return.'>(new Dictionary<string, object>()';
					$defs[] = "\t{";
					$defs[] = "\t\t{ \"method\", \"".$method->name.'" },';
					foreach($params as $name => $type) 
					{
						$type = trim($type, '[]');

						if($type == 'string'
						|| $type == 'int'
						|| $type == 'float'
						|| $type == 'boolean'
						|| $type == 'dateTime'
						|| $type == 'base64Binary')
						{
							$defs[] = "\t\t{ \"".$name."\", ".$name.' },';
						}
						else
						{
							// TODO: multi-level members

							$rctype = new ReflectionClass($type);

							$props = $rctype->getProperties(ReflectionProperty::IS_PUBLIC);

							foreach($props as $prop)
							{
								$defs[] = "\t\t{ \"".$name.'['.strtolower($prop->getName()).']", '.$name.' != null ? '.$name.'.'.$prop->getName().' : null },';
							}
						}
					}
					$defs[] = "\t}, ct, param);";
					$defs[] = '}';
					$defs[] = '';
				}
			}

			$tpl = str_replace('%DEFS%', "\t\t".implode("\r\n\t\t", $defs), $tpl);

			foreach($types as $type => $dummy)
			{
				if($type == 'string'
				|| $type == 'int'
				|| $type == 'float'
				|| $type == 'boolean'
				|| $type == 'dateTime'
				|| $type == 'base64Binary')
				{
					continue;
				}

				$decl[] = '[DataContract]';
				$decl[] = 'public partial class '.$type.($bindable ? ' : ServiceNotifyPropertyChanged' : '');
				$decl[] = '{';

				$rctype = new ReflectionClass($type);

				$props = $rctype->getProperties(ReflectionProperty::IS_PUBLIC);

				foreach($props as $prop)
				{
					$comment = $prop->getDocComment();

					$type = !empty($comment) ? JSONServer::GetVarTypeFromComment($comment) : 'object';
					$type = JSONServer::GetCSType($type);

					if($bindable)
					{
						if($bindable) $type = preg_replace('/(.+)\\[\\]$/', 'ObservableCollection<${1}>', $type);

						$decl[] = "\t[DataMember(EmitDefaultValue = false, Name=\"".$prop->name."\")]";
						$decl[] = "\tprivate $type _".$prop->name.";";
						$decl[] = '';
						$decl[] = "\tpublic $type ".$prop->name;
						$decl[] = "\t{";
						$decl[] = "\t\tget { return _".$prop->name."; }";
						$decl[] = "\t\tset { SetProperty(ref _".$prop->name.", value); }";
						$decl[] = "\t}";
					}
					else
					{
						$decl[] = "\t[DataMember(EmitDefaultValue = false)]";
						$decl[] = "\tpublic $type ".$prop->name." { get; set; }";
					}

					$decl[] = '';
				}

				array_pop($decl);

				$decl[] = '}';
				$decl[] = '';
			}

			$tpl = str_replace('%DECL%', "\t".implode("\r\n\t", $decl), $tpl);

			echo $tpl;
		}

		exit;
	}
}

class JSONFault extends Exception
{
	public function __construct($code = 0, $extra, $userlang = 'eng', Exception $previous = null)
	{
		$message = JSONFault::GetErrorMessage($code, $extra, $userlang);

		parent::__construct($message, $code, $previous);
	}

	public static function GetErrorMessage($code, $extra, $userlang = 'eng')
	{
		global $jsonfaultmsgs;

		$message = null;

		foreach(array($userlang, 'eng') as $lang)
		{
			if(isset($jsonfaultmsgs[$lang]))
			{
				if(isset($jsonfaultmsgs[$lang][(int)$code]))
				{
					$message = $jsonfaultmsgs[$lang][(int)$code];
                
					break;
				}
			}
		}

		if(empty($message))
		{
			foreach(array($userlang, 'eng') as $lang)
			{
				if(isset($jsonfaultmsgs[$lang]))
				{
					if(isset($jsonfaultmsgs[$lang][-1]))
					{
						$message = $jsonfaultmsgs[$lang][-1];

						break;
					}
				}
			}
		}

		if(empty($message)) 
		{
			$message = $jsonfaultmsgs['eng'][-1];
		}

		if(!empty($extra))
		{
			$message .= ' ('.$extra.')';
		}

		return $message;
	}
}

function walk_recursive_remove_null($array)
{
	if(is_array($array) || is_object($array))
	{
		foreach($array as $k => $v)
		{
			if(is_array($v))
			{
				walk_recursive_remove_null($v);
			}
			else if(is_object($v))
			{
				walk_recursive_remove_null($v);
			}
			else if($v === null)
			{
				if(is_array($array)) unset($array[$k]);
				else if(is_object($array)) unset($array->{$k});
	        	}
			else if(is_string($v) && empty($v))
			{
				unset($array->{$k});
			}
		}
	}
}

define('JF_OK', 0);
define('JF_ERROR', -1);
define('JF_INVALID_LOGIN', -2);
define('JF_MISSING_PARAMETER', -3);
define('JF_INVALID_METHOD', -4);
define('JF_DATABASE_ERROR', -5);
define('JF_INVALID_SESSIONID', -6);
define('JF_ACCESS_DENIED', -7);
define('JF_GS_SESSION_ERROR', -8);
define('JF_INVALID_GEOLOCATION', -9);
define('JF_INVALID_DEVICE', -10);
define('JF_INVALID_DEVICE_DESC', -11);
define('JF_INVALID_CLIENT_VERSION', -12);
define('JF_USERNAME_TOO_SHORT', -13);
define('JF_USERNAME_INVALID_CHARACTERS', -14);
define('JF_USERNAME_ALREADY_REGISTERED', -15);
define('JF_PASSWORD_TOO_SHORT', -16);
define('JF_PASSWORD_INVALID', -17);
define('JF_EMAIL_INVALID_CHARACTERS', -18);
define('JF_EMAIL_ALREADY_REGISTERED', -19);
define('JF_REGISTRATION_FAILED', -20);
define('JF_NOT_A_GUEST_ACCOUNT', -21);
define('JF_NO_DEVICE', -22);
define('JF_DEVICE_SLOTS_FULL', -23);
define('JF_USER_HAS_PARENT', -24);
define('JF_INVALID_PARAMETER', -25);
define('JF_USERNAME_INVALID', -26);
define('JF_OUT_OF_COUPON_CODES', -27);
define('JF_DEVICE_NOT_AUTHORIZED', -28);
define('JF_NO_VALID_SUBSCRIPTION', -29);
define('JF_LICENSE_URL_MISSING', -30);
define('JF_DEVICE_OFF_ERROR', -31);
define('JF_MEDIA_NOT_FOUND', -32);

$jsonfaultmsgs = array(
	'eng' => array(
		JF_OK => 'OK',
		JF_ERROR => 'Error',
		JF_INVALID_LOGIN => 'Invalid login',
		JF_MISSING_PARAMETER => 'Missing parameter',
		JF_INVALID_METHOD => 'Invalid method',
		JF_DATABASE_ERROR => 'Database error',
		JF_INVALID_SESSIONID => 'Invalid session id',
		JF_ACCESS_DENIED => 'Access denied',
		JF_GS_SESSION_ERROR => 'Cannot start grooveshark session',
		JF_INVALID_GEOLOCATION => 'Access is denied from this location!',
		JF_INVALID_DEVICE => 'This device is not supported',
		JF_INVALID_DEVICE_DESC => 'Invalid device description',
		JF_INVALID_CLIENT_VERSION => 'This client version is no longer supported, please update',
		JF_USERNAME_TOO_SHORT => 'Username too short',
		JF_USERNAME_INVALID_CHARACTERS => 'Username contains invalid characters',
		JF_USERNAME_ALREADY_REGISTERED => 'Username already registered',
		JF_PASSWORD_TOO_SHORT => 'Password is too short',
		JF_PASSWORD_INVALID => 'Wrong password',
		JF_EMAIL_INVALID_CHARACTERS => 'Email address contiains invalid characters',
		JF_EMAIL_ALREADY_REGISTERED => 'Email already registered',
		JF_REGISTRATION_FAILED => 'Registration failed',
		JF_NOT_A_GUEST_ACCOUNT => 'Not a guest account',
		JF_NO_DEVICE => 'No device is set on this session',
		JF_DEVICE_SLOTS_FULL => 'It is not allowed to view protected content on more than three devices in a month, please free up another first in the settings.',
		JF_USER_HAS_PARENT => 'This user has a parent user already',
		JF_INVALID_PARAMETER => 'Invalid parameter',
		JF_USERNAME_INVALID => 'Invalid username',
		JF_OUT_OF_COUPON_CODES => 'No more coupon code available',
		JF_DEVICE_NOT_AUTHORIZED => 'Device not authorized to play protected content',
		JF_NO_VALID_SUBSCRIPTION => 'No valid subscription',
		JF_LICENSE_URL_MISSING => 'license_url missing',
		JF_DEVICE_OFF_ERROR => 'You can only watch protected content on three devices per month, if you want to disable one device, please request it in email (support@fuso.tv).',
		JF_MEDIA_NOT_FOUND => 'Media/trailer id not found',
		),
	'hun' => array(
		JF_OK => 'OK',
		JF_ERROR => 'Hiba',
		JF_INVALID_LOGIN => 'Hibás bejelentkezési adatok',
		JF_MISSING_PARAMETER => 'Hiányzó paraméter',
		JF_INVALID_METHOD => 'Nem létező hívás',
		JF_DATABASE_ERROR => 'Adatbázis hiba',
		JF_INVALID_SESSIONID => 'Érvénytelen session azonosító',
		JF_ACCESS_DENIED => 'Hozzáférés megtagadva',
		JF_GS_SESSION_ERROR => 'Grooveshark nem elérhető',
		JF_INVALID_GEOLOCATION => 'A szolgáltatás erről a helyről nem elérhető!',
		JF_INVALID_DEVICE => 'Nem támogatott eszköz',
		JF_INVALID_DEVICE_DESC => 'Hibás eszközleíró',
		JF_INVALID_CLIENT_VERSION => 'Nem támogatott szoftver verzió, frissítés szükséges',
		JF_USERNAME_TOO_SHORT => 'Felhasználói név túl rövid',
		JF_USERNAME_INVALID_CHARACTERS => 'Felhasználói név nem megengedett karaktereket tartalmaz',
		JF_USERNAME_ALREADY_REGISTERED => 'Felhasználói név már létezik',
		JF_PASSWORD_TOO_SHORT => 'Jelszó túl rövid',
		JF_PASSWORD_INVALID => 'Nem megfelelő jelszó',
		JF_EMAIL_INVALID_CHARACTERS => 'Az email cím nem megengedett karaktereket tartalmaz',
		JF_EMAIL_ALREADY_REGISTERED => 'Ezzel az email címmmel már regisztráltak korábban',
		JF_REGISTRATION_FAILED => 'Hiba történt a regisztráció során',
		JF_NOT_A_GUEST_ACCOUNT => 'Nem vendég felhasználó',
		JF_NO_DEVICE => 'Nince eszköz megadva',
		JF_DEVICE_SLOTS_FULL => 'Nincs lehetőség havonta háromnál több eszközön védett tartalmat nézni, próbáljon meg felszabadítani egy másik regisztrált gépet a beállításokban.',
		JF_USER_HAS_PARENT => 'Ennek a felhasználónak már van megadva szülő-felahsználója',
		JF_INVALID_PARAMETER => 'Hibás paraméter',
		JF_USERNAME_INVALID => 'Érdvénytelen felhasználói név',
		JF_OUT_OF_COUPON_CODES => 'Nincs több kupon kód',
		JF_DEVICE_NOT_AUTHORIZED => 'Védett tartalom lejátszása nem engedélyezett ezen az eszközön',
		JF_NO_VALID_SUBSCRIPTION => 'Nincs érvényes előfizetés',
		JF_LICENSE_URL_MISSING => 'license_url hiányzik',
		JF_DEVICE_OFF_ERROR => 'Havonta három eszközön nézhet védett tartalmat, ha elérte a limitet és ki szeretne kapcsolni egyet, írjon a support@fuso.tv címre.',
		JF_MEDIA_NOT_FOUND => 'Nem található ilyen media/trailer id',
		),
	);

?>