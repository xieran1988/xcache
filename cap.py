#!/usr/bin/python

import re, os, shutil, hashlib, marshal, logging, traceback, pcap
from urlparse import *
from socket import *

conn = {}
sock = socket(AF_INET, SOCK_DGRAM)
addr = 'localhost', 1653
cache = '/var/lib/xcache'

def should_cache(payload):
	r = re.match(r'^GET (\S+) \S+\r\n', payload)
	if r is None:
		return None
	url = r.groups()[0]
	u = urlparse(url)
	root, ext = os.path.splitext(u.path)
	print 'GET', url
	if ext != '.flv' and ext != '.mp4':
		return None
	basename = os.path.basename(u.path)
	short = cache+'/%s/'%hashlib.sha1(basename).hexdigest()[:7]
	qs = parse_qs(u.query)
	print '\t', qs
	if len(qs) == 0:
		start = 0.0
	else:
		if 'start' not in qs:
			return None
		try:
			start = float(qs['start'][0])
		except:
			return None
	info = {'stat':'caching', 'start':start, 'path':basename, 'short':short, 'ext':ext}
	if not os.path.exists(short):
		return info
	m = marshal.load(open(short+'info','rb'))
	print '\talready exists', m
	if m['stat'] == 'cached' and start < m['start']:
		return info
	if m['stat'].startswith('error'):
		return info
	return None

def process_packet(srcip, dstip, srcport, dstport, seq, ack, tcpflags, payload, get):
	p = (srcip, dstip, srcport, dstport)
	if get == 1 and p not in conn:
		m = should_cache(payload)
		if m:
			print 'start caching', m['short'], m['start']
			if not os.path.exists(m['short']):
				os.mkdir(m['short'])
			marshal.dump(m, open(m['short']+'info','wb+'))
			conn[p] = {
					'ack':ack, 'info':m, 
					'fp':open(m['short']+'pcap', 'wb+'),
					}
	p = (dstip, srcip, dstport, srcport)
	if p in conn:
		c = conn[p]
		fp = c['fp']
		pos = seq - c['ack']
		m = c['info']
		short = m['short']
		if pos >= 0 and len(payload) > 0:
			fp.seek(pos)
			fp.write(payload)
		if (tcpflags & 5) != 0: # fin & rst
			fp.close()
			m['stat'] = 'processing'
			print 'caching done', short
			marshal.dump(m, open(short+'info','wb+'))
			sock.sendto('', addr)
			del conn[p]

if __name__ == '__main__':
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 111, "GET /var/heelo.flv?start=10 HTTP2.2\r\n", 1)
	process_packet(0x123, 0x123, 11, 11, 1111, 1111, 5, "GET dd", 1)

