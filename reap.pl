#!/usr/bin/perl

use File::Temp qw/ :mktemp  /;

sub reap {
	open F, "find -L /c -name 'C.*'|";
	$tot = 0; $cached = 0;
	while (<F>) {
		chomp;
		$tot++;
		$f = $_;
		($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
			$atime,$mtime,$ctime,$blksize,$blocks) = stat($f);
		$ptn = $1 if $f =~ /\w\.(.*)/;
		$du = $blocks*$blksize;
		$t = time;
		if ($du > $size) {
			$s = `head -c 4096 $f`;
			$start = index($s, "\r\n\r\n");
			($url) = $s =~ /GET (\S+)/;
			($clen) = $s =~ /Content-Length: (\d+)/;
			($host) = $s =~ /Host: (\S+)/;
			($range) = $s =~ /Range: bytes=(.*)/;
			if ($clen > 1.5*1000000 && $start > 0 && $size > $clen) {
				$cached++;
				$filesize = $size - $start;
				($sha, $ext) = split /\s+/, `/usr/lib/xcache/urlsha.py $url`;
				$mb = 1024.*1024;
				printf "%.2lf $ext http://$host%s\n", $filesize/$mb, $url;
				if (!`xcache-inrange /d/R.$sha $range`) {
					`tail -c $filesize $f > /d/CF.$sha.$ptn`;
					`mv /d/CF.$sha.$ptn /d/CF.$sha`;
					`head -c $start $f > /d/CH.$sha`;
					`echo $clen $range > /d/R.$sha`;
					`echo "Overwrite: $range" >> /l/L`;
				}
				`head -c $start $f >> /l/L`;
				`echo "Done: $sha $ext $t" >> /l/L`;
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
