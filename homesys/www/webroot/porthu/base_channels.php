<?php

$s = file_get_contents('http://port.hu/tvapi/init?i_page_id=1');
//$s = iconv('iso-8859-2', 'UTF-8', $s);
$o = json_decode($s);

$channels = array();

$xml = new SimpleXMLElement('<?xml version="1.0" encoding="utf-8"?><port_data></port_data>');

$xml_channels = $xml->addChild('Channels');

foreach($o->channels as $ch)
{
	$channels[] = $ch;

	$xml_channel = $xml_channels->addChild('Channel');

	$xml_channel['chid'] = $ch->id;
	$xml_channel['id'] = preg_replace('/[^0-9]*([0-9]+)/', '\\1', $ch->id);
	$xml_channel->Title = $ch->name;
	$xml_channel->Logo = $ch->logo;
}

header('Content-Type: text/xml');

echo $xml->asXML();

?>