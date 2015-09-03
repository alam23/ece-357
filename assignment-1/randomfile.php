<?PHP
	$opts = getopt("s:");
	$optd = count($opts) * 2 + 1;
	for ($i = $optd; $i < $argc; $i++) {
		$f = fopen($argv[$i], 'w');
		$c = 0;
		while ($c < $opts['s']) {
			fwrite($f, str_repeat(rand(0,9), 1024));
		}
	}
?>
