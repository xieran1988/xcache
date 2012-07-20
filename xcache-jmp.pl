#!/usr/bin/perl

sub dumpenv {
	use Data::Dumper;
	print "Content-type: text/plain\r\n";
	print "\r\n";
	print Dumper %ENV;
	exit;
}
$uri = $ENV{REQUEST_URI};
$sha = `/usr/lib/xcache/urlsha.py $uri`;
chomp $sha;
$r = `xcahce-inrange /d/R.$sha $ENV{HTTP_RANGE}`;
if ($r) {
	print "Location: http://$ENV{HTTP_HOST}/xcache/CF.$sha?$r\r\n\r\n";
} else {
	$suf = $uri =~ /\?/ ? "&y=yjwt08" : "?y=yjwt08";
	print "Location: http:/$uri$suf\r\n\r\n";
}

