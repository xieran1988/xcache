#!/usr/bin/python

import re, os, shutil, hashlib, marshal, logging, traceback, copy
from urlparse import *
from xcache import *
from socket import *
from cStringIO import *

conn = {}
exts = ['.flv', '.mp4', '.mp3', '.exe', '.rar', '.zip']

def check_request(payload):
	r = re.match(r'^GET (\S+) \S+\r\n', payload)
	if r is None:
		return None
	url = r.groups()[0]
	u = XCacheURL(url)
	if u.ext not in exts:
		return None
	print 'GET', url
	rm = XCacheInfo()
	rm.url = url
	rm.start = u.start
	rm.path = u.basename
	rm.short = u.short
	rm.ext = u.ext
	rm.sha = u.sha
	if not os.path.exists(u.short):
		return rm
	m = XCacheInfo(u.short)
	if m.stat == 'cached' and u.start < m.start:
		print 'EXISTS', m
		return rm
	if m.stat == 'error':
		print 'REWRITE', m
		return rm
	return None

def del_conn(p):
	conn[p].fp.close()
	conn[p].fph.close()
	conn[p].dump()
	del conn[p]

def check_finish(p, rst=False):
	if p in conn:
		m = conn[p]
		if m.fp.tell() >= m.clen:
			m.fp.truncate(m.clen)
			m.stat = 'cached'
			print 'CACHED', m
			del_conn(p)
		elif rst:
			m.stat = 'error'
			m.reason = 'got rst %d/%d' % (m.fp.tell(), m.clen)
			print 'RST', m
			del_conn(p)

def check_response(p, payload):
	m = conn[p]
	f = StringIO(payload)
	m.clen = 0
	while True:
		l = f.readline()
		if l == '\r\n' or l == '':
			break
		g = re.match(r"^Content-Length: (\d+)", l)
		if g is not None:
			m.clen = int(g.groups()[0])
	if l == '\r\n' and m.clen > 0:
		if not os.path.exists(m.short):
			os.mkdir(m.short)
			os.symlink('file', m.short+'file'+m.ext+'c')
		m.stat = 'caching'
		m.fp = open(m.short+'file', 'wb+')
		m.fph = open(m.short+'rsp.txt', 'wb+')
		m.fph.write(payload[:f.tell()])
		m.fp.write(payload[f.tell():])
		print 'CACHING', m
	else:
		m.stat = 'error'
		m.reason = 'invalid header'
		m.fph.write(payload)
		print 'ERROR', m
		del_conn(p)

def process_packet(srcip, dstip, srcport, dstport, seq, ack, tcpflags, payload, get, tome):
	p = (srcip, dstip, srcport, dstport)
	if get == 1 and p not in conn and not tome:
		m = check_request(payload)
		if m:
			m.ack = ack
			conn[p] = m
	p2 = (dstip, srcip, dstport, srcport)
	if p2 in conn and len(payload) > 0:
		m = conn[p2]
		pos = seq - m.ack
		if pos == 0:
			check_response(p2, payload)
		elif pos > 0:
			m.fp.seek(pos)
			m.fp.write(payload)
			check_finish(p2)
	if (p in conn or p2 in conn) and (tcpflags & 5) != 0:
		check_finish(p2, True)
		check_finish(p, True)

if __name__ == '__main__':
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 111, "GET /var/heelo.flv?start=10 HTTP2.2\r\n", 1)
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 5, "GET dd", 1)

