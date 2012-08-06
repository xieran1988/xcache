#!/usr/bin/python

import re, os, hashlib, marshal, time, random, sys
from urlparse import *

if __name__ == '__main__':
	r = marshal.load(open(sys.argv[1], 'rb'))
	clen = r['clen']
	m = r['cov']
	tot = 0
	lse = []
	for i in m:
		s, e = i[:2]
		tot += e - s + 1
		lse.append((s, e))
	left = clen - tot
	if left == 0:
		print 'complete'
	else:
		print 'part'
	print 'count', len(m)
	print 'clen', clen
	print 'tot', tot, 'left', left
	for t in lse:
		s, e = t
		print s, e

		

