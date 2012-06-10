#!/usr/bin/python

import os, shutil, marshal, re, sys, select
from socket import *
from optparse import OptionParser
from xcache import *


if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option("-p", "--path", dest="p",
												help="process cache entry in PATH", metavar="PATH")
	(opt, args) = parser.parse_args()

	sock = socket(AF_INET, SOCK_DGRAM)
	sock.bind(('localhost', 1653))
	sock2 = socket(AF_INET, SOCK_DGRAM)
	sock2.bind(('localhost', 1654))

	print 'started'

	path = cachelog + 'cur'
	while True:
		r, w, x = select.select([sock, sock2], [], [])
		if sock in r:
			t = XCacheStat(path)
			b = sock.recvfrom(65536)[0]
			s = XCacheStat(loads=b)
			t.add(s)
			t.dump()
			print s
	
