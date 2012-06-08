#!/usr/bin/python

import re, os, shutil, hashlib, marshal, logging, traceback, copy
from urlparse import *
from xcache import *
from socket import *
from cStringIO import *

conn = {}
exts = ['.flv', '.mp4', '.mp3', '.exe', '.rar', '.zip']
stat = XCacheStat()
sock = socket(AF_INET, SOCK_DGRAM)

def new_conn(p, u, payload, ack):
	m = XCacheInfo()
	m.url = u.url
	m.start = u.start
	m.path = u.basename
	m.short = u.short
	m.ext = u.ext
	m.sha = u.sha
	m.stat = 'waiting'
	m.fp = open(m.short+'file', 'wb+')
	m.fph = open(m.short+'rsp.txt', 'wb+')
	m.ack = ack
	m.clen = 0
	m.dump()
	open(m.short+'req.txt', 'wb+').write(payload)
	conn[p] = m
	stat.inc('new', 1)
	stat.inc('start0' if m.start == 0.0 else 'start1', 1)

def check_request(p, payload, ack):
	r = re.match(r'^GET (\S+) \S+\r\n', payload)
	if r is None:
		return 
	url = r.groups()[0]
	u = XCacheURL(url)
	if u.ext not in exts:
		return 
	print 'GET', url
	if os.path.exists(u.short):
		m = XCacheInfo(u.short)
		if (m.stat == 'cached' and u.start < m.start) or \
				m.stat == 'error' or \
				m.stat == 'waiting' or \
				m.stat == 'caching':
			print 'REWRITE', m
			stat.inc('rewrite_'+m.stat, 1)
			new_conn(p, u, payload, ack)
	else:
		print 'NEW', u.short
		os.mkdir(u.short)
		os.symlink('file', u.short+'file'+u.ext)
		new_conn(p, u, payload, ack)

def del_conn(p):
	conn[p].fp.close()
	conn[p].fph.close()
	conn[p].dump()
	del conn[p]

def check_finish(p, rst=False):
	if p in conn:
		m = conn[p]
		if m.fp.tell() >= m.clen and m.stat == 'caching':
			m.fp.truncate(m.clen)
			m.stat = 'cached'
			print 'CACHED', m
			stat.inc('cached', 1)
			del_conn(p)
		elif rst:
			m.stat = 'error'
			m.reason = 'got rst %d/%d' % (m.fp.tell(), m.clen)
			print 'RST', m
			stat.inc('error.rst', 1)
			del_conn(p)

def check_response(p, pos, payload):
	m = conn[p]
	m.fph.write(payload)
	if pos > 8192:
		m.stat = 'error'
		m.reason = 'invalid header'
		del_conn(p)
		stat.inc('error.hdr', 1)
		print 'ERROR', m
		return 
	f = StringIO(payload)
	if pos == 0:
		r = f.readline()
		if '200' not in r:
			m.stat = 'error'
			m.reason = 'rsp not 200 OK'
			del_conn(p)
			stat.inc('error.rsp', 1)
			print 'ERROR', m
			return 
	while True:
		l = f.readline()
		if l == '\r\n' or l == '':
			break
		g = re.match(r"^Content-Length: (\d+)", l)
		if g is not None:
			m.clen = int(g.groups()[0])
	if l == '\r\n' and m.clen > 0:
		m.stat = 'caching'
		m.hdrlen = pos + f.tell()
		print 'clen', m.clen, 'pos', pos, 'tell', f.tell()
		m.fp.write(payload[f.tell():])
		m.dump()
		stat.inc('caching', 1)
		print 'CACHING', m

def process_packet(srcip, dstip, srcport, dstport, seq, ack, tcpflags, payload, get, tome):
	p = (srcip, dstip, srcport, dstport)
	stat.inc('packet_bytes', len(payload), care=0)
	stat.inc('packet_nr', 1, care=0)
	if get == 1 and p not in conn and not tome:
		check_request(p, payload, ack)
	p2 = (dstip, srcip, dstport, srcport)
	if p2 in conn and len(payload) > 0:
		m = conn[p2]
		pos = seq - m.ack
		if m.stat == 'waiting':
			check_response(p2, pos, payload)
		elif m.stat == 'caching' and pos > 0:
			stat.inc('io_bytes', len(payload), thresold=200000)
			m.fp.seek(pos - m.hdrlen)
			m.fp.write(payload)
			check_finish(p2)
	if (p in conn or p2 in conn) and (tcpflags & 5) != 0:
		check_finish(p2, True)
		check_finish(p, True)
	if stat.cared:
		sock.sendto(stat.dumps(), ('localhost', 1653))
		stat.clear()

if __name__ == '__main__':
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 111, "GET /var/heelo.flv?start=10 HTTP2.2\r\n", 1)
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 5, "GET dd", 1)

