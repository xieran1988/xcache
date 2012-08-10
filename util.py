#!/usr/bin/python

import re, os, hashlib, marshal, time, random, sys, traceback, datetime

logf = open('/l/cgi2', 'a+')

def logw(s):
	logf.write(s+'\n')
	logf.flush()

def logend():
	logw('')

def logsha(sha, rs, re, clen, result):
	try:
		open('/d/JMP.%s'%sha, 'a+').write('%s\n' % (
			'|'.join('jmp', repr(time.time()), rs, re, clen, result)
			))
	except:
		pass

def calc_http_range(ran, clen):
	if ran is None:
		return 0, clen-1
	if not re.match(r'^(\d+)?-(\d+)?$', ran):
		return 
	s, e = ran.split('-')
	if len(s) > 0 and len(e) > 0:
		return int(s), int(e)
	elif len(s) > 0 and len(e) == 0:
		return int(s), clen-1
	elif len(s) == 0 and len(e) > 0:
		return 0, int(e)

def calc_rs(rlist, rs, re, clen):
	return filter(lambda x: x[0] <= rs and re <= x[1], rlist)

def path_get_sha(p):
	basename = os.path.basename(p)
	sha = lambda x: hashlib.sha1(x).hexdigest()[:7]
	return sha(basename)

def sha_get_rs(sha):
	p = '/d/R.%s' % sha
	logw('path: %s %s' % (p, type(p)))
	return marshal.load(open(p, 'rb'))

def url_is_valid(url):
	if 'youku' in url and 'start' in url:
		return False
	if 'mccq' in url:
		return False
	return True

def jmp(p, qry, ran, sha=None):
	err = ''
	try:
		if qry is None:
			qry = ''
		if 'yjwt' not in qry:
			if qry == '':
				qry = 'y=yjwt08'
			else:
				qry += '&y=yjwt08'

		logw('path=%s qry=%s ran=%s sha=%s' % (p, qry, ran, sha))

		if not url_is_valid(p+'?'+qry):
			logw('url not valid')
			raise

		if sha is None:
			sha = path_get_sha(p)
		logw('sha: %s' % sha)

		m = sha_get_rs(sha)
		clen = m['clen']
		logw('clen %d len(cov) %d' % (clen, len(m['cov'])))

		rs, re = calc_http_range(ran, clen)
		r = calc_rs(m['cov'], rs, re, clen)
		root = '/d/%s' % sha
		fname = os.listdir(root)[0]

		if r:
			logw('mine %s %s %s %d %d %d' % (p, sha, ran, rs, re, clen))
			logsha(sha, rs, re, clen, 'mine')
#			logend()
			return 'mine', \
						 (root+'/'+fname, rs, re-rs+1, root, '/'+fname), \
						 'bytes %d-%d/%d' % (rs, re, clen)
	except:
		traceback.print_exc()
		err = traceback.format_exc()
	to = 'http:/%s?%s' % (p, qry.replace('yjwt08', 'yjwt09'))
	logw('pass %s %s' % (to, sha))
	logsha(sha, rs, re, clen, 'pass')
#	logend()
	return 'pass', to

#logw('==== starts ====')
#logend()

if __name__ == '__main__':
	if len(sys.argv) > 1 and sys.argv[1] == 'sha':
		r = jmp('', '', None, sha=sys.argv[2])
		print r
		sys.exit(0)
	#print pass_url('/www.youku.com/hahaha', 'fuckyou&y=yjwt09')
	print calc_http_range('3-5', 12)
	print calc_http_range('-3', 12)
	print calc_http_range('3-', 12)
	print jmp('/www.youku.com/aaa.mp4', 'y=yjwt08', '1-9')
	print jmp('/www.youku.com/aaa.mp4', 'y=yjwt08', '0-9')
	#print jmp(sys.argv[1])

