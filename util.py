#!/usr/bin/python

import re, os, hashlib, marshal
from urlparse import *

cache = '/var/lib/xcache/'

class XCacheURL():
	def __init__(self, url):
		self.url = url
		self.u = urlparse(url)
		self.root, self.ext = os.path.splitext(self.u.path)
		self.basename = os.path.basename(self.u.path)
		self.sha = hashlib.sha1(self.basename).hexdigest()[:7]
		self.short = cache+'/%s/'%self.sha
		self.qs = parse_qs(self.u.query)
		if len(self.qs) == 0:
			self.start = 0.0
		else:
			try:
				self.start = float(self.qs['start'][0])
			except:
				self.start = 0.0

class XCacheInfo():
	def __init__(self, short=None):
		self.short = short
		if short:
			self.load(short)
	def dump(self, short=None):
		if short is None:
			short = self.short
		marshal.dump(self.__dict__, open(short+'info', 'wb+'))
	def load(self, short=None):
		if short is None:
			short = self.short
		self.__dict__ = marshal.load(open(short+'info', 'rb'))
	def __repr__(self):
		return repr(self.__dict__)

if __name__ == '__main__':
	u = XCacheURL("/youku/dddddd/aaaa.flv?start=123dd")
	print u.start
	m = XCacheInfo()
	m.dd = 'c'
	m.cc = 'f'
	print m

