#!/usr/bin/python

import re, os, hashlib, marshal, time, random, sys, traceback
from urlparse import *

cache = '/var/lib/xcache/'
cachelog = '/var/lib/xcache-log/'

def filesize(s):
	if s > 1024*1024:
		return '%.1fM'%(s/1024./1024.)
	elif s > 1024:
		return '%.1fK'%(s/1024.)
	return '%dB'%s

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
			elif 'care' in kw:
				self.cared = True
	def add(self, a):
		for k in a._d:
			self.inc(k, a._d[k])

class XCacheInfo(XCacheDB):
	def __init__(self, short):
		XCacheDB.__init__(self, short+'/info', unpick=['fp','fph'])
		self.short = short
	def __str__(self):
		return ' '.join([self._d['ext'], self._d['sha'], self._d['stat']])

class XCacheURL():
	def __init__(self, url):
		self.url = url
		self.u = urlparse(url)
		self.root, self.ext = os.path.splitext(self.u.path)
		self.basename = os.path.basename(self.u.path)
		sha = lambda x: hashlib.sha1(x).hexdigest()[:7]
		self.sha = sha(self.basename)
		self.p1 = cache+self.sha+'/'
		self.p2 = self.p1+sha(str(random.random()))+'/'
		self.qs = parse_qs(self.u.query)
		try:
			self.start = float(self.qs['start'][0])
		except:
			self.start = 0.0

def calc_http_range(ran, clen):
	if ran is None:
		return 0, clen-1
	if not re.match(r'^(\d+)?-(\d+)?$', ran):
		return 
	s, e = ran.split('-')
	if len(s) > 0 and len(e) > 0:
		return int(s), int(e)
	elif len(s) > 0 and len(e) == 0:
		return int(s), clen-1
	elif len(s) == 0 and len(e) > 0:
		return 0, int(e)

def calc_rs(rlist, rs, re, clen):
	err = ''
	sfrom = rs
	r = []
	for t in rlist:
		s, e, path, hl = t
		if s <= sfrom and sfrom <= e and sfrom <= re:
			to = min(e, re)
			fstart = sfrom - s + hl
			flen = to - sfrom + 1
			r.append((path, fstart, flen))
			print '[%d,%d]=>[%d,%d],%s,%d,%d,' % (
					s, e, sfrom, to, path, fstart, flen)
			#print 'file=', open(path).read()[fstart:fstart+flen]
			sfrom = to + 1
	err += 'sfrom=%d,re=%d' % (sfrom, re)
	if sfrom == re + 1:
		return 'ok', r, err
	return 'fail', [], err

def path_get_rs(url):
	u = XCacheURL(url)
	path = '/d/RSM.%s' % u.sha
	if not os.path.exists(path):
		return None
	return marshal.load(open(path, 'rb'))


def jmp(path, qry, ran):
	err = 'qry=%s;' % qry
	try:
		err += 'path_get_rs(%s):' % path
		clen, m = path_get_rs(path)
		err += 'calc_http_range(%s,%d):' % (ran, clen)
		rs, re = calc_http_range(ran, clen)
		err += ';calc_rs(len(m)=%d,rs=%d,re=%d):' % (
					len(m), rs, re)
		r = calc_rs(m, rs, re, clen)
		err += '%s,%s' % (r[0], r[2])
		if r[0] == 'ok':
			logw('mine %s %s\n' % (path, err))
			return 'mine', r[1], 'bytes %d-%d/%d' % (rs, re, clen)
#	u = XCacheURL(s)
	except:
		traceback.print_exc()
		pass
	to = 'http:/%s?%s' % (path, qry.replace('yjwt08', 'yjwt09'))
	logw('pass %s %s\n' % (to, err))
	return 'pass', to, ''

	return pass_url(path, qry, err)

def testjmp():
	return 'pass', 'hahah', [
			(1, 2, '/tmp/cc', 3),
			(2, 3, '/mpt/dd', 3) ]
	
def logw(s):
	logf.write(s)
	logf.flush()

logf = open('/l/cgi2', 'a+')
logw('starts\n')

#	return 'mine', '/var/xxx'

if __name__ == '__main__':
	if len(sys.argv) > 1 and sys.argv[1] == 'testjmp':
		u = XCacheURL(sys.argv[2])
		ran = sys.argv[3]
		print 'path', u.u.path, 'query', u.u.query, 'ran', ran
		print jmp(u.u.path, u.u.query, ran)
		sys.exit(0)
	#print pass_url('/www.youku.com/hahaha', 'fuckyou&y=yjwt09')
	print calc_http_range('3-5', 12)
	print calc_http_range('-3', 12)
	print calc_http_range('3-', 12)
	print 'test calc_rs'
	clen, m = marshal.load(open('/d/RSM.3da812f', 'rb'))
	print calc_rs(m, 3, 5, clen)
	print calc_rs(m, 6, 7, clen)
	print calc_rs(m, 1, 9, clen)
	print calc_rs(m, 6, 10, clen)
	print 'test jmp'
	print jmp('/www.youku.com/aaa.mp4', 'y=yjwt08', '1-9')
	print jmp('/www.youku.com/aaa.mp4', 'y=yjwt08', '0-9')
	#print jmp(sys.argv[1])

