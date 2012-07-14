#!/usr/bin/perl

$uri = $ENV{REQUEST_URI};
$sha = `/usr/lib/xcache/urlsha.py $uri`;
chomp $sha;
if (-f "/tmp/CH.$sha") {
	print "Location: http://localhost/xcache/CF.$sha\r\n\r\n";
} else {
	print "Location: http:/$uri\r\n\r\n";
}


