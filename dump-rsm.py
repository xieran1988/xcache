#!/usr/bin/python

import re, os, hashlib, marshal, time, random, sys
from urlparse import *

if __name__ == '__main__':
	clen, m = marshal.load(open(sys.argv[1], 'rb'))
	print 'clen', clen
	print 'count', len(m)
	for i in m:
		s, e = i[:2]
		print s, e
	print 'covers'
	i = 0
	while i < len(m):
		s, e = m[i][:2]
		j = i+1
		while j < len(m) and m[j][0] <= e+1 and e+1 <= m[j][1]:
			e = m[j][1]
			j += 1
		i = j
		print s, e

		

