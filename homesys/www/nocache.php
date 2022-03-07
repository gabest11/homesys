<?php

header("Cache-Control: no-store, no-cache, must-revalidate");
header("Cache-Control: pre-check=0, post-check=0, max-age=0", false);
header("Cache-Control: private", false);
header("Pragma: no-cache");
header("Expires: " . gmdate("D, d M Y H:i:s", time()) . " GMT");
header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");

?>