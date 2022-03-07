<?php

require_once('Mail.php');
require_once(dirname(__FILE__).'/config.php');

function mail2($to, $subject, $body)
{
	global $g_config;
	
	$headers = array ('From' => $g_config['mail']['from'],
		'To' => $to,
		'Subject' => '=?UTF-8?B?'.base64_encode($subject).'?=',
		'Content-Type' => 'text/plain; charset=utf-8');

	$smtp = Mail::factory('smtp', $g_config['mail']['smtp']);

	$mail = $smtp->send($to, $headers, $body);

	return !PEAR::isError($mail);
}

function mail2_admins($subject, $body)
{
	global $g_config;
	
	foreach($g_config['mail']['admins'] as $to)
	{
		mail2($to, $subject, $body);
	}
}

function xml_dump_fixobject(&$object)
{
	if(is_array($object))
	{
		foreach($object as $key => $value)
		{
			xml_dump_fixobject($object[$key]);
		}
	}
	else if(is_object($object))
	{
		foreach($object as $key => $value)
		{
			if(is_array($value) && !empty($value))
			{
				$items = array_slice($value, 0, 1);

				$class = get_class($items[0]);

				if($class != $key)
				{
					$object->$key = new stdClass();

					$object->$key->$class = $value;

					xml_dump_fixobject($object->$key->$class);
				}
			}
		}

		foreach($object as $key => $value)
		{
			if(is_object($value) && !empty($value))
			{
				xml_dump_fixobject($object->$key);
			}
			else if(is_bool($value))
			{
				$object->$key = $value ? 'true' : 'false';
			}
		}
	}
}

require_once('XML/Serializer.php');

function xml_dump($object, $return = false)
{
	xml_dump_fixobject($object);

	// TODO: simplexml

	$serializer_options = array (
	   'addDecl' => TRUE,
	   'encoding' => 'UTF-8',
	   //'indent' => ' ', 'linebreak' => "\r\n",
	   'indent' => '', 'linebreak' => '',
	   'rootName' => 'EPG',
	   'mode' => 'simplexml',
	);

	$serializer = new XML_Serializer($serializer_options);

	$status = $serializer->serialize($object);

	if(PEAR::isError($status))
	{
		die($status->getMessage());
	}

	$xml = $serializer->getSerializedData();

	if($return)
	{
		return $xml;
	}

	echo $xml;
}
/*
function url_exists($url)
{
	if($fp = @fopen($url, 'rb'))
	{
		fclose($fp);

		return true;
	}

	return false;
}
*/
function url_exists($url)
{
    $hdrs = @get_headers($url);

    return is_array($hdrs) ? preg_match('/^HTTP\\/\\d+\\.\\d+\\s+2\\d\\d\\s+.*$/', $hdrs[0]) : false;
}

function serial_bit_extract($bin, $i, $n)
{
	if($n > 8) die('bit_extract: cannot extract more than 8 bits');

	$byte = $i >> 3;
	$bit = $i & 7;

	if($byte >= count($bin)) die('bit_extract: invalid bit position');

	$ret = $bin[$byte] >> $bit;

	if($bit != 0 && $bit + $n > 8)
	{
		$ret |= ($bin[$byte + 1] & ((1 << $bit) - 1)) << (8 - $bit);
	}

	$ret &= (1 << $n) - 1;

	return $ret;
}

function serial_gen_from_md5($md5)
{
	$hash = pack("H*" , $md5);

	$bin = array();

	for($i = 0; $i < strlen($hash); $i++)
	{
		$bin[] = ord($hash{$i});
	}

	$map = array();

	for($i = '0'; ord($i) <= ord('9'); $i = chr(ord($i) + 1)) $map[] = $i;
	for($i = 'A'; ord($i) <= ord('V'); $i = chr(ord($i) + 1)) $map[] = $i;

	$key = '';

	$chksum = 0;

	for($i = 0, $j = 0; $i < 5 * 15; $i += 5, $j++)
	{
		$index = serial_bit_extract($bin, $i, 5);

		$chksum += $index;

		$key .= $map[$index];

		if($j != 0 && (($j + 1) % 4) == 0) $key .= '-';
	}

	$key .= $map[$chksum & 31];

	return $key;
}

function file_get_contents_gzip($url, $useragent = null, $referer = null)
{
	$ch = curl_init($url);

	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($ch, CURLOPT_ENCODING, 'gzip');
	//curl_setopt($ch, CURLOPT_USERAGENT, 'Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36');

	if(isset($useragent))
	{
		if(is_numeric($useragent))
		switch($useragent)
		{
		default: 
		case 1: // chrome
			$useragent = 'Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36'; 
			break;
		}

		curl_setopt($ch, CURLOPT_USERAGENT, $useragent);
	}

	if(!empty($referer))
	{
		curl_setopt($ch, CURLOPT_REFERER, $referer);
	}
	
	$data = curl_exec($ch);

	curl_close($ch);

	return $data;
}

function file_get_contents_wget($url)
{
	return shell_exec('wget --no-check-certificate -O - "'.$url.'" 2> NUL');
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
		}
	}
}

if(isset($argc))
{
require_once(dirname(__FILE__).'/class/repository.php');

if($argc == 5 && $argv[1] == 'register')
{
	$r = new Repository();

	$id = $r->Register($argv[2], $argv[3], $argv[4]);

	echo $id != null ? $id : 'Failed!';

	exit;
}
else if($argc == 2 && $argv[1] == 'fixepnum')
{
	$r = new Repository();

	$db = $r->GetDB();

	$res = $db->queryAll(
		'SELECT t1.Id, t1.Description, t1.MovieId, t1.EpisodeNumber, t2.EpisodeCount '.
		'FROM Episode t1 '.
		'JOIN Movie t2 ON t2.Id = t1.MovieId '.
		'WHERE t1.Description LIKE \'%. rész%\' AND t1.EpisodeNumber = 0 ');

	$r->IsError($res);

	foreach($res as $row)
	{
		$id = $row[0];
		$description = $row[1];
		$movieId = $row[2];
		$episodeNumber = $row[3];
		$episodeCount = $row[4];

		echo $description."\r\n";

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

		echo '('.$episodeNumber.'/'.$episodeCount.') '.$description."\r\n\r\n";

		if($episodeNumber > 0)
		{
			$res2 = $db->query(
				'UPDATE Episode '.
				'SET EpisodeNumber = '.$db->quote($episodeNumber, 'integer').', '.
				'    Description = '.$db->quote($description, 'text').' '.
				'WHERE Id = '.$db->quote($id, 'integer'));
				
			$r->IsError($res2);
		}

		if($episodeCount > 0)
		{
			$res2 = $db->query(
				'UPDATE Movie '.
				'SET EpisodeCount = '.$db->quote($episodeCount, 'integer').' '.
				'WHERE Id = '.$db->quote($movieId, 'integer'));

			$r->IsError($res2);
		}
	}
	
	exit;
}

}

?>