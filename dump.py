#!/usr/bin/python

from flvlib.constants import *
from flvlib.astypes import MalformedFLV, FLVObject
from flvlib.tags import *
from flvlib.helpers import *
from flvlib.primitives import *
import os, shutil, marshal, re, sys
from socket import *
from optparse import OptionParser
from xcache import *

sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(('localhost', 1653))

def validate(infile):
	fi = open(infile, 'rb')
	try:
		flv = FLV(fi)
		tag_iter = flv.iter_tags()
		while True:
			tag = tag_iter.next()
	except StopIteration:
		pass
	except:
		return 'error: file invalid'
	return 'cached'

def _process(short):
	m = XCacheInfo(short)
	if m.stat != 'processing':
		return m
	os.system('ln -sf file '+m.short+'file'+m.ext+'c')
	print 'start processing', m
	fi = open(m.short+'pcap', 'rb')
	clen = 0
	m.stat = 'error'
	while True:
		l = fi.readline()
		if l == '\r\n':
			break
		if l == '':
			m.reason = 'http header invalid'
			return m
		g = re.match(r"^Content-Length: (\d+)", l)
		if g is not None:
			clen = int(g.groups()[0])
	if clen == 0:
		m.reason = 'content-length not found'
		return m
	print '\tclen', clen
	fo = open(m.short+'file', 'wb+')
	shutil.copyfileobj(fi, fo, clen)
	if fo.tell() < clen:
		m.reason = 'file incomplete'
		return m
	fo.close()
	m.stat = 'cached'
	return m

def process(short):
	m = _process(short)
	print 'result', m
	m.dump()

def process_all():
	for i in os.listdir(cache):
		process(cache+'/'+i+'/')

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option("-p", "--path", dest="p",
												help="process cache entry in PATH", metavar="PATH")
	(opt, args) = parser.parse_args()
	if opt.p:
		process(opt.p+'/')
	else:
		s = XCacheStat()
		while True:
			path = cachelog + 'cur'
			t = XCacheStat(path)
			s.loads(sock.recvfrom(65536)[0])
			t.add(s)
			t.dump(path)
			print s.__dict__
	#		process_all()
#	processall()
#	sys.exit()
#	print dump('/home/xb/out', '/home/xb/out.flv')
#	print validate('/home/xb/out.flv')
#	os.exit()
	
