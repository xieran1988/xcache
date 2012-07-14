#!/usr/bin/perl

use File::Temp qw/ :mktemp  /;

sub reap {
	open F, "find /c -name 'C.*'|";
	$tot = 0; $cached = 0;
	while (<F>) {
		chomp;
		$tot++;
		$f = $_;
		($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
			$atime,$mtime,$ctime,$blksize,$blocks) = stat($f);
		$ptn = $1 if $f =~ /\w\.(.*)/;
		$du = $blocks*$blksize;
		`echo "Size: $size" > /l/LH.$ptn`;
		`echo "Du: $du" >> /l/LH.$ptn`;
		`head -c 8192 $f >> /l/LH.$ptn`;
		if ($du > $size) {
			open H, "head -c 8192 $f|";
			$rn = 0; $start = 0; $clen = 0; $inv = 0;
			while (<H>) {
				if ($_ eq "\r\n") {
					$rn++;
					$start = tell H if $rn == 2;
				}
				chomp;
				$clen = int($1) if /Content-Length:(.*)/;
				$host = $1 if /Host: *(\S+)/;
				$url = $1 if /GET (\S+)/;
				#$inv++ if /^Range/;
			}
			if (!$inv && $clen && $start && $size > $clen) {
				$cached++;
				$ss = $size - $start;
				$sha = `/root/xcache/urlsha.py $url`;
				chomp $sha;
				`tail -c $ss $f > /c/F.$ptn`;
				`head -c $start $f > /c/H.$ptn`;
				print "$f $clen http://$host$url $sha\n";
				if (! -f "/d/CH.$sha") {
					`mv /c/F.$ptn /d/CF.$sha`;
					`mv /c/H.$ptn /d/CH.$sha`;
				} else {
					`rm -rf /c/F.$ptn /c/H.$ptn`;
				}
			}
		}
		`rm -rf $f`;
	}
	print "tot $tot cached $cached\n";
}

while (1) {
	reap;
	sleep 1;
}
