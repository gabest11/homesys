<?php

date_default_timezone_set('Europe/Budapest');

$now = !empty($_GET['now']) ? $_GET['now'] : time();
$chid = $_GET['chid'];

$s = file_get_contents(sprintf('http://port.hu/tvapi?channel_id=%s&i_datetime_from=%s&i_datetime_to=%s', $chid, date('Y-m-d', $now), date('Y-m-d', strtotime('+14 day', $now))));
//$s = iconv('iso-8859-2', 'UTF-8', $s);
$o = json_decode($s);

$xml = new SimpleXMLElement('<?xml version="1.0" encoding="utf-8"?><port_data></port_data>');

$xml_channel = $xml->addChild('Channel');

$xml_channel['Id'] = preg_replace('/[^0-9]*([0-9]+)/', '\\1', $chid);

$xml_eventlist = $xml_channel->addChild('EventList');

foreach($o as $k => $v)
{
	$from = strtotime($v->date_from);

	$xml_eventdate = $xml_eventlist->AddChild('EventDate');

	$xml_eventdate['Date'] = date('Y.m.d.', $from);

	foreach($v->channels as $ch)
	{
		//if($ch->id != $chid) continue;

		foreach($ch->programs as $p)
		{
			$start = strtotime($p->start_datetime);
			$end = strtotime($p->end_datetime);

			if(date('Y.m.d.', $start) != (string)$xml_eventdate['Date'] || empty($p->end_datetime)) continue;

			$xml_event = $xml_eventdate->addChild('Event');

			$xml_event['Id'] = preg_replace('/[^0-9]*([0-9]+)/', '\\1', $p->id);//$p->id;
			$xml_event->StartDate = date('Y-m-d', $start).'T'.date('H:i:s', $start);
			$xml_event->StopDate = date('Y-m-d', $end).'T'.date('H:i:s', $end);
			$xml_event->Title = $p->title;
			$xml_event->Episodetitle = $p->episode_title;
			$xml_event->Filmid = preg_replace('/[^0-9]*([0-9]+)/', '\\1', $p->film_id);//$p->film_id;
			//$xml_event->Episodeid = ;
			$xml_event->Isrepeat = $p->is_repeat ? 1 : 0;
			//$xml_event->Filmtypeid = ;
			//$xml_event->Filmtypenameshort = ;
			//$xml_event->Filmtypenamelong = ;
			$xml_event->Shortdescription = $p->short_description;
			$xml_event->Longdescription = $p->description;
			//$xml_event->DVBCategoryName = ;
			//$xml_event->CategoryNibbleLevel1 = ;
			$xml_event->Rating = !empty($p->restriction->age_limit) ? (int)$p->restriction->age_limit : 0;

			$xml_pictures = $xml_event->addChild('Pictures');

			if(!empty($p->media) && !empty($p->media->src))
			{
				$xml_picture = $xml_pictures->addChild('Picture');

				$xml_picture->id = 0;
				$xml_picture->url = $p->media->src;

				if(preg_match('/\\/([0-9]+)_[0-9]+\\./', $p->media->src, $m))
				{
					$xml_picture->id = $m[1];
				}
			}

			if(!empty($p->italics)) $xml_event->Shortdescription .= ' '.$p->italics;

			if(!empty($xml_event->Shortdescription))
			{
				$xml_event->Shortdescription = str_replace('<br/>', "\r\n", $xml_event->Shortdescription);
			}

			if(!empty($xml_event->Longdescription))
			{
				$xml_event->Longdescription = str_replace('<br/>', "\r\n", $xml_event->Longdescription);
			}
		}
	}
}

header('Content-Type: text/xml');

echo $xml->asXML();

?>