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
	err = 'op'
	try:
		if 'youku' in url and 'start' in url:
			err += '-youku-start'
			raise
		R = '/d/R.' + u.sha
		err += '|open-file-%s' % R
		rs = open(R, 'r').read()
		err += '|matchR'
		r = re.match(r'(\d+)-(\d+)/(\d+) (\d+)', rs)
		s, e, clen, start = r.groups()
		s = int(s)
		e = int(e)
		clen = int(clen)
		start = int(start)
		return \
				'mine', \
				'http://%s/xcache-d/%s/%s?hl=%d&rs=%d&re=%d&clen=%d&y=yjwt08' % \
				(u.u.hostname, u.sha, u.basename, start, s, e, clen), \
				err
		if True or cr is None:
			err += '|cr=None'
			s2 = 0
			e2 = clen - 1
		else:
			err += '|cr=%s' % cr
			r = re.match(r'bytes=(\d+)-(\d+)?', cr)
			s2, e2 = r.groups()
			s2 = int(s2)
			if e2 is None:
				e2 = clen - 1
			else:
				e2 = int(e2)
		err += '|range-%d-%d-%d-%d' % (s, e, s2, e2)
		if s <= s2 and e2 <= e and e2 >= s2:
			return 'mine', 'http://%s/xcache-d/%s/%s?rs=%d&rl=%d&y=yjwt08' % \
					(u.u.hostname, u.sha, u.basename, s2-s+start, e2-s2+1), err
		raise
	except:
		return 'pass', 'http:/%s?%s' % (
				u.u.path, u.u.query.replace('yjwt08', 'yjwt09')), err

if os.getuid() == 0:
	if len(sys.argv) == 3:
		print check(sys.argv[1], sys.argv[2])
	if len(sys.argv) == 2:
		print check(sys.argv[1], None)
else:
	def myapp(env, start_rsp):
		cr = env.get('HTTP_RANGE')
		uri = 'http://%s%s' % (env['HTTP_HOST'], env['REQUEST_URI'])
		d, r, err = check(uri, cr)
		log('%s %s => %s %s\n' % (d, uri, r, err))
		start_rsp('302 Found', [('Location', r)])
		return ''
	log('starts')
	WSGIServer(myapp).run()

