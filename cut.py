#!/usr/bin/python

import os
import sys
import shutil
import logging
from flvlib.constants import *
from flvlib.astypes import MalformedFLV, FLVObject
from flvlib.tags import *
from flvlib.helpers import *
from flvlib.primitives import *
from flup.server.fcgi_base import *
from flup.server.fcgi import *
from flup.server.threadedserver import *
from xcache import *

def cutflv(filename, start, tellsize=False):

	fi = open(filename, 'rb')
	flv = FLV(fi)

	h = create_flv_header()
	startpos = len(h)
	yield h

	tag_iter = flv.iter_tags()
	while True:
		tag = tag_iter.next()
		if isinstance(tag, ScriptTag):
			meta = tag.variable
			break

	avhdrstart = fi.tell()
	tag = tag_iter.next()
	tag = tag_iter.next()
	avhdrsize = fi.tell() - avhdrstart

	oldtimes = meta.keyframes.times
	oldpos = meta.keyframes.filepositions
	newtimes = filter(lambda x:x>start, oldtimes)
	firsttime = newtimes[0]
	newtimes = map(lambda x:x-firsttime, newtimes)
	newpos = oldpos[-len(newtimes):]
	firstpos = newpos[0]

	startts = {}
	fi.seek(firstpos)
	while len(startts) < 2: 
		tag = tag_iter.next()
		startts[type(tag)] = tag.timestamp
	#print startts

	meta.keyframes.times = [oldtimes[0]] + newtimes
	meta.keyframes.filepositions = [oldpos[0]] + newpos
	s = create_script_tag('onMetaData', meta)
	startpos += len(s) + avhdrsize
	newpos = map(lambda x:x-firstpos+startpos, newpos)

	meta.duration -= firsttime;
	meta.keyframes.times = [oldtimes[0]] + newtimes
	meta.keyframes.filepositions = [oldpos[0]] + newpos
	yield create_script_tag('onMetaData', meta)

	fi.seek(avhdrstart)
	yield fi.read(avhdrsize)

	fi.seek(firstpos)
	try :
		while True: 
			i = fi.tell()
			tag = tag_iter.next()
			size = fi.tell() - i
			fi.seek(i)
			yield fi.read(4)
			fi.read(4)
			yield make_si32_extended(tag.timestamp - startts[type(tag)])
			yield fi.read(size-8)
	except StopIteration:
		pass

	fi.close()

if __name__ == '__main__':
	def app(env, start_rsp):
		start_rsp('200 OK', [ ('Content-Type', 'video/x-flv') ])
		u = XCacheURL(env['REQUEST_URI'])
		for i in cutflv('/var/www'+u.u.path, u.start):
			yield i
	WSGIServer(app).run()

