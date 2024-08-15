<?php

set_time_limit(0);

ini_set('memory_limit', '1024M');

require_once(dirname(__FILE__).'/../../config.php');
require_once(dirname(__FILE__).'/../../class/repository.php');
require_once(dirname(__FILE__).'/service.php');

function GetTimestamps($epgdir, &$timestamps, &$timestampmax)
{
    $timestamps = array();
    $timestampmax = 0;

    foreach(scandir($epgdir) as $file)
    {
	if(preg_match('/([0-9]+)\.xml/i', $file, $matches))
	{
	    $timestamp = (int)$matches[1];

	    if($timestamp > 0)
	    {
		$timestamps[] = $timestamp;

		if($timestamp > $timestampmax)
		{
		    $timestampmax = $timestamp;
		}
	    }
	}
    }
}

$epgdir = dirname(__FILE__).'/epg';

if(!empty($argv[1]) && $argv[1] == 'import')
{
	$r = new PorthuRepository();

	if(!empty($argv[2]))
	{
		$targz = $argv[2];
	}
	else
	{
		// create porthu gz
	
		$baseurl = $g_config['site']['url'].'porthu/';

		$now = time();

		$tar = sprintf("epg_files_%s.tar", date('Y-m-d', $now));
		$targz = $tar.'.gz';

		@unlink($tar);
		@unlink($targz);

		$archive = new PharData($tar);

		$s = file_get_contents($baseurl.'base_channels.php?now='.$now);

		$xml = simplexml_load_string($s);

		if(empty($xml->Channels->Channel)) die('???');

		$archive->addFromString('base_channels.xml', $s);

		foreach($xml->Channels->Channel as $ch)
		{
			$chid = $ch['id'];

			echo $chid.' '.$ch->Title."\r\n";

			$s2 = file_get_contents($baseurl.'n_events.php?now='.$now.'&chid='.$ch['chid']);

			$xml2 = simplexml_load_string($s2);

			if(empty($xml2)) die('???');

			$archive->addFromString(sprintf('n_events_%d.xml', $chid), $s2);

			sleep(5); // throttle, or get empty string after a few requests
		}

		$archive->compress(Phar::GZ);

		unset($archive);

		@unlink($tar);
	}
	
	// import gz

	$r->Import($targz);

	if(empty($argv[2]))
	{
		@unlink($targz);
	}

	// generate downloadable epg file
	
	$regions = array('hun');//, 'isr', 'jey', 'deu', 'che', 'aut');

	foreach($regions as $region)
	{
		echo date('c').': '.$region."\r\n";

		$epg = $r->GetEPG($region);

		foreach($epg->Channels as $i => $c)
		{
			$epg->Channels[$i]->Logo = base64_encode($c->Logo);
		}

		$timestamps = array();
		$timestampmax = 0;

		$dir = $epgdir.'/'.$region;

		@mkdir($dir);

		GetTimestamps($dir, $timestamps, $timestampmax);

		$timestampmax = strtotime($epg->Date);

		echo date('c').': xml_dump'."\r\n";

		$xml = xml_dump($epg, true);

		$fn = $dir.'/'.$timestampmax.'.xml';
		$fn_temp = $dir.'/'.$timestampmax.'.bak';
		$fn_bz2 = $fn.'.bz2';
		$fn_temp_bz2 = $fn_temp.'.bz2';

		echo date('c').': '.$fn_temp_bz2."\r\n";

		file_put_contents($fn_temp_bz2, bzcompress($xml, 9));

		echo date('c').': '.$fn_temp."\r\n";

		file_put_contents($fn_temp, $xml);

		rename($fn_temp_bz2, $fn_bz2);
		rename($fn_temp, $fn);

		foreach($timestamps as $timestamp)
		{
			if($timestamp < $timestampmax - 2 * 24 * 3600)
			{
				@unlink($dir.'/'.$timestamp.'.xml');
				@unlink($dir.'/'.$timestamp.'.xml.bz2');
			}
		}

		echo date('c').': OK'."\r\n";
	}
}
else if(!empty($argv[1]) && $argv[1] == 'cleanup')
{
	$r = new PorthuRepository();

	$r->CleanUpGuide();
}
else
{
	// TODO: obsolite

	if(!empty($_GET['sessionId']))
	{
		//$service = new Service();

		$sessionId = $_GET['sessionId'];

		//$subscription = $service->GetSubscription($sessionId);

		//if(!empty($subscription))
		{
			$region = 'hun';

			if(!empty($_GET['region'])) $region = $_GET['region'];

			$dir = $epgdir.'/'.$region;

			if(!is_dir($dir)) $dir = $epgdir;

			$timestamps = array();
			$timestampmax = 0;

			GetTimestamps($epgdir, $timestamps, $timestampmax);

			if($timestampmax > 0)
			{
				@readfile($epgdir.'/'.$timestampmax.'.xml');
			}
		}
	}
}

?>
