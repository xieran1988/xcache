#!/usr/bin/perl

sub ll { `echo "@_[0]" >> /l/cgi`; }
sub jmp { 
	ll "@_[0] $uri $jl $sha $ext $fname cr=$cr ut=$ut rt=$rt"; 
	print "Location: $jl\r\n\r\n"; exit; 
}
sub mine {
	$jl = "http://$ENV{HTTP_HOST}/xcache-d/$sha/$fname?$r&y=yjwt08";
	jmp "mine";
}
sub pass { $jl = "http:/$uri"; $jl =~ s/yjwt08/yjwt09/; jmp "pass"; }

$uri = $ENV{REQUEST_URI};
chomp(($sha, $ext, $fname) = split / /, `xcache-urlinfo $uri`);
chomp($ut = `xcache-urltest $uri`);
pass if $ut !~ /^ok/;
$cr = $ENV{HTTP_RANGE};
chomp($rt = `xcache-rangetest-jmp /d/R.$sha $cr`);
pass if !$rt;
mine;

