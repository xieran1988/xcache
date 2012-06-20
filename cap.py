#!/usr/bin/python

import re, os, sys 
import shutil, hashlib, marshal, logging, traceback, copy
from urlparse import *
from xcache import *
from socket import *
from cStringIO import *

exts = ['.flv', '.mp4', '.mp3', '.exe', '.rar', '.zip']
#exts = ['.flv' ]
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
	m.fp = open(m.short+'file', 'wb+')
	m.fph = open(m.short+'rsp.txt', 'wb+')
	m.ack = ack
	m.clen = 0
	m.seq = []
	m.io_bytes = 0
	open(m.short+'req.txt', 'wb+').write(payload)
	stat.inc('new', 1)
#	print 'insert', m
	m.dump()
	return m

rcomp = re.compile(r'^GET (\S+) \S+\r\n')

def check_request(payload, ack):
	r = rcomp.match(payload)
	if r is None:
		return 
	url = r.groups()[0]
	#print 'GET', url
	u = XCacheURL(url)
	if u.ext not in exts:
		return 
	stat.inc('get.start0' if u.start == 0.0 else 'get.start1', 1)
	if not os.path.exists(u.short):
		os.mkdir(u.short)
		os.symlink(u.short+'/file'+u.ext, u.short+'/file')
	return new_conn(u, payload, ack)

def del_conn(m, f=None):
	m.fp.close()
	m.fph.close()
	#print 'del', m, f
	m.dump()

def seq_analyze(m, s):
	f = open(m.short+'/seq', 'w+')
	f.write('%d\n'%m.clen)
	for a,b in s:
		f.write('%d %d\n' % (a,b))
#	for i in range(0, len(s)-1):
#		if s[i][0]+s[i][1] != s[i+1][0]:
#			print 'seq', '%x'%s[i][0], s[i][1], '%x'%s[i+1][0], s[i+1][1]
	f.close()
	os.system('/root/xcache/seq \
			%s/seq %s/seq-stat %s/seq-holes.txt \
			> /dev/null' % (m.short, m.short, m.short)
			)

def check_finish(m):
#	print 'finish', m
	if m.fp.tell() >= m.clen and m.stat == 'caching':
		m.fp.truncate(m.clen)
		m.stat = 'cached'
		print 'CACHED', m, 'clen', m.clen, 'left', m.clen - m.io_bytes, m.url
		if len(m.seq) > 0:
			seq_analyze(m, m.seq)
			sys.exit(0)
		stat.inc('cached', 1, care=1)
		stat.inc('tot_clen', m.clen)
	else:
		#print 'RST', m, m.stat, m.reason if hasattr(m, 'reason') else ''
		m.stat = 'error'
		m.reason = 'got rst %d/%d' % (m.fp.tell(), m.clen)
		stat.inc('error.rst', 1)
	del_conn(m, 'fin')

def check_response(m, pos, payload):
	m.fph.write(payload)
	if pos > 8192:
		m.stat = 'error'
		m.reason = 'invalid header'
		del_conn(m, 'rsp')
		stat.inc('error.hdr', 1)
		#print 'ERROR', m
		return 
	f = StringIO(payload)
	if pos == 0:
		r = f.readline()
		m.rsp = r.strip()
		if '200' not in r:
			m.stat = 'error'
			m.reason = r
			del_conn(m, 'rsp')
			stat.inc('error.rsp', 1)
			#print 'ERROR', m
			return 
	while True:
		l = f.readline()
		if l == '\r\n' or l == '':
			break
		g = re.match(r"^Content-Length:\s*(\d+)", l)
		if g is not None:
			m.clen = int(g.groups()[0])
	if l == '\r\n' and m.clen > 0:
		m.stat = 'caching'
		m.fp.truncate(m.clen)
		m.hdrlen = pos + f.tell()
		#print 'clen', m.clen, 'pos', pos, 'tell', f.tell()
		m.fp.write(payload[f.tell():])
		m.dump()
		stat.inc('caching', 1)
		print 'CACHING', m
		return m

def process_packet(m, payload, seq):
	stat.inc('packet_bytes', len(payload))
	stat.inc('packet_nr', 1)
	if len(payload) > 0:
		pos = seq - m.ack
		if m.stat == 'waiting':
			return check_response(m, pos, payload)
		elif m.stat == 'caching' and pos > 0:
			stat.inc('io_bytes', len(payload))
			stat.inc('io_packets', 1)
			p = pos - m.hdrlen
			if 'seq' in os.environ and ( \
					m.sha == os.environ['seq'] or \
					os.environ['seq'] == 'all') \
					:
				m.seq.append((p, len(payload)))
			m.io_bytes += len(payload)
			m.fp.seek(pos - m.hdrlen)
			m.fp.write(payload)
	if stat.cared:
		print stat
		print 'io', filesize(stat.io_bytes), filesize(stat.tot_clen), stat.io_packets
		stat.cared = False
	return m
	#	sock.sendto(stat.dumps(), ('localhost', 1653))
	#	stat.clear()

