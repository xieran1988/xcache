#!/usr/bin/perl

$a = $ARGV[0];
$b = $ARGV[1];
print `md5sum $a $b`;
$r = `cmp $a $b`;
die 'same' if !$r;
print $r;
($byte) = $r =~ /byte (\d+)/;
$byte -= 1;
print "at $byte\n";
print "orig $a\n";
print `hexdump -s $byte -n 20 -C $a`;
print "cur $b\n";
print `hexdump -s $byte -n 20 -C $b`;
