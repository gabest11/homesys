<?php   

function findheaderin($class, $dir)
{
	$file = $dir.'/'.$class.'.h';
	$pattern = '/class +'.$class."(\t|\r|\n| )/i";

	if(file_exists($file))
	{
		if(preg_match($pattern, file_get_contents($file)))
		{
    			return $file;
		}
	}

	if($dh = opendir($dir))
	{
		while(($file = readdir($dh)) !== false)
		{
			if(preg_match('/\\.h$/i', $file))
			{
				if(preg_match($pattern, file_get_contents($dir.'/'.$file)))
				{
					return $dir.'/'.$file;
				}
			}
		}

		closedir($dh);
	}

	return null;
}

function findheader($class)
{
	foreach(array('.', '..', '../../dxui') as $dir)
	{
		if($file = findheaderin($class, $dir))
		{
			return $file;
		}
	}

	return null;
}

function attr($node, $varname)
{
	if(!empty($varname)) $varname .= '.';
	else $varname = '';

	$anchor = false;

	$rows = array();

	foreach($node->attributes() as $key => $value)
	{
		if($key == 'Name' || $key == 'Rect')
		{
			continue;
		}

		if(preg_match('/^ *([0-9]+) *$/', $value, $m))
		{
			$value = (int)$m[1];
		}
		else if(preg_match('/^ *0x([0-9A-Za-z]+) *$/', $value, $m))
		{
			$value = '0x'.$m[1];
		}
		else if(preg_match('/^ *([0-9]+) *, *([0-9]+) *, *([0-9]+) *, *([0-9]+) *$/', $value, $m))
		{
			$value = 'Vector4i('.$m[1].', '.$m[2].', '.$m[3].', '.$m[4].')';
		}
		else if(preg_match('/^ *([0-9]+) *, *([0-9]+) *$/', $value, $m) && strpos($key, 'Size') !== false)
		{
			$value = 'Vector2i('.$m[1].', '.$m[2].')';
		}
		else if(preg_match('/^ *([0-9]+) *, *([0-9]+) *$/', $value, $m) && strpos($key, 'Point') !== false)
		{
			$value = 'Vector2i('.$m[1].', '.$m[2].')';
		}
		else if(preg_match('/^s\\|(.*)$/', $value, $m))
		{
			if(preg_match('/^[A-Z][A-Z0-9_]*$/', $m[1])) 
			{
				throw new Exception('This string looks like a localized string id ('.$value.')', 0);
			}
			
			$value = 'L"'.$m[1].'"';
		}
		else if(preg_match('/^sl\\|(.*)$/', $value, $m))
		{
			if(!preg_match('/^[A-Z][A-Z0-9_]*$/', $m[1])) 
			{
				throw new Exception('Localized string id must contain only the letters of [A-Z0-9_] ('.$value.')', 0);
			}
			
			$value = 'Control::GetString("'.$m[1].'")';
		}
		else
		{
			$value = $value;
		}

		if(array_search($key, array('Left', 'Top', 'Right', 'Bottom', 'Size', 'Width', 'Height', 'Rect')) !== false)
		{
			$anchor = true;
		}
		else if($key == 'Anchor')
		{
			$anchor = false;
		}

		if(strpos($key, 'Color') !== false && !preg_match('/^[0-9]/', $value[0]))
		{
			$value = 'Color::'.trim($value);
		}
		else if(strpos($key, 'TextFace') !== false && strpos($value, 'L"') === false)
		{
			$value = 'FontType::'.trim($value);
		}
		else if($key == 'BackgroundStyle')
		{
			$value = 'BackgroundStyleType::'.trim($value);
		}
		else if($key == 'TextAlign')
		{
			$a = explode(',', $value);
			$value = array();
			foreach($a as $s) $value[] = 'Align::'.trim($s);
			$value = implode('|', $value);
		}
		else if($key == 'Anchor' || $key == 'AnchorRect')
		{
			$value = trim($value);
			if($value == 'Fill') $value = 'Left, Top, Right, Bottom';
			if($value == 'Left') $value = 'Left, Top, Left, Bottom';
			if($value == 'Right') $value = 'Right, Top, Right, Bottom';
			if($value == 'Top') $value = 'Left, Top, Right, Top';
			if($value == 'Bottom') $value = 'Left, Bottom, Right, Bottom';
			$a = explode(',', $value);
			if(count($a) == 2) $a = array_merge($a, $a);
			$value = array();
			foreach($a as $s) $value[] = 'Anchor::'.trim($s);
			$value = 'Vector4i('.implode(', ', $value).')';
      
			$key = 'AnchorRect';
		}
		else if(array_search($key, array('AnchorLeft', 'AnchorTop', 'AnchorRight', 'AnchorBottom')) !== false)
		{
			$value = 'Anchor::'.trim($value);

			$anchor = false;
		}

		$rows[] = sprintf("%s%s = %s;", $varname, $key, $value);
	}

	if($anchor)
	{
		$rows[] = sprintf("%sAnchorRect = %sAnchorRect;", $varname, $varname);
	}

	return $rows;
}

function create($node, $parent, &$varcount, &$varnames, &$include)
{
	$rows = array();

	if($parent == 'this')
	{
		foreach(attr($node, null) as $row)
		{
			array_push($rows, $row);
		}

		$rows[] = "";
	}

	foreach($node->children() as $key => $value)
	{
		if($key == 'Property' || $key == 'Event')
		{
			continue;
		}

		// inc

		$header = $key.'.h';

		if(!isset($include[$header]))
		{
			$header = findheader($key);

			if(!file_exists($header))
			{
				throw new Exception('Definition of '.$key.' is missing', 0);
			}
		}

		$include[$header] = true;

		// def

		$class = $value->getName();
		$name = (string)$value['Name'];

		if(empty($name))
		{
			if(empty($varcount[$class])) $varcount[$class] = 0;
			$prefix = strtolower($class);
			do {$name = sprintf("m_%s%d", $prefix, ++$varcount[$class]);}
			while(isset($varnames[$name]));
		}

		if(isset($varnames[$name]))
		{
			throw new Exception("Definition '$name' is a dup", 1);
		}

		$varnames[$name] = $class;

		// create

		$left = $top = $right = $bottom = null;

		if(!empty($value['Rect']))
		{
			$rect = explode(',', $value['Rect']);

			if(count($rect) != 4) throw new Exception('Value of Rect is invalid "'.$value['Rect'].'"', 2);

			list($left, $top, $right, $bottom) = $rect;
		}
		else
		{
			if(!empty($value['Left']) && !empty($value['Right']))
			{
				$left = $value['Left'];
				$right = $value['Right'];
			}
			else if(!empty($value['Left']) && !empty($value['Width']))
			{
				$left = $value['Left'];
				$right = '('.$value['Left'].') + ('.$value['Width'].')';
			}
			else if(!empty($value['Right']) && !empty($value['Width']))
			{
				$left = '('.$value['Right'].') - ('.$value['Width'].')';
				$right = $value['Right'];
			}
			else if(!empty($value['Right']))
			{
				$left = 0;
				$right = $value['Right'];
			}
			else if(!empty($value['Width']))
			{
				$left = 0;
				$right = $value['Width'];
			}

			if(!empty($value['Top']) && !empty($value['Bottom']))
			{
				$top = $value['Top'];
				$bottom = $value['Bottom'];
			}
			else if(!empty($value['Top']) && !empty($value['Height']))
			{
				$top = $value['Top'];
				$bottom = '('.$value['Top'].') + ('.$value['Height'].')';
			}
			else if(!empty($value['Bottom']) && !empty($value['Height']))
			{
				$top = '('.$value['Bottom'].') - ('.$value['Height'].')';
				$bottom = $value['Bottom'];
			}
			else if(!empty($value['Bottom']))
			{
				$top = 0;
				$bottom = $value['Bottom'];
			}
			else if(!empty($value['Height']))
			{
				$top = 0;
				$bottom = $value['Height'];
			}
		}

		// TODO: Size?

		if($left === null || $top === null)
		{
			$left = $top = $right = $bottom = 0;
		}

		$rows[] = sprintf("%s.Create(Vector4i(%s, %s, %s, %s), %s);",
			$name,
			trim($left), trim($top), trim($right), trim($bottom),
			$parent);

		foreach(attr($value, $name) as $row)
		{
			array_push($rows, $row);
		}

		$rows[] = "";

		foreach(create($value, '&'.$name, $varcount, $varnames, $include, null, null) as $row)
		{
			array_push($rows, $row);
		}
	}

	return $rows;
}

function compile($node, $dir, $name, $class)
{
	$cpp = $dir.'/'.$name.'.cpp';
	$header = $dir.'/'.$name.'.h';

	$base = $node->getName();

	$varcount = array();
	$varnames = array();

	$include = array(findheader($base) => true);

	$rows = create($node, 'this', $varcount, $varnames, $include);
/*
	// .cpp

	$fp = fopen($cpp, 'w');

	fputs($fp, "\xEF\xBB\xBF");

	fprintf($fp, '#include "stdafx.h"'."\r\n");

	fprintf($fp, '#include "'.$name.'.h"'."\r\n");

	fprintf($fp, "namespace DXUI\r\n{\r\n");

	fprintf($fp, "\tbool %s::Create(const Vector4i& r, Control* parent)\r\n\t{\r\n", $class);

	fprintf($fp, "\t\tif(!__super::Create(r, parent))\r\n\t\t\treturn false;\r\n");

	fprintf($fp, "\r\n");

	foreach($rows as $row)
	{
		fprintf($fp, "\t\t%s\r\n", $row);
	}

	fprintf($fp, "\t\treturn true;\r\n");

	fprintf($fp, "\t}\r\n");

	fprintf($fp, "}\r\n\r\n");

	fclose($fp);
*/
	// .h

	$fp = fopen($header, 'w');

	fputs($fp, "\xEF\xBB\xBF");

	fprintf($fp, "#pragma once\r\n\r\n");

  // fprintf($fp, '#include "./Property.h"'."\r\n");

  foreach($include as $key => $value)
	{
		if(!file_exists($dir.'/'.$key)) die($dir.'/'.$key.' is missing');

		fprintf($fp, "#include \"%s\"\r\n", $key);
	}

	fprintf($fp, "\r\n");

	fprintf($fp, "namespace DXUI\r\n{\r\n\tclass %s : public %s\r\n\t{\r\n\tpublic:\r\n", $class, $base);

	foreach($varnames as $name => $class)
	{
		fprintf($fp, "\t\t%s %s;\r\n", $class, $name);
	}

	if(count($varnames) > 0) fprintf($fp, "\r\n");

	fprintf($fp, "\t\tbool Create(const Vector4i& r, Control* parent)\r\n\t\t{\r\n");

	fprintf($fp, "\t\t\tif(!__super::Create(r, parent))\r\n\t\t\t\treturn false;\r\n\r\n");

	foreach($rows as $row)
	{
		fprintf($fp, "\t\t\t%s\r\n", $row);
	}

	fprintf($fp, "\t\t\treturn true;\r\n");

	fprintf($fp, "\t\t}\r\n");

	$first = true;

	foreach($node->children() as $key => $value)
	{
		if($key == 'Property')
		{
			if($first) {fprintf($fp, "\r\n"); $first = false;}

			fprintf($fp, "\t\tProperty<%s> %s;\r\n", $value['Type'], $value['Name']);
		}
	}

	$first = true;

	foreach($node->children() as $key => $value)
	{
		if($key == 'Event')
		{
			if($first) {fprintf($fp, "\r\n"); $first = false;}

			fprintf($fp, "\t\tEvent<%s> %s;\r\n", $value['Type'], $value['Name']);
		}
	}

	fprintf($fp, "\t};\r\n");

	fprintf($fp, "}\r\n\r\n");

	fclose($fp);
}

for($i = 1; $i < count($argv); $i++)
{
	$dir = dirname($argv[$i]);
	$name = basename($argv[$i]);
	$class = basename($argv[$i], '.dxui').'DXUI';

	$header = $dir.'/'.$name.'.h';
  
	// if(!file_exists($header) || filemtime($header) < filemtime($argv[1]))
	{
		// $cwd = getcwd();

		// chdir($dir);

		try
		{
			compile(simplexml_load_file($argv[$i]), $dir, $name, $class);
		}
		catch(Exception $e)
		{
			printf("%s(%d): error E%04d: %s\r\n", $argv[$i], 1, $e->getCode(), $e->getMessage());
		}

		// chdir($cwd);
	}
}

?>