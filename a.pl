
sub aa {
	$c = @_[0];
	if ($c) {
		$a = 1;
	}
	print "$a\n" if 0;
}

$u = "http://localhost/testhaha/ff.mp4";
($a,$a,$host,@ur) = split /\//, $u;
$uur = join('/', @ur);
print `./urlsha.py $uur`;

$a = "/tmp/C.ddd";
$b = $1 if $a =~ /\w\.(.*)/;
print $b;

while (<STDIN>) {
	if ($_ eq "\r\n") {
		$rn++;
		print "rn\n";
		$start = tell STDIN if $rn == 2;
	}
	chomp;
}

print "$start\n";

