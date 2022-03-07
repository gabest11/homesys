<?php

$ini = ini_set("soap.wsdl_cache_enabled", 0);

require_once(dirname(__FILE__).'/../lib/wsdl-writer/classes/WsdlDefinition.php');
require_once(dirname(__FILE__).'/../lib/wsdl-writer/classes/WsdlWriter.php');
require_once(dirname(__FILE__).'/soapfaultex.php');

class SoapServerEx extends SoapServer
{
	var $name;
	var $class;

	public function __construct($name, $class)
	{
		parent::__construct(SoapServerEx::CreateWSDL($name, $class));

		$this->name = $name;
		$this->class = $class;

		$input = file_get_contents('php://input');

		if(strpos($input, '<tns:ListMachines />') !== false) exit;
		if(strpos($input, '<tns:GetStations />') !== false) exit;
		if(strpos($input, '2010-04-27T04:21:24</timestamp></tns:GetPrograms>') !== false) exit;

		//parent::__construct('http://'.$_SERVER['HTTP_HOST'].$_SERVER['PHP_SELF'].'?WSDL');

		$this->setClass($class);
	}

	public function SendResponse()
	{
		set_time_limit(600);

		$input = file_get_contents('php://input');

		//header('Connection: close'); // IIS6 needs this

$log = false;
//$log = $_SERVER['REMOTE_ADDR'] == '92.249.157.49';//strpos($input, 'InitiateSession') === false && strpos($input, 'ListRecordings') === false;
//$log = strpos($input, 'VideoFeedView') !== false;
//

if($log)
{
	$fn = 'c:/WebSites/temp/soap/'.time().' - '.$_SERVER['REMOTE_ADDR'].' - '.rand();
	file_put_contents($fn, $this->name."\r\n".$this->class."\r\n".$input);
	$start = time();

	ob_start();
}

		$this->handle();

if($log)
{
	$diff = time() - $start;
	if(1/*$diff >= 10*/) @rename($fn, $fn.'_'.$diff);
	else @unlink($fn);
	$s = ob_get_clean();
	file_put_contents($fn.'_out.txt', $s);
	echo $s;
}

	}

	public function GetResponse()
	{
		ob_start();

		$this->SendResponse();

		$str = ob_get_clean();

		// $str = str_replace('<SOAP-ENV:Envelope ', '<SOAP-ENV:Envelope xmlns:mst="http://microsoft.com/wsdl/types/" ', $str);
		// $str = str_replace('xsd:guid', 'mst:guid', $str);
		// $str = preg_replace_callback('/"xsd:dateTime">(.+?)</i', array('SoapServerEx', 'FixDateTimeRegex'), $str);

		header('Content-Length: '.strlen($str));

		return $str;
	}

	public static function CreateWSDL($name, $class)
	{
		$wsdl = dirname(__FILE__).'/../wsdl/'.$name.'_'.strtolower($class).'.wsdl';

		if(file_exists($wsdl))
		{
			if(filemtime($wsdl) < filemtime(strtolower($class).'.php'))
			{
				unlink($wsdl);
			}
		}

		if(!file_exists($wsdl))
		{
			$def = new WsdlDefinition();

			$def->setDefinitionName($name);
			$def->setClassFileName(strtolower($class).'.php');
			$def->setNameSpace('http://homesyslite.timbuktu.hu/');
			$def->setEndPoint('[ENDPOINT]');

			$writer = new WsdlWriter($def);

			$str = $writer->classToWsdl();
			$str = str_replace('xsd:guid', 'mst:guid', $str);

			file_put_contents($wsdl, $str);
		}

		return $wsdl;
	}

	public static function GetWSDL($name, $class)
	{
		$wsdl = SoapServerEx::CreateWSDL($name, $class);

		$str = file_get_contents($wsdl);

		$str = str_replace('[ENDPOINT]', 'http://'.$_SERVER['HTTP_HOST'].$_SERVER['PHP_SELF'], $str);

		return $str;
	}

	private static function GetVarTypeFromComment($comment)
	{
		if(!empty($comment))
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
		if(!empty($comment))
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
								SoapServerEx::GetTypesFromComment($comment, $types);
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

		if(!empty($comment))
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
							throw new SoapFaultEx(SFEX_MISSING_PARAMETER, $param->name);
						}
					}

					$svc = new Service();

					$logdir = 'C:/WebSites/temp/soap/';

					if(!empty($svc->_logdir)) {$logdir .= $svc->_logdir.'/'; @mkdir($logdir);}

					$today = date('YmdH');

					$soaplog = isset($_REQUEST['soaplog']) || !empty($svc->_log)/*
						|| strcasecmp($_REQUEST['method'], 'IsRegisteredVodDevice') === 0
						|| strcasecmp($_REQUEST['method'], 'RegisterVodDevice') === 0
						|| strcasecmp($_REQUEST['method'], 'FeedViewInProgress') === 0
						|| strcasecmp($_REQUEST['method'], 'FeedViewEnd') === 0;*/;

					if($soaplog) @mkdir($logdir.$today);
					
					$fn = $logdir.$today.'/'.time().'_'.rand().'_'.$_REQUEST['method'].'.txt';

					if($soaplog)
					{
						$log = array(
							gethostbyaddr($_SERVER['REMOTE_ADDR']),
							$_SERVER['REMOTE_ADDR'],
							$_REQUEST['method'],
							$params,
							'GET' => $_GET,
							'POST' => $_POST);

						@file_put_contents($fn, print_r($log, true));
					}

					$res = $method->invokeArgs($svc, $params);

					if(isset($_REQUEST['__DOTNET__']))
					{
						$tmp = new stdClass();
						$tmp->Obj = $res;
						$res = $tmp;

						walk_recursive_remove_null($res);
					}

					if(is_object($res)) $res->_exectime = microtime(true) - $start;

					$json = empty($svc->returnAsIs) ? json_encode($res) : $res;

					if(empty($svc->returnAsIs) && $json === false)
					{
						// throw new SoapFault('Server', 'json_encode error: '.json_last_error_msg());
					}

					if($soaplog || empty($svc->returnAsIs) && $json === false) @file_put_contents($fn, print_r(array('print_r' => $res, 'json' => $json), true), FILE_APPEND | LOCK_EX);
				}
				else
				{
					throw new SoapFaultEx(SFEX_INVALID_METHOD, $_REQUEST['method']);
				}
			}
			catch(Exception $e)
			{
				if(!empty($logdir)) file_put_contents($logdir.time().'_'.rand(), print_r(array($_REQUEST, $_SERVER['REMOTE_ADDR'], $e), true));

				if(isset($_REQUEST['__DOTNET__']))
				{
					$res = new stdClass();
					$res->Error = $e->getMessage();

					if($e instanceof SoapFaultEx)
					{
						$res->Error = $e->errmsg;
						$res->ErrorCode = $e->code;
					}

					$json = json_encode($res);
				}
				else
				{
					header($_SERVER['SERVER_PROTOCOL'].' 500 Internal Server Error', true, 500);

					die($e->getMessage());
				}
			}

			if(strpos($_SERVER['HTTP_ACCEPT'], 'application/json+bzip') !== false)
			{
				header('Content-Type: application/json+bzip');

				$json = @bzcompress($json, 9);
			}
			else 
			{
				header('Content-Type: application/json');

				if(strpos($_SERVER['HTTP_ACCEPT_ENCODING'], 'bzip2') !== false)
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
						SoapServerEx::GetTypesFromComment($comment, $types);

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

						if(!empty($comment)) $type = SoapServerEx::GetVarTypeFromComment($comment);

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

			$tpl = file_get_contents(dirname(__FILE__).'/soapserverex.php.tpl');
			$tpl = str_replace('%URL%', 'http://'.$_SERVER['HTTP_HOST'].$_SERVER['PHP_SELF'], $tpl);
			$tpl = str_replace('%SERVICENAME%', $_GET['name'], $tpl);
			$tpl = str_replace('%CLIENTNAME%', isset($_GET['client']) ? $_GET['client'] : 'unknown', $tpl);
			$tpl = str_replace('%MEMBERS%', '', $tpl); // TODO

			echo $tpl;
		}
		else if(isset($_GET['CS']))
		{
			header('Content-Type: text/plain');

			$tpl = file_get_contents(dirname(__FILE__).'/soapserverex.cs.tpl');
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

					SoapServerEx::GetTypesFromComment($comment, $types);

					$params = array();
					$params2 = array();

					$return = SoapServerEx::GetParamsFromComment($comment, $params);
					$return = SoapServerEx::GetCSType($return);

					$funct = 'public Task<'.$return.'> '.$method->name.'Async(';

					if(!empty($params)) 
					{
						if($method->name != 'SessionExists') 
						{
							unset($params['sessionId']);
						}

						foreach($params as $name => $type) 
						{
							$params2[$name] = SoapServerEx::GetCSType($type).' '.$name;
						}

						if(!empty($params2))
						{
							$funct .= implode(', ', $params2).', ';
						}
					}

					$funct .= 'CancellationToken? ct = null)';

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
					$defs[] = "\t}, ct);";
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
				$decl[] = 'public partial class '.$type;
				$decl[] = '{';

				$rctype = new ReflectionClass($type);

				$props = $rctype->getProperties(ReflectionProperty::IS_PUBLIC);

				foreach($props as $prop)
				{
					$comment = $prop->getDocComment();

					$type = !empty($comment) ? SoapServerEx::GetVarTypeFromComment($comment) : 'object';
					$type = SoapServerEx::GetCSType($type);

					$decl[] = "\t[DataMember(EmitDefaultValue = false)]";
					$decl[] = "\tpublic $type ".$prop->name." { get; set; }";
					$decl[] = '';
				}

				array_pop($decl);

				$decl[] = '}';
				$decl[] = '';
			}

			$tpl = str_replace('%DECL%', "\t".implode("\r\n\t", $decl), $tpl);

			echo $tpl;
		}
		else if(isset($_GET['WSDL']))
		{
			header('Content-Type: text/xml');

			echo SoapServerEx::GetWSDL($name, $class);
		}
		else
		{
			$soapserver = new SoapServerEx($name, $class);

			$bzip2 = !empty($_SERVER['HTTP_ACCEPT_ENCODING']) && strpos($_SERVER['HTTP_ACCEPT_ENCODING'], 'bzip2') !== false;

			if($bzip2)
			{
				ob_start();
			}

			$soapserver->SendResponse();

			if($bzip2)
			{
				$bz = @bzcompress(ob_get_clean(), 9);

				header('Content-Encoding: bzip2');
				header('Content-Length: '.strlen($bz));

				echo $bz;
			}
		}

		exit;
	}

	public static function FixDateTime($str)
	{
		if(is_numeric($str) || preg_match('/^[0-9]+$/', $str))
		{
			$timestamp = $str;
		}
		else if(preg_match('/([0-9]+)-([0-9]+)-([0-9]+) ([0-9]+):([0-9]+):([0-9]+)/', $str, $m))
		{
			$timestamp = mktime($m[4], $m[5], $m[6], $m[2], $m[3], $m[1]);
		}
		else
		{
			$timestamp = strtotime($str);
		}

		if(isset($_REQUEST['__DOTNET__']))
		{
			if($str == null) return null;

			return '/Date('.$timestamp.'000)/';
		}

		return date('c', $timestamp);
	}

	public static function FixDateTimeRegex($matches)
	{
		return '"xsd:dateTime">'.SoapServerEx::FixDateTime($matches[1]).'<';
	}
}

?>