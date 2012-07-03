#!/usr/bin/perl

use Time::HiRes qw ( time );

my %hc;

sub bit { @_[0]*8./1024/1024 }

while (<>) {
	chomp;
	($c, @a) = split / /;
	if ($c eq 's_cached') {
		$hc{$a[2]}++;
	}
	if ($c eq 's_iostat') {
		($t, $tot, $valid, $io, $ht, $io0) = @a;
		if ($lasttm) {
			$e = $t - $lasttm;
			printf "{%.2lf %.2lf %.2lf %d %d %d %d %d}\n",
				$e, bit($tot/$e), bit($valid/$e), bit($io/$e),
				$hc{complete}, $hc{rst}, $hc{hole}, $ht, $io0
				;
			%hc = ();
		}
		$lasttm = $t;
	}
}

