#!/usr/bin/perl

use File::Temp qw/ :mktemp  /;

sub reap {
	open F, "find /tmp/ -name 'C.*'|";
	$tot = 0;
	$cached = 0;
	while (<F>) {
		chomp;
		$tot++;
		$f = $_;
		($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
			$atime,$mtime,$ctime,$blksize,$blocks)
		= stat($f);
		$du = $blocks*$blksize;
		if ($du > $size) {
			open H, "head -c 8192 $f|";
			$rn = 0;
			$start = 0;
			$clen = 0;
			$inv = 0;
			$sohu = 0;
			while (<H>) {
				if ($_ eq "\r\n") {
					$rn++;
					$start = tell H if $rn == 2;
				}
				chomp;
				$clen = int($1) if /Content-Length:(.*)/;
				$host = $1 if /Host: *(\S+)/;
				$url = $1 if /GET (\S+)/;
				$inv++ if /^Range/;
				$sohu++ if /sohu/;
			}
			if (!$inv && $sohu && $clen && $start && $size > $clen) {
				$cached++;
				$ss = $size - $start;
				$ptn = $1 if $f =~ /\w\.(.*)/;
				$sha = `/root/xcache/urlsha.py $url`;
				chomp $sha;
				`tail -c $ss $f > /tmp/F.$ptn`;
				`head -c $start $f > /tmp/H.$ptn`;
				print "$f $clen http://$host$url $sha\n";
				if (! -f "/tmp/CH.$sha") {
					`mv /tmp/F.$ptn /tmp/CF.$sha`;
					`mv /tmp/H.$ptn /tmp/CH.$sha`;
				}
			}
		}
		`rm -rf $f`;
	}
	print "tot $tot cached $cached\n";
}

while (1) {
	sleep 1;
	reap;
}
