#!/usr/bin/python

import os, sys, marshal
from flup.server.fcgi import *
from urlparse import *
from xcache import *
import logging 

lf = open('/l/cgi2', 'a+')

def log(s):
	lf.write(s + '\n')
	lf.flush()

def check(url, cr):
	u = XCacheURL(url)
	try:
		if 'youku' in url and 'start' in url:
			raise
		R = '/d/R.' + u.sha
		if not os.path.exists(R):
			raise
		rs = open(R, 'r').read()
		r = re.match(r'(\d+)-(\d+)/(\d+)', rs)
		s, e, clen = r.groups()
		s = int(s)
		e = int(e)
		clen = int(clen)
		if cr is None:
			s2 = 0
			e2 = clen - 1
		else:
			r = re.match(r'(\d+)-(\d+)?', cr)
			s2, e2 = r.groups()
			s2 = int(s2)
			if e2 is None:
				e2 = clen - 1
		if s <= s2 and e2 <= e and e2 >= s2:
			return 'mine', 'http://%s/xcache-d/%s/%s?rs=%d&rl=%d' % \
					(u.u.hostname, u.sha, u.basename, s2-s, e2-s2+1)
		raise
	except:
		return 'pass', 'http:/%s?%s' % (
				u.u.path, u.u.query.replace('yjwt08', 'yjwt09'))

if os.getuid() == 0:
	if len(sys.argv) == 3:
		print check(sys.argv[1], sys.argv[2])
	if len(sys.argv) == 2:
		print check(sys.argv[1], None)
else:
	def myapp(env, start_rsp):
		cr = env.get('HTTP_RANGE')
		uri = 'http://%s%s' % (env['HTTP_HOST'], env['REQUEST_URI'])
		d, r = check(uri, cr)
		log('%s %s => %s\n' % (d, uri, r))
		start_rsp('302 Found', [('Location', r)])
		return ''
	log('starts')
	WSGIServer(myapp).run()

