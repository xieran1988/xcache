#!/usr/bin/python

import re, os, sys, datetime, mimetools, threading, json, traceback
from urlparse import *
from xcache import *
from cStringIO import *

MAXCONN = 1000
conn = {}
stat = {'io':0, 'tot':0, 'fin':0, 'reget':0, 'timeout':0, 'inv':0}
hosts = {}
conn_lock = threading.RLock()

class XCacheConn:
	pass

def mime(pay):
	try:
		r, h = pay.split('\r\n', 1)
		m = mimetools.Message(StringIO(h))
	except:
		return 
	return (m, pay)

def check_request(t, pay, ack):
	if t in conn:
		check_finish(conn[t], 'reget')
		return 
	c = XCacheConn()
	c.req = mime(pay)
	c.fp = open('/tmp/%d'%ack, 'wb+')
	c.stat = 'active'
	c.ack = ack 
	c.t_io0 = 0
	c.io = 0
	c.s_io = 0
	conn[t] = c

def check_response(c, pay):
	c.rsp = mime(pay)

def check_finish(c, s):
	if c.stat == 'active':
		stat[s] += 1
		c.stat = s
		try:
			m, pay = c.rsp
			#print 'pay', len(pay)
			clen = int(m['Content-Length'])
			if c.pos > clen:
				print 's', s, 'clen', '%.2f'%(clen/1024./1024)
				h = c.req[0]['Host']
				c.stat = 'cached'
				if h not in hosts:
					hosts[h] = 0
				hosts[h] += 1
		except:
			pass

def process_packet(sip, dip, sport, dport, ack, seq, tcpf, pay):
	with conn_lock:
		stat['tot'] += len(pay)
		t = (sip,dip,sport,dport) if sport == 80 else (dip,sip,dport,sport)
		c = conn[t] if t in conn else None
		if dport == 80:
			if pay.startswith('GET'):
				if len(conn) < MAXCONN:
					check_request(t, pay, ack)
		else:
			if c:
				c.pos = seq - c.ack + len(pay)
				if pay.startswith('HTTP'):
					check_response(c, pay)
				stat['io'] += len(pay)
				c.fp.write(pay)
				c.io += len(pay)
				c.s_io += len(pay)
		if c and tcpf & 5:
			check_finish(c, 'fin')

invfuck = []

def invalid_fuck(inv):
	with conn_lock:
		print 'INVAID FUCK:', inv
		invfuck.append([inv, datetime.datetime.now()])
		for k in conn:
			check_finish(conn[k], 'inv')
	
def timer():
	with conn_lock:
		active = 0
		max_timeout = 0
		to_del = []
		for k in conn:
			c = conn[k]
			if c.s_io == 0:
				c.t_io0 += 1
			else:
				c.t_io0 = 0
			c.s_io = 0
			if c.t_io0 > 3:
				if c.stat == 'active':
					c.stat = 'timeout'
					stat['timeout'] += 1
			if c.stat != 'active':
				to_del.append(k)
		for k in to_del:
			del conn[k]
	stat['active'] = len(conn)
	stat['io'] /= 1024*1024.
	#print json.dumps(stat)
	#print json.dumps(hosts)
	print stat['io'], len(conn), invfuck
#	for k in ['io', 'tot', 'clen']:
	for k in stat:
		stat[k] = 0
	threading.Timer(1, timer).start()

threading.Timer(1, timer).start()

