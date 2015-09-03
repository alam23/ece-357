<?PHP
	$opts = getopt("s:");
	$optd = count($opts) * 2 + 1;
	for ($i = $optd; $i < $argc; $i++) {
		$f = fopen($argv[$i], 'w');
		$c = 0;
		while ($c < $opts['s']) {
			$t = "";
			for ($j = 0; $j < 1024; $j++)
				$t .= rand(0,9);
			fwrite($f, $t);
			usleep(1);
			$c+=1024;
		}
		fclose($f);
	}
?>
