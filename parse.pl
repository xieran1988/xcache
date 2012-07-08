#!/usr/bin/perl

use Time::HiRes qw ( time );

my %hc;

sub bit { @_[0]*8./1024/1024 }
$| = 1;
while (<>) {
	chomp;
	next until ~ /^s_/;
	($c, @a) = split / /;
	if ($c eq 's_cached') {
		$hc{$a[2]}++;
		print STDERR "$_\n";
	}
	if ($c eq 's_iostat') {
		print STDERR "$_\n";
		($t, $ht, $tot, $valid, $io, $io0) = @a;
		if ($lasttm) {
			$e = $t - $lasttm;
			printf '{"tot":%.2lf, "valid":%.2lf, "io":%.2lf, '.
				'"complete":%d, "rst":%d, "hole":%d, "ht":%d, "io0":%d}'."\n",
				bit($tot/$e), bit($valid/$e), bit($io/$e), 
				$hc{complete}, $hc{rst}, $hc{hole}, $ht, $io0
				;
			%hc = ();
		}
		$lasttm = $t;
	}
}

