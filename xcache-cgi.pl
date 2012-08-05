#!/usr/bin/perl

$qry = $ENV{QUERY_STRING};
print "Content-Type: text/plain\r\n";
print "\r\n";
if ($qry eq 'listc') {
	print `xcache-listc`;
}

