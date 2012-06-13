#!/usr/bin/python

import re, os, sys 
import shutil, hashlib, marshal, logging, traceback, copy
from urlparse import *
from xcache import *
from socket import *
from cStringIO import *

exts = ['.flv', '.mp4', '.mp3', '.exe', '.rar', '.zip']
#exts = ['.flv', '.mp4']
stat = XCacheStat()
sock = socket(AF_INET, SOCK_DGRAM)

def new_conn(u, payload, ack):
	m = XCacheInfo(u.short)
	m.url = u.url
	m.start = u.start
	m.path = u.basename
	m.ext = u.ext
	m.sha = u.sha
	m.stat = 'waiting'
	#m.fp = open(m.short+'file', 'wb+')
	#m.fph = open(m.short+'rsp.txt', 'wb+')
	m.ack = ack
	m.clen = 0
	m.io_bytes = 0
#	m.dump()
	#open(m.short+'req.txt', 'wb+').write(payload)
	stat.inc('new', 1)
	return m

rcomp = re.compile(r'^GET (\S+) \S+\r\n')

def check_request(payload, ack):
	r = re.match(r'^GET (\S+) \S+\r\n', payload)
	if r is None:
		return 
	url = r.groups()[0]
	#print 'GET', url
	u = XCacheURL(url)
	if u.ext not in exts:
		return 
	stat.inc('get.start0' if u.start == 0.0 else 'get.start1', 1)
	if os.path.exists(u.short):
		m = XCacheInfo(u.short)
		if (m.stat == 'cached' and u.start < m.start) or \
				m.stat == 'error' or \
				m.stat == 'waiting' or \
				m.stat == 'caching':
			#print 'REWRITE', m
			stat.inc('rewrite_'+m.stat, 1)
			return new_conn(u, payload, ack)
	else:
		#print 'NEW', u.short
		#os.mkdir(u.short)
		#os.symlink('file', u.short+'file'+u.ext)
		return new_conn(u, payload, ack)

def del_conn(m):
	pass
#	conn[p].fp.close()
#	conn[p].fph.close()
	#conn[p].dump()

def check_finish(m):
	#if m.fp.tell() >= m.clen and m.stat == 'caching':
	if m.stat == 'caching':
		#m.fp.truncate(m.clen)
		m.stat = 'cached'
		print 'CACHED', m, m.clen, m.io_bytes, m.url
		stat.inc('cached', 1, care=1)
		stat.inc('tot_clen', m.clen)
		del_conn(m)
	else:
		m.stat = 'error'
		#m.reason = 'got rst %d/%d' % (m.fp.tell(), m.clen)
		m.reason = 'got rst'
		#print 'RST', m
		stat.inc('error.rst', 1)
		del_conn(m)

def check_response(m, pos, payload):
	#m.fph.write(payload)
	if pos > 8192:
		m.stat = 'error'
		m.reason = 'invalid header'
		del_conn(m)
		stat.inc('error.hdr', 1)
		#print 'ERROR', m
		return 
	f = StringIO(payload)
	if pos == 0:
		r = f.readline()
		m.rsp = r.strip()
		if '200' not in r:
			m.stat = 'error'
			m.reason = 'rsp not 200 OK'
			del_conn(m)
			stat.inc('error.rsp', 1)
			#print 'ERROR', m
			return 
	while True:
		l = f.readline()
		if l == '\r\n' or l == '':
			break
		g = re.match(r"^Content-Length: ?(\d+)", l)
		if g is not None:
			m.clen = int(g.groups()[0])
	if l == '\r\n' and m.clen > 0:
		m.stat = 'caching'
		m.hdrlen = pos + f.tell()
		#print 'clen', m.clen, 'pos', pos, 'tell', f.tell()
		#m.fp.write(payload[f.tell():])
		#m.dump()
		stat.inc('caching', 1)
		#print 'CACHING', m

def process_packet(m, payload, seq):
	stat.inc('packet_bytes', len(payload))
	stat.inc('packet_nr', 1)
	if len(payload) > 0:
		pos = seq - m.ack
		if m.stat == 'waiting':
			check_response(m, pos, payload)
		elif m.stat == 'caching' and pos > 0:
			stat.inc('io_bytes', len(payload))
			stat.inc('io_packets', 1)
			m.io_bytes += len(payload)
			#m.fp.seek(pos - m.hdrlen)
			#m.fp.write(payload)
			#check_finish(p2)
	if stat.cared:
		print 'io', filesize(stat.io_bytes), filesize(stat.tot_clen), stat.io_packets
		print 'conn', len(conn)
		stat.cared = False
	#	sock.sendto(stat.dumps(), ('localhost', 1653))
	#	stat.clear()

