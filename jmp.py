#!/usr/bin/python

import os, sys, marshal
from flup.server.fcgi import *
from urlparse import *
from xcache import *
import logging 

if __name__ == '__main__':
	def find(url):
		u = XCacheURL(url)
#		logging.info('url:'+url)
		if os.path.exists(u.short):
			m = XCacheInfo(u.short)
#			logging.info(repr(m))
#			logging.info(u.start)
			if m.stat == 'cached':
				if m.ext in ['.mp4', '.flv']:
					if u.start == m.start:
						return '/xcache/%s/file'%m.sha
					if m.start < u.start:
						return '/xcache/%s/file%sc?start=%f'%(m.sha, m.ext, u.start-m.start)
				else:
					return '/xcache/%s/file'%m.sha
		return None

	def app(env, start_rsp):
		u = urlparse(env['REQUEST_URI'])
#		logging.info('uri:'+env['REQUEST_URI'])
		# http://myip/jmp/oldhost/xxxxx
		a = u.path.split('/')
		old = '/'.join(a[2:]) + '?' + u.query
		s = find(old)
		start_rsp('302 Found', [('Location', 'http://' + (env['HTTP_HOST']+s if s else old))])
		return ''

#	logging.basicConfig(filename='/tmp/jmp', level=logging.INFO)
	if 'PHP_FCGI_CHILDREN' not in os.environ:
		print find('http://localhost/www.youku.com/03000801004F374476C9E6006CBD7CB892776A-3E41-5A90-3A4D-030192C8F269.mp4?start=200')
	else:
	#	logging.info('fastcgi')
		WSGIServer(app).run()

