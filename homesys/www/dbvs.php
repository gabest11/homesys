<?php

// TODO: test if this still works

require_once(dirname(__FILE__).'/class/repository.php');

$repository = new Repository();

$db = $repository->GetDB();

$repository->BeginTransaction();

$url = 'http://en.kingofsat.net/satellites.php';
$pattern = '/href="(dl.php\\?[^"]+)"/i';

if(preg_match_all($pattern, file_get_contents($url), $matches))
{
	$urls = $matches[1];
	
	foreach($urls as $url)
	{
		$url = 'http://en.kingofsat.net/'.$url;
		
		echo $url."\r\n";
		
		$str = iconv('windows-1252', 'utf-8', file_get_contents($url));
		
		$str = str_replace("\n", '_', $str);
		$str = str_replace("\r", '', $str);
		
		$pattern = '/1=([0-9]+)_2=(.+?)_/';
		
		if(preg_match($pattern, $str, $matches))
		{
			$position = trim($matches[1]);
			$name = trim($matches[2]);
			
			if(empty($name)) continue;
			
			echo $name.' '.$position."\r\n";
			
			$req = 'SELECT Id FROM TuneRequestPackage WHERE Type = \'DVB-S\' AND Name = '.$db->quote($name, 'text');

			$res = $db->queryOne($req);
			
			$repository->IsError($res);
			
			$pattern = '/[0-9]+=([0-9]+),(H|V),([0-9]+),([0-9]+)(,([^,]+?)(,([^,]+?)))?_/i';
			
			if(preg_match_all($pattern, $str, $matches))
			{
				if(!empty($res))
				{
					$req = 'DELETE FROM TuneRequestPackage WHERE Id = '.(int)$res;
					$res = $db->query($req);
					$repository->IsError($res);
					
					$req = 'DELETE FROM TuneRequest WHERE PackageId = '.(int)$res;
					$res = $db->query($req);
					$repository->IsError($res);
				}
	
				$req = 'INSERT INTO TuneRequestPackage (Type, Name) VALUES (\'DVB-S\', '.$db->quote($name, 'text').') ';

				echo $req."\r\n";
				
				$res = $db->query($req);

				$repository->IsError($res);
				
				$packageId = $db->lastInsertId();
				
				for($i = 0, $j = count($matches[0]); $i < $j; $i++)
				{
					$frequency = (int)$matches[1][$i] * 1000;
					$polarisation = 'LINEAR_'.$matches[2][$i];
					$symbolrate = (int)$matches[3][$i];
					$ifecrate = $matches[4][$i];
					$type = $matches[6][$i];
					$modulation = $matches[8][$i];

					if($type == 'S2') $type = 'DVB-S2';

					if(empty($modulation))
					{
						if($type == 'DVB-S') $modulation = 'QPSK';
						else if($type == 'DVB-S2') $modulation = '8PSK';
					}
					
					if($type == 'DVB-S2')
					{
						$modulation = 'NBC_'.$modulation;
					}
					
					$ifecrate = substr($ifecrate, 0, 1).'_'.substr($ifecrate, 1);
										
					$req = 
						'INSERT INTO TuneRequest (PackageId, Frequency, IFECRate, Modulation, SymbolRate, Polarisation, IFECMethod, OFECMethod, OFECRate, SpectralInversion, LPIFECMethod, LPIFECRate) '.
						'VALUES ('.
							$packageId.', '.
							$db->quote($frequency, 'text').', '.
							$db->quote($ifecrate, 'text').', '.
							$db->quote($modulation, 'text').', '.
							$db->quote($symbolrate, 'text').', '.
							$db->quote($polarisation, 'text').', '.
							'-1, -1, -1, -1, -1, -1)';
							
					echo $req."\r\n";
							
					$res = $db->query($req);
					
					$repository->IsError($res);
				}
			}
		}
	}
	
	{
		$req = 'DELETE FROM TuneRequestPackage WHERE Type = \'DVB-S\' AND Provider = \'UPC Direct\'';
	
		$res = $db->queryOne($req);
		
		$repository->IsError($res);
		
		$req = 'INSERT INTO TuneRequestPackage (Type, Name, Provider) VALUES (\'DVB-S\', \'Thor 6 (0.8W)\', \'UPC Direct\') ';
		
		echo $req."\r\n";
	
		$res = $db->queryOne($req);
		
		$repository->IsError($res);
	
		$packageId = $db->lastInsertId();
		
		$upcdirect = array(
			array(11727000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			array(11766000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			array(11804000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			array(11843000, '3_4', 'NBC_8PSK', 30000, 'LINEAR_V'),
			array(11862000, '7_8', 'QPSK', 28000, 'LINEAR_H'),
			array(11938000, '7_8', 'QPSK', 28000, 'LINEAR_H'),
			array(11996000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			array(12034000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			array(12073000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			array(12188000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			array(12265000, '7_8', 'QPSK', 28000, 'LINEAR_V'),
			);
		
		foreach($upcdirect as $tr)
		{
			$req = 
				'INSERT INTO TuneRequest (PackageId, Frequency, IFECRate, Modulation, SymbolRate, Polarisation, IFECMethod, OFECMethod, OFECRate, SpectralInversion, LPIFECMethod, LPIFECRate) '.
				'VALUES ('.
					$packageId.', '.
					$db->quote($tr[0], 'text').', '.
					$db->quote($tr[1], 'text').', '.
					$db->quote($tr[2], 'text').', '.
					$db->quote($tr[3], 'text').', '.
					$db->quote($tr[4], 'text').', '.
					'-1, -1, -1, -1, -1, -1)';
					
			echo $req."\r\n";
					
			$res = $db->query($req);
			
			$repository->IsError($res);
		}
	}
}

$repository->Commit();

?>