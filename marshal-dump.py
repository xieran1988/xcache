#!/usr/bin/python

import re, os, hashlib, marshal, time, random, sys
from urlparse import *

if __name__ == '__main__':
	m = marshal.load(open(sys.argv[1], 'rb'))
	print m

