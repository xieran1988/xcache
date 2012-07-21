#!/usr/bin/perl

sub dumpenv {
	use Data::Dumper;
	print "Content-type: text/plain\r\n";
	print "\r\n";
	print Dumper %ENV;
	exit;
}
$uri = $ENV{REQUEST_URI};
($sha) = split / /, `xcache-urlinfo $uri`;
chomp $sha;
$r = `xcache-inrange /d/R.$sha $ENV{HTTP_RANGE}`;
`echo "r=$r uri=$uri sha=$sha env=$ENV{HTTP_RANGE}" >> /tmp/cgilog`;
if ($r) {
	print "Location: http://$ENV{HTTP_HOST}/xcache-d/CF.$sha?$r\r\n\r\n";
} else {
	$suf = $uri =~ /\?/ ? "&y=yjwt08" : "?y=yjwt08";
	print "Location: http:/$uri$suf\r\n\r\n";
}

