#!/usr/bin/perl

$m = int($ARGV[0]);
$now = time;
%h = ();
while (<STDIN>) {
	chomp;
	($t) = /(\d+)/;
	$i = $now - $t;
	if ($i < $m*60) {
		$h{$t} = $_;
#		print "$m $t $i $_";
	}
}

@sh = sort { $a <=> $b } keys %h;
for $i (@sh) {
	print "$t ", `cat $h{$i}`;
}

