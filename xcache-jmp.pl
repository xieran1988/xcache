#!/usr/bin/perl

sub ll { `echo "@_[0]" >> /l/cgi`; }
sub jmp { ll "-> @_[0]\n"; print "Location: @_[0]\r\n\r\n"; exit; }
sub mine {
	ll "mine"; 
	jmp "http://$ENV{HTTP_HOST}/xcache-d/$sha/$fname?$r&y=yjwt08";
}
sub pass { ll "pass"; $uri =~ s/yjwt08/yjwt09/; jmp "http:/$uri"; }

$uri = $ENV{REQUEST_URI};
ll "uri: $uri";

chomp(($sha, $ext, $fname) = split / /, `xcache-urlinfo $uri`);
ll "sha:$sha crange:$ENV{HTTP_RANGE} fname:$fname";

chomp($u = `xcache-urltest $uri`);
ll "urltest: $u";
if ($u !~ /^ok/) { ll "urltest failed"; pass; }

chomp($r = `xcache-rangetest-jmp /d/R.$sha $ENV{HTTP_RANGE}`);
ll "rangetest: $r";
if (!$r) { ll "out-of-range"; pass; }

mine;

