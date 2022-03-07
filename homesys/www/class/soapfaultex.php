<?php

define('SFEX_OK', 0);
define('SFEX_ERROR', -1);
define('SFEX_INVALID_LOGIN', -2);
define('SFEX_MISSING_PARAMETER', -3);
define('SFEX_INVALID_METHOD', -4);
define('SFEX_DATABASE_ERROR', -5);
define('SFEX_INVALID_SESSIONID', -6);
define('SFEX_ACCESS_DENIED', -7);
define('SFEX_GS_SESSION_ERROR', -8);
define('SFEX_INVALID_GEOLOCATION', -9);
define('SFEX_INVALID_DEVICE', -10);
define('SFEX_INVALID_DEVICE_DESC', -11);
define('SFEX_INVALID_CLIENT_VERSION', -12);
define('SFEX_USERNAME_TOO_SHORT', -13);
define('SFEX_USERNAME_INVALID_CHARACTERS', -14);
define('SFEX_USERNAME_ALREADY_REGISTERED', -15);
define('SFEX_PASSWORD_TOO_SHORT', -16);
define('SFEX_PASSWORD_INVALID', -17);
define('SFEX_EMAIL_INVALID_CHARACTERS', -18);
define('SFEX_EMAIL_ALREADY_REGISTERED', -19);
define('SFEX_REGISTRATION_FAILED', -20);
define('SFEX_NOT_A_GUEST_ACCOUNT', -21);
define('SFEX_NO_DEVICE', -22);
define('SFEX_DEVICE_SLOTS_FULL', -23);
define('SFEX_USER_HAS_PARENT', -24);
define('SFEX_INVALID_PARAMETER', -25);
define('SFEX_USERNAME_INVALID', -26);
define('SFEX_OUT_OF_COUPON_CODES', -27);
define('SFEX_DEVICE_NOT_AUTHORIZED', -28);
define('SFEX_NO_VALID_SUBSCRIPTION', -29);
define('SFEX_LICENSE_URL_MISSING', -30);
define('SFEX_DEVICE_OFF_ERROR', -31);

$soapfaultmsgs = array(
	'eng' => array(
		SFEX_OK => 'OK',
		SFEX_ERROR => 'Error',
		SFEX_INVALID_LOGIN => 'Invalid login',
		SFEX_MISSING_PARAMETER => 'Missing parameter',
		SFEX_INVALID_METHOD => 'Invalid method',
		SFEX_DATABASE_ERROR => 'Database error',
		SFEX_INVALID_SESSIONID => 'Invalid session id',
		SFEX_ACCESS_DENIED => 'Access denied',
		SFEX_GS_SESSION_ERROR => 'Cannot start grooveshark session',
		SFEX_INVALID_GEOLOCATION => 'Access is denied from this location!',
		SFEX_INVALID_DEVICE => 'This device is not supported',
		SFEX_INVALID_DEVICE_DESC => 'Invalid device description',
		SFEX_INVALID_CLIENT_VERSION => 'This client version is no longer supported, please update',
		SFEX_USERNAME_TOO_SHORT => 'Username too short',
		SFEX_USERNAME_INVALID_CHARACTERS => 'Username contains invalid characters',
		SFEX_USERNAME_ALREADY_REGISTERED => 'Username already registered',
		SFEX_PASSWORD_TOO_SHORT => 'Password is too short',
		SFEX_PASSWORD_INVALID => 'Wrong password',
		SFEX_EMAIL_INVALID_CHARACTERS => 'Email address contiains invalid characters',
		SFEX_EMAIL_ALREADY_REGISTERED => 'Email already registered',
		SFEX_REGISTRATION_FAILED => 'Registration failed',
		SFEX_NOT_A_GUEST_ACCOUNT => 'Not a guest account',
		SFEX_NO_DEVICE => 'No device is set on this session',
		SFEX_DEVICE_SLOTS_FULL => 'It is not allowed to view protected content on more than three devices in a month, please free up another first in the settings.',
		SFEX_USER_HAS_PARENT => 'This user has a parent user already',
		SFEX_INVALID_PARAMETER => 'Invalid parameter',
		SFEX_USERNAME_INVALID => 'Invalid username',
		SFEX_OUT_OF_COUPON_CODES => 'No more coupon code available',
		SFEX_DEVICE_NOT_AUTHORIZED => 'Device not authorized to play protected content',
		SFEX_NO_VALID_SUBSCRIPTION => 'No valid subscription',
		SFEX_LICENSE_URL_MISSING => 'license_url missing',
		SFEX_DEVICE_OFF_ERROR => 'You can only watch protected content on three devices per month, if you want to disable one device, please request it in email (support@fuso.tv).',
		),
	'hun' => array(
		SFEX_OK => 'OK',
		SFEX_ERROR => 'Hiba',
		SFEX_INVALID_LOGIN => 'Hibás bejelentkezési adatok',
		SFEX_MISSING_PARAMETER => 'Hiányzó paraméter',
		SFEX_INVALID_METHOD => 'Nem létező hívás',
		SFEX_DATABASE_ERROR => 'Adatbázis hiba',
		SFEX_INVALID_SESSIONID => 'Érvénytelen session azonosító',
		SFEX_ACCESS_DENIED => 'Hozzáférés megtagadva',
		SFEX_GS_SESSION_ERROR => 'Grooveshark nem elérhető',
		SFEX_INVALID_GEOLOCATION => 'A szolgáltatás erről a helyről nem elérhető!',
		SFEX_INVALID_DEVICE => 'Nem támogatott eszköz',
		SFEX_INVALID_DEVICE_DESC => 'Hibás eszközleíró',
		SFEX_INVALID_CLIENT_VERSION => 'Nem támogatott szoftver verzió, frissítés szükséges',
		SFEX_USERNAME_TOO_SHORT => 'Felhasználói név túl rövid',
		SFEX_USERNAME_INVALID_CHARACTERS => 'Felhasználói név nem megengedett karaktereket tartalmaz',
		SFEX_USERNAME_ALREADY_REGISTERED => 'Felhasználói név már létezik',
		SFEX_PASSWORD_TOO_SHORT => 'Jelszó túl rövid',
		SFEX_PASSWORD_INVALID => 'Nem megfelelő jelszó',
		SFEX_EMAIL_INVALID_CHARACTERS => 'Az email cím nem megengedett karaktereket tartalmaz',
		SFEX_EMAIL_ALREADY_REGISTERED => 'Ezzel az email címmmel már regisztráltak korábban',
		SFEX_REGISTRATION_FAILED => 'Hiba történt a regisztráció során',
		SFEX_NOT_A_GUEST_ACCOUNT => 'Nem vendég felhasználó',
		SFEX_NO_DEVICE => 'Nince eszköz megadva',
		SFEX_DEVICE_SLOTS_FULL => 'Nincs lehetőség havonta háromnál több eszközön védett tartalmat nézni, próbáljon meg felszabadítani egy másik regisztrált gépet a beállításokban.',
		SFEX_USER_HAS_PARENT => 'Ennek a felhasználónak már van megadva szülő-felahsználója',
		SFEX_INVALID_PARAMETER => 'Hibás paraméter',
		SFEX_USERNAME_INVALID => 'Érdvénytelen felhasználói név',
		SFEX_OUT_OF_COUPON_CODES => 'Nincs több kupon kód',
		SFEX_DEVICE_NOT_AUTHORIZED => 'Védett tartalom lejátszása nem engedélyezett ezen az eszközön',
		SFEX_NO_VALID_SUBSCRIPTION => 'Nincs érvényes előfizetés',
		SFEX_LICENSE_URL_MISSING => 'license_url hiányzik',
		SFEX_DEVICE_OFF_ERROR => 'Havonta három eszközön nézhet védett tartalmat, ha elérte a limitet és ki szeretne kapcsolni egyet, írjon a support@fuso.tv címre.',
		),
	'__old' => array(
		SFEX_INVALID_SESSIONID => 'Invalid sessionId',
		SFEX_ACCESS_DENIED => 'Access denied',
		SFEX_GS_SESSION_ERROR => 'Cannot start grooveshark session',
		SFEX_INVALID_GEOLOCATION => 'Invalid geolocation',
		SFEX_INVALID_DEVICE => 'Invalid device',
		SFEX_INVALID_DEVICE_DESC => 'Invalid device description',
		SFEX_INVALID_CLIENT_VERSION => 'Invalid version',
		SFEX_USERNAME_TOO_SHORT => 'Username too short, min 4 char',
		SFEX_USERNAME_INVALID_CHARACTERS => 'Username invalid',
		SFEX_USERNAME_ALREADY_REGISTERED => 'Username already registered',
		SFEX_PASSWORD_TOO_SHORT => 'Password too short, min 4 char',
		SFEX_PASSWORD_INVALID => 'Invalid password',
		SFEX_EMAIL_INVALID_CHARACTERS => 'Email address invalid',
		SFEX_EMAIL_ALREADY_REGISTERED => 'Email already registered',
		SFEX_REGISTRATION_FAILED => 'Failed to register user',
		SFEX_NOT_A_GUEST_ACCOUNT => 'Not a guest account',
		SFEX_NO_DEVICE => 'No device',
		SFEX_DEVICE_SLOTS_FULL => 'Device slots are full',
		SFEX_USER_HAS_PARENT => 'Has parent user',
		SFEX_INVALID_PARAMETER => 'Invalid parameter',
		SFEX_USERNAME_INVALID => 'Invalid username',
		SFEX_OUT_OF_COUPON_CODES => 'No more coupon code available',
		SFEX_DEVICE_NOT_AUTHORIZED => 'Device not authorized',
		SFEX_NO_VALID_SUBSCRIPTION => 'Invalid subscription',
		SFEX_LICENSE_URL_MISSING => 'license_url missing',
		),
	);

class SoapFaultEx extends SoapFault
{
	public $code;
	public $errmsg;

	public function __construct($code, $extra, $userlang = 'eng')
	{
		$this->code = $code;
		$this->errmsg = SoapFaultEx::GetErrorMessage($code, $extra, $userlang);

		$errmsg = SoapFaultEx::GetOldErrorMessage($code, $extra);

		if(empty($errmsg)) $errmsg = $this->errmsg;

		parent::__construct('Server', $errmsg);
	}

	public static function GetErrorMessage($code, $extra, $userlang = 'eng')
	{
		global $soapfaultmsgs;

		$errmsg = null;

		foreach(array($userlang, 'eng') as $lang)
		{
			if(isset($soapfaultmsgs[$lang]))
			{
				if(isset($soapfaultmsgs[$lang][(int)$code]))
				{
					$errmsg = $soapfaultmsgs[$lang][(int)$code];
                
					break;
				}
			}
		}

		if(empty($errmsg))
		{
			foreach(array($userlang, 'eng') as $lang)
			{
				if(isset($soapfaultmsgs[$lang]))
				{
					if(isset($soapfaultmsgs[$lang][-1]))
					{
						$errmsg = $soapfaultmsgs[$lang][-1];

						break;
					}
				}
			}
		}

		if(empty($errmsg)) 
		{
			$errmsg = $soapfaultmsgs['eng'][-1];
		}

		if(!empty($extra))
		{
			$errmsg .= ' ('.$extra.')';
		}

		return $errmsg;
	}

	public static function GetOldErrorMessage($code, $extra)
	{
		global $soapfaultmsgs;

		$errmsg = null;

		if(isset($soapfaultmsgs['__old'][$code]))
		{
			$errmsg = $soapfaultmsgs['__old'][$code];

			if(!empty($extra))
			{
				$errmsg .= ' ('.$extra.')';
			}
		}

		return $errmsg;
	}
}
/*
if(0)
{
	foreach($soapfaultmsgs as $lang => $msgs)
	{
		echo $lang."\r\n";

		foreach($msgs as $code => $msg)
		{
			try
			{
				throw new SoapFaultEx($code, 'extra', $lang);
			}
			catch(Exception $ex)
			{
				echo $ex->code.' '.$ex->errmsg."\r\n";
				if($ex->errmsg != $ex->getMessage()) echo "\t".$ex->getMessage()."\r\n";
			}
		}
	}
}
*/
?>