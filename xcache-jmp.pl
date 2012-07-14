#!/usr/bin/perl

$uri = $ENV{REQUEST_URI};
$sha = `/usr/lib/xcache/urlsha.py $uri`;
chomp $sha;
if (-f "/d/CH.$sha") {
	print "Location: http://$ENV{HTTP_HOST}/xcache/CF.$sha\r\n\r\n";
} else {
	$suf = $uri =~ /\?/ ? "&y=yjwt08" : "?y=yjwt08";
	print "Location: http:/$uri$suf\r\n\r\n";
}


