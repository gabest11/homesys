<?php

require_once('../../class/soapserverex.php');
require_once('../../class/repository.php');
require_once('service.php');

SoapServerEx::Run('Homesys', 'Service');

?>