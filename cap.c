#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <arpa/inet.h>

void xcache_process_packet(uint8_t *p, int plen);
void xcache_init(void);

static double now(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec+tv.tv_usec/1000000.;
}

typedef struct {
	double t;
	FILE *f;
	char p[256];
	int io, h, ack, reqlen;
	char s;
} conn_t;
#define NR 2048
conn_t conn[NR];

static int hash(int a, int b, int c, int d) {
	return a*33+b*37+c*23+d*3;
}

static void fin_conn(conn_t *c, char s)
{
	//printf("fin %p f=%p\n", c, c->f);
	fclose(c->f);
	if (s == 'r') {
		char tmp[128] = "/tmp/C.XXXXXX";
		rename(c->p, mktemp(tmp));
	}
	if (s == 'd') {
		unlink(c->p);
	}
	//printf("fin %p done\n", c);
	memset(c, 0, sizeof(*c));
}

static void timer(void)
{
	int i, active=0, io=0;
	for (i = 0; i < NR; i++) {
		if (!conn[i].io && conn[i].f) 
			fin_conn(&conn[i], 'd');
		io += conn[i].io;
		conn[i].io = 0;
		if (conn[i].f)
			active++;
	}
	printf("active %d io %.2fM\n", active, io/1024./1024);
}

void xcache_process_packet(uint8_t *p, int plen)
{
	char *pay;
	uint8_t *ip, *tcp, tcpf;
	uint32_t iplen, tcplen, totlen, paylen;
	uint32_t dip, sip,  seq, ack;
	uint16_t dport, sport;

	static int tn;
	static double last;
	tn++;
	if (!(tn%1000) && now() - last > 1.0) {
		timer();
		last = now();
	}

	/*
	static int fn, fnn, finv, ninv;
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
			printf("fuck\n");
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
	*/

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

	int h = dport == 80 ? 
		hash(sip, dip, sport, dport) : 
		hash(dip, sip, dport, sport);

	int i;
	conn_t *c = NULL;

	for (i = 0; i < NR; i++)
		if (conn[i].h == h) {
			c = &conn[i];
			break;
		}
	//printf("c=%p\n", c);

	if (!strncmp(pay, "GET", 3)) {
		//printf("GET c=%p\n", c);
		if (c)
			fin_conn(c, 'r');
		else {
			for (i = 0; i < NR; i++)
				if (!conn[i].f) {
					c = &conn[i];
					c->h = h;
					c->ack = (int)ack;
					strcpy(c->p, "/tmp/A.XXXXXX");
					mktemp(c->p);
					c->f = fopen(c->p, "wb+");
					fwrite(pay, paylen, 1, c->f);
					c->reqlen = ftell(c->f);
					break;
				}
		}
		return ;
	} 

	if (c) {
		c->io += paylen;
		fseek(c->f, (int)seq - c->ack + c->reqlen, 0);
		fwrite(pay, paylen, 1, c->f);
	}

	if (tcpf & 5) {
		if (c)
			fin_conn(c, 'r');
	}
}

void xcache_init(void)
{
	setbuf(stdout, NULL); 
}

