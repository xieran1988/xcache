#!/usr/bin/python

import re, os, hashlib, marshal
from urlparse import *

cache = '/var/lib/xcache/'
cachelog = '/var/lib/xcache-log/'

class XCacheStat():
	def __init__(self, p=None):
		self.cared = False
		self.__dict__ = {}
		if p and os.path.exists(p):
			self.loads(open(p, 'rb'))
	def clear(self):
		self.__init__()
	def inc(self, s, i, care=1, thresold=0):
		if not hasattr(self, s):
			setattr(self, s, 0)
		setattr(self, s, getattr(self, s) + i)
		if thresold > 0:
			care = (getattr(self, s) > thresold)
		if care == 1:
			self.cared = True
	def dumps(self):
		return marshal.dumps(self.__dict__)
	def dump(self, p):
		open(p, 'wb+').write(self.dumps())
	def loads(self, s):
		self.__dict__ = marshal.loads(s)
	def add(self, a):
		for k in a.__dict__:
			self.inc(k, a.__dict__[k])

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
		d = self.__dict__.copy()
		del d['fp']
		del d['fph']
		marshal.dump(d, open(short+'info', 'wb+'))
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

