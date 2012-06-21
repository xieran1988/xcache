#!/usr/bin/python

import re, os, sys 
import shutil, hashlib, marshal, logging, traceback, copy
import datetime, random
from urlparse import *
from xcache import *
from socket import *
from cStringIO import *

exts = ['.flv', '.mp4', '.mp3', '.exe', '.rar', '.zip']
stat = XCacheStat()
sock = socket(AF_INET, SOCK_DGRAM)
conn = {}

def new_conn(u, payload, ack):
	m = XCacheInfo(u.p2)
	m.url = u.url
	m.start = u.start
	m.ext = u.ext
	m.sha = u.sha
	m.p2 = u.p2
	m.stat = 'waiting'
	m.fp = open(m.p2+'file', 'wb+')
	m.fph = open(m.p2+'rsp.txt', 'wb+')
	m.ack = ack
	m.clen = 0
	m.seq = []
	m.iolen = 0
	open(m.p2+'req.txt', 'wb+').write(payload)
	stat.inc('new', 1)
	m.dump()
	conn[m.p2] = m
	return m

def del_conn(m, f=None):
	m.fp.close()
	m.fph.close()
	m.dump()
	del conn[m.p2]

def stat_conn():
	s = XCacheStat()
	for k in conn:
		m = conn[k]
		s.inc(m.stat, 1)
	print s

rcomp = re.compile(r'^GET (\S+) \S+\r\n')

def check_request(payload, ack):
	r = rcomp.match(payload)
	if r is None:
		return 
	url = r.groups()[0]
	u = XCacheURL(url)
	if u.ext not in exts:
		return 
	stat.inc('get.start0' if u.start == 0.0 else 'get.start1', 1)
	if not os.path.exists(u.p1):
		os.mkdir(u.p1)
	if not os.path.exists(u.p2):
		os.mkdir(u.p2)
		os.symlink(u.p2+'/file', u.p2+'/file'+u.ext)
	return new_conn(u, payload, ack)

def seq_append(m, s):
	if 'seq' in os.environ and os.environ['seq'] == 'all':
		m.seq.append(s)

def seq_analyze(m, s):
	f = open(m.short+'/seq.txt', 'w+')
	f.write('%d\n'%m.clen)
	for a,b in s:
		f.write('%d %d\n' % (a,b))
	f.close()
	os.system( \
			'/root/xcache/seq \
			%s/seq.txt %s/seq-stat.txt %s/seq-holes.txt \
			> /dev/null' % (m.short, m.short, m.short) \
			)

def check_finish(m):
	if m.fp.tell() >= m.clen and m.stat == 'caching':
		m.fp.truncate(m.clen)
		m.stat = 'cached'
		print 'CACHED', m, 'clen', m.clen, 'left', m.clen - m.iolen, m.url
		if len(m.seq) > 0:
			seq_analyze(m, m.seq)
		stat.inc('cached', 1, care=1)
		stat.inc('tot_clen', m.clen)
	else:
		m.stat = 'error'
		m.reason = 'got rst %d/%d' % (m.fp.tell(), m.clen)
		stat.inc('error.rst', 1)

def check_response(m, pos, payload):
	m.fph.write(payload)
	if pos > 8192:
		m.stat = 'error'
		m.reason = 'invalid header'
		stat.inc('error.hdr', 1)
		return 
	f = StringIO(payload)
	if pos == 0:
		r = f.readline()
		m.rsp = r.strip()
		if '200' not in r:
			m.stat = 'error'
			m.reason = r
			stat.inc('error.rsp', 1)
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
		m.rsplen = len(payload)
		seq_append(m, (0, len(payload[f.tell():])))
		m.fp.write(payload[f.tell():])
		m.dump()
		stat.inc('caching', 1)
		print 'CACHING', m

def process_packet(m, payload, seq, fin):
	stat.inc('packet_bytes', len(payload))
	stat.inc('packet_nr', 1)
	pos = seq - m.ack
	if m.stat == 'waiting':
		check_response(m, pos, payload)
	if m.stat == 'caching' and pos >= m.hdrlen:
		stat.inc('io_bytes', len(payload))
		stat.inc('io_packets', 1)
		seq_append(m, (pos - m.hdrlen, len(payload)))
		m.iolen += len(payload)
		m.fp.seek(pos - m.hdrlen)
		m.fp.write(payload)
	if fin:
		check_finish(m)
	if m.stat == 'cached' or m.stat == 'error':
		del_conn(m)
		return True
	return False
	#	sock.sendto(stat.dumps(), ('localhost', 1653))
	#	stat.clear()

