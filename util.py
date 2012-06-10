#!/usr/bin/python

import re, os, hashlib, marshal
from urlparse import *

cache = '/var/lib/xcache/'
cachelog = '/var/lib/xcache-log/'

class XCacheDB():
	def __init__(self, path=None, **kw):
		self._d = {}
		self._unpick = kw['unpick'] if 'unpick' in kw else []
		self._path = path
		if path and os.path.isfile(path):
			self.load(path)
		if 'loads' in kw:
			self.loads(kw['loads'])
	def attr(self, k):
		return self.__dict__ if \
				(k.startswith('_') or k in self._unpick) else self._d
	def __setattr__(self, k, v):
		self.attr(k)[k] = v
	def __getattr__(self, k):
		return self.attr(k)[k]
	def dump(self, path=None):
		marshal.dump(self._d, open(path if path else self._path, 'wb+'))
	def dumps(self):
		return marshal.dumps(self._d)
	def load(self, path=None):
		self._d = marshal.load(open(path if path else self._path, 'rb'))
	def loads(self, s):
		self._d = marshal.loads(s)
	def clear(self):
		self._d = {}
	def __str__(self):
		return repr(self._d)

class XCacheStat(XCacheDB):
	def __init__(self, path=None, **kw):
		XCacheDB.__init__(self, path, unpick=['cared'], **kw)
		self.cared = False
	def clear(self):
		XCacheDB.clear(self)
		self.cared = False
	def inc(self, k, v, **kw):
		if k not in self._d:
			self._d[k] = 0
		if type(self._d[k]) == int:
			self._d[k] += v
			if 'thresold' in kw:
				if self._d[k] > kw['thresold']:
					self.cared = True
			elif 'care' not in kw:
				self.cared = True
	def add(self, a):
		for k in a._d:
			self.inc(k, a._d[k])

class XCacheInfo(XCacheDB):
	def __init__(self, short):
		XCacheDB.__init__(self, short+'info', unpick=['fp','fph'])
		self.short = short

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

if __name__ == '__main__':
	u = XCacheURL("/youku/dddddd/aaaa.flv?start=123dd")
	print u.start
	d = XCacheDB('/tmp/cc', unpick=['k'])
	d.k = 'd'
	d.dump()
	print d

