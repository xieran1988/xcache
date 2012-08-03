#!/usr/bin/python

import re, os, hashlib, marshal, time, random, sys, traceback
from urlparse import *

logs = []
logf = open('/l/cgi2', 'a+')

def logw(s):
	logs.append(s+'\n')

def logend():
	global logs
	logf.write(''.join(logs)+'\n')
	logf.flush()
	logs = []

class XCacheURL():
	def __init__(self, url):
		self.url = url
		self.u = urlparse(url)
		self.root, self.ext = os.path.splitext(self.u.path)
		self.basename = os.path.basename(self.u.path)
		sha = lambda x: hashlib.sha1(x).hexdigest()[:7]
		self.sha = sha(self.basename)
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
	return filter(lambda x: x[0] <= rs and re <= x[1], rlist)

def path_get_sha(path):
	u = XCacheURL(path)
	return u.sha

def sha_get_rs(sha):
	path = '/d/R.%s' % sha
	if os.path.exists(path):
		return marshal.load(open(path, 'rb'))

def jmp(path, qry, ran, sha=None):
	err = ''
	try:
		logw('path=%s qry=%s ran=%s sha=%s' % (path, qry, ran, sha))
		if sha is None:
			sha = path_get_sha(path)
		logw('sha: %s' % sha)
		m = sha_get_rs(sha)
		clen = m['clen']
		logw('clen %d len(cov) %d' % (clen, len(m['cov'])))
		rs, re = calc_http_range(ran, clen)
		r = calc_rs(m['cov'], rs, re, clen)
		if r:
			logw('mine %s %s %s' % (path, ran, sha))
			logend()
			return 'mine', \
						 ('/d/F.%s' % sha, rs, re-rs+1), \
						 'bytes %d-%d/%d' % (rs, re, clen)
	except:
		err = traceback.print_exc()
	to = 'http:/%s?%s' % (path, qry.replace('yjwt08', 'yjwt09'))
	logw('pass %s %s' % (to, err))
	logend()
	return 'pass', to

logw('==== starts ====')
logend()

if __name__ == '__main__':
	if len(sys.argv) > 1 and sys.argv[1] == 'sha':
		r = jmp('', '', None, sha=sys.argv[2])
		print r
		sys.exit(0)

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
	print jmp('/www.youku.com/aaa.mp4', 'y=yjwt08', '1-9')
	print jmp('/www.youku.com/aaa.mp4', 'y=yjwt08', '0-9')
	#print jmp(sys.argv[1])

