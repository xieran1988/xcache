#!/usr/bin/perl

use FCGI;
use IPC::Run qw( start pump finish timeout );
my @cat = qw( xcache-proc-jmp );
my $request = FCGI::Request();
start \@cat, \$in, \$out;
while ($request->Accept() >= 0) {
	print F "$ENV{REQUEST_URI} $ENV{HTTP_HOST} $ENV{HTTP_RANGE}\n";
	print "Location: ",<F>,"\r\n\r\n";
}

