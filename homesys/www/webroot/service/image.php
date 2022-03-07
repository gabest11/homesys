<?php

require_once('../../nocache.php');
require_once('../../class/repository.php');

if(!empty($_GET['movieId']))
{
	$movieId = (int)$_GET['movieId'];

	$repository = new Repository();

	$db =& $repository->GetDB();

	$req = 'SELECT PictureUrl, UNIX_TIMESTAMP(DATE_ADD(PictureDate, INTERVAL 1 MONTH)), PorthuId FROM Movie WHERE Id = '.$movieId;

	$row = $db->queryRow($req);

	$repository->IsError($row);

	if(!empty($row))
	{
		if(!empty($row[0]))
		{
			$pictureUrl = $row[0];
		}
		else if(!empty($row[2]))
		{
			$pictureUrl = null;
			
			if(empty($row[1]) || $row[1] < time())
			{
				$porthuId = $row[2];
				
				$url = 'https://port.hu/document/get-movie-images?movieId=movie-'.$porthuId.'&imageId%5B%5D=all&height=310&resize=0';
				
				$s = file_get_contents($url);
				
				$imgs = json_decode($s, true);

				if(!empty($imgs[0]['url']))
				{
					$pictureUrl = (string)$imgs[0]['url'];
					
					$req =
						'UPDATE Movie '.
						'SET '.
						' PictureDate = NOW(), '.
						' PictureUrl = '.$db->quote($pictureUrl, 'text').' '.
						'WHERE Id = '.$movieId;
				}
				else
				{
					$req =
						'UPDATE Movie '.
						'SET PictureDate = NOW() '.
						'WHERE Id = '.$movieId;
				}

				$res = $db->query($req);
/*				
				$url = 'http://www.port.hu/pls/po/portal.out_main?i_area=1&i_link_l2=1&i_where=323&i_p4='.$porthuId;
				
				$xml = @simplexml_load_string(file_get_contents($url));

				$pictureUrl = $xml->film->pic->pic_link;

				if(!empty($pictureUrl))
				{
					$req =
						'UPDATE Movie '.
						'SET '.
						' PictureDate = NOW(), '.
						' PictureUrl = '.$db->quote($pictureUrl, 'text').' '.
						'WHERE Id = '.$movieId;
				}
				else
				{
					$req =
						'UPDATE Movie '.
						'SET PictureDate = NOW() '.
						'WHERE Id = '.$movieId;
				}

				$res = $db->query($req);
*/				
			}
		}

		if(!empty($pictureUrl))
		{
			header('Location: '.$pictureUrl);

			exit;
		}
	}
}

header("HTTP/1.0 404 Not Found");

?>