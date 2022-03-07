<?php

$svc = new SoapClient('http://'.$_SERVER['HTTP_HOST'].'/service/?WSDL');

$av = $svc->GetAppVersion($_GET['app'] ? $_GET['app'] : 'homesys', '');

header('Location: '.$av->Url);

?>