#!/usr/bin/python

import re, os, hashlib, marshal, time, random, sys
from urlparse import *

if __name__ == '__main__':
	r = marshal.load(open(sys.argv[1], 'rb'))
	clen = r['clen']
	m = r['cov']
	tot = 0
	for i in m:
		s, e = i[:2]
		tot += e - s + 1
		print s, e
	print 'count', len(m)
	print 'clen', clen
	print 'tot', tot, 'left', clen - tot

		

