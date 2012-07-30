
while (1) {
	$a = int(`cat /proc/net/dev | grep eth0 | awk '{print \$2}'`);
	$b = ($a - $last)/1024./1024;
	printf "%.2fMb/s\n", $b*8 if $last;
	$last = $a;

	$a1 = int(`cat /proc/net/dev | grep eth0 | awk '{print \$3}'`);
	$b1 = ($a1 - $last1);
	printf "%d packets/s\n", $b1 if $last1;
	$last1 = $a1;

	sleep 1;
}

