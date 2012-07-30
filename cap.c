#define _GNU_SOURCE 
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

void xcache_process_packet(uint8_t *p, int plen);
void xcache_init(void);

#define dbg printf

typedef struct {
#define IOBUFSIZ 8192
	char buf[IOBUFSIZ + 2000];
	double t;
	FILE *f;
	int fd;
	char p[256];
	char get[1600];
	int totio, io, h, ack, reqlen;
	int rsp;
	char s;
	int buflen;
} conn_t __attribute__ ((aligned (IOBUFSIZ)));

#define NR 4800
conn_t conn[NR];
static int nrconn, maxconn;
static FILE *logf;

static int f_delsmall, f_got;
static int totio;

static int alrm;
static int nrrsp;

static void file_open(conn_t *c, char *p) {
	c->f = fopen(p, "wb+");
	if (!c->f) {
		printf("open %s failed: %s\n", p, strerror(errno));
		exit(1);
	}
}

static void file_seek(conn_t *c, long pos) {
	fseek(c->f, pos, SEEK_SET);
}

static void file_write(conn_t *c, void *buf, int len) {
	fwrite(buf, 1, len, c->f);
	c->io += len;
	c->totio += len;
	totio += len;
}

static void file_close(conn_t *c) {
	if (c->f)
		fclose(c->f);
}

static int hash(int a, int b, int c, int d) {
	return a*33+b*37+c*23+d*3;
}

static char *mymktemp(char *p) { return mktemp(p); }

static void fin_conn(conn_t *c, char *s)
{
	file_close(c);
	if (c->totio < 1000*1024) {
		f_delsmall++;
		unlink(c->p);
	} else {
		f_got++;
		char tmp[128];
		strcpy(tmp, c->p);
		tmp[3] = 'C';
		rename(c->p, tmp);
	}
	fprintf(logf, "fin %s %d\n", s, c->totio);
	memset(c, 0, sizeof(*c));
	nrconn--;
}

static void timer(void)
{
	int i, active=0, io=0;
	for (i = 0; i < NR; i++) {
		conn_t *c = &conn[i];
		if (!c->h)
			continue;
		if (c->io)
			active++;
		if (!c->io)
			fin_conn(&conn[i], "zero-io");
		c->io = 0;
	}
	double mb = 1024*1024;
	fprintf(getenv("logstdout") ? stdout : logf, 
					"info %d %d %d %d %d %d "
					"active %d io %.2fM max %d nrrsp %d\n",
				 	totio, maxconn, active, f_delsmall, f_got, (int)time(0),
					active, totio/mb, maxconn, nrrsp
					);
	nrrsp = maxconn = 0;
	f_delsmall = f_got = totio = 0;
}

static int t_pcnt;

void xcache_process_packet(uint8_t *p, int plen)
{
	char *pay;
	uint8_t *ip, *tcp, tcpf;
	uint32_t iplen, tcplen, totlen, paylen;
	uint32_t dip, sip,  seq, ack;
	uint16_t dport, sport;

	t_pcnt++;
	if (alrm) {
		timer();
		alrm = 0;
	}

	if (*(uint16_t *)(p + 12) == 0x6488) {
		ip = (uint8_t *)p + 14 + 8;
	} else {
		ip = (uint8_t *)p + 14;
	}
	iplen = (*ip&0x0f)*4;
	tcp = ip + iplen;
	totlen = htons(*(uint16_t*)(ip+2));
	tcplen = (*(tcp+12)&0xf0)>>2;
	sport = htons(*(uint16_t*)(tcp));
	dport = htons(*(uint16_t*)(tcp+2));
	sip = *(uint32_t*)(ip+12);
	dip = *(uint32_t*)(ip+16);
	seq = htonl(*(uint32_t*)(tcp+4));
	ack = htonl(*(uint32_t*)(tcp+8));
	tcpf = *(tcp+13);
	pay = (char *)(ip + iplen + tcplen);
	paylen = totlen - iplen - tcplen;

//		printf("type=%.4x iplen=%d paylen=%d totlen=%d\n", 
//			*(uint16_t*)(p+12), iplen, paylen, totlen);

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

	if (paylen > 3 && !strncmp(pay, "GET", 3)) {
		if (c)
			fin_conn(c, "re-get");
		else {
			for (i = 0; i < NR; i++)
				if (!conn[i].h) {
					c = &conn[i];
					c->h = h;
					c->ack = (int)ack;
					strcpy(c->p, "/c/A.XXXXXX");
					mymktemp(c->p);
					memcpy(c->get, pay, paylen);
					c->reqlen = paylen;
					nrconn++;
					if (nrconn > maxconn)
						maxconn = nrconn;
					break;
				}
		}
		return ;
	}

	if (c) {
		int pos = (int)seq - c->ack;
		if (!c->rsp) {
			c->rsp++; 
			nrrsp++; 
			file_open(c, c->p);
			file_write(c, c->get, c->reqlen);
			file_write(c, pay, paylen);
		}
		file_seek(c, pos + c->reqlen);
		file_write(c, pay, paylen);
	}

	if (tcpf & 5) {
		if (c)
			fin_conn(c, "fin-rst");
	}
}

static void sigalarm(int _D)
{
	alrm++;
	alarm(4);
	t_pcnt = 0;
}

void xcache_init(void)
{
	logf = fopen("/l/cap", "w+");
	setbuf(logf, NULL); 
	signal(SIGALRM, sigalarm);
	alarm(1);
	setbuf(stdout, NULL); 
}

