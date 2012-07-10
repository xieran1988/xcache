#include <Python.h>
#include <stdio.h>
#include <arpa/inet.h>

static PyObject *py_process_packet;
static PyObject *py_invalid_fuck;
static PyObject *py_mod;

static void py_assert(void *p)
{
	if (!p) {
		PyErr_Print();
		exit(1);
	}
}

void xcache_process_packet(uint8_t *p, int plen)
{
	char *pay;
	uint8_t *ip, *tcp, tcpf;
	uint32_t iplen, tcplen, totlen, paylen;
	uint32_t dip, sip,  seq, ack;
	uint16_t dport, sport;

	static int fn, fnn, finv;
	if (*(uint16_t*)(p+4) == *(uint16_t*)"it" &&
			*(uint16_t*)(p+10) == *(uint16_t*)"sh")
	{
		fn++;
	} else if (
			*(uint16_t*)(p+4) == *(uint16_t*)"ck" &&
			*(uint16_t*)(p+10) == *(uint16_t*)"fu")
	{
		if (fnn && fn != 50000-1) {
			finv = fn;
		} else {
			fn = 0;
			fnn++;
		}
	}
	if (!fnn) 
		return ;
	if (finv) {
		py_assert(PyObject_CallFunction(py_invalid_fuck, "i", finv));
		fnn = 0; finv = 0;
		return ;
	}

	ip = (uint8_t *)p + 14;
	iplen = (*ip&0x0f)*4;
	tcp = (uint8_t *)p + 14 + iplen;
	totlen = htons(*(uint16_t*)(ip+2));
	tcplen = (*(tcp+12)&0xf0)>>2;
	sport = htons(*(uint16_t*)(tcp));
	dport = htons(*(uint16_t*)(tcp+2));
	pay = (char *)(p + 14 + iplen + tcplen);
	sip = *(uint32_t*)(ip+12);
	dip = *(uint32_t*)(ip+16);
	seq = htonl(*(uint32_t*)(tcp+4));
	ack = htonl(*(uint32_t*)(tcp+8));
	tcpf = *(tcp+13);
	paylen = totlen - iplen - tcplen;

	/*
	PyObject *r = PyObject_CallFunction(py_process_packet, "kkkkiiks#",
			sip, dip, sport, dport, (int)ack, (int)seq, tcpf, pay, paylen);
	py_assert(r);
	*/
}

void xcache_init(void)
{
	Py_Initialize();
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('/usr/lib/xcache')");
	py_mod = PyImport_ImportModule("cap");
	py_assert(py_mod);
	py_process_packet = PyObject_GetAttrString(py_mod, "process_packet");
	py_assert(py_process_packet);
	py_invalid_fuck = PyObject_GetAttrString(py_mod, "invalid_fuck");
	py_assert(py_invalid_fuck);
	setbuf(stdout, NULL); 
}

