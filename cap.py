#!/usr/bin/python

import re, os, shutil, hashlib, marshal, logging, traceback
from urlparse import *
from xcache import *
from socket import *

conn = {}
sock = socket(AF_INET, SOCK_DGRAM)
addr = 'localhost', 1653

def should_cache(payload):
	r = re.match(r'^GET (\S+) \S+\r\n', payload)
	if r is None:
		return None
	url = r.groups()[0]
	u = XCacheURL(url)
	print 'GET', url
	if u.ext not in ['.flv', '.mp4']:
		return None
	print '\t', u.qs
	rm = XCacheInfo()
	rm.url = url
	rm.stat = 'caching'
	rm.start = u.start
	rm.path = u.basename
	rm.short = u.short
	rm.ext = u.ext
	rm.sha = u.sha
	if not os.path.exists(u.short):
		return rm
	m = XCacheInfo(u.short)
	print '\talready exists', m
	if m.stat == 'cached' and u.start < m.start:
		return rm
	if m.stat.startswith('error'):
		return rm
	return None

def process_packet(srcip, dstip, srcport, dstport, seq, ack, tcpflags, payload, get, tome):
	p = (srcip, dstip, srcport, dstport)
	if get == 1 and p not in conn and not tome:
		m = should_cache(payload)
		if m:
			print 'start caching', m
			if not os.path.exists(m.short):
				os.mkdir(m.short)
			m.dump()
			conn[p] = {'ack':ack, 'info':m, 'fp':open(m.short+'pcap', 'wb+')}
	p = (dstip, srcip, dstport, srcport)
	if p in conn:
		c = conn[p]
		fp = c['fp']
		pos = seq - c['ack']
		m = c['info']
		if pos >= 0 and len(payload) > 0:
			fp.seek(pos)
			fp.write(payload)
		if (tcpflags & 5) != 0: # fin & rst
			fp.close()
			m.stat = 'processing'
			print 'caching done', m
			m.dump()
			sock.sendto('', addr)
			del conn[p]

if __name__ == '__main__':
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 111, "GET /var/heelo.flv?start=10 HTTP2.2\r\n", 1)
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 5, "GET dd", 1)

