#!/usr/bin/perl

$a = $ARGV[0];
$b = $ARGV[1];
($byte) = `cmp $a $b` =~ /byte (\d+)/;
system("hexdump -s $byte -C $b | less");
