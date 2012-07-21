#define _GNU_SOURCE 
#include <stdio.h>
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

/*
static double now(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec+tv.tv_usec/1000000.;
}
*/

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

#define NR 400
conn_t conn[NR];
static int nrconn, maxconn;

static int alrm;
static int iomode; 
/*
 * 0=file buffered
 * 1=disk 4k direct
 * 2=disk 4k buffered
 * 4=file 4k buffered
 * 5=file 4k direct
 * 6=file buffered
 * 7=none
 * 8=all in one seq
 */
static int zoom = 1;
static int nrrsp;
static FILE *diskfp;

static void file_open(conn_t *c, void *p) {
	if (iomode == 1)
		c->fd = open("/dev/sdd", O_DIRECT|O_WRONLY);
	if (iomode == 0 || iomode == 4 || iomode == 6) {
		c->f = fopen(p, "wb+");
		//setvbuf(c->f, 0, _IONBF, 0);
		//setvbuf(c->f, c->buf, _IOFBF, 4096);
	}
	if (iomode == 2 || iomode == 3) 
		c->f = fopen("/dev/sdd", "wb");
	if (iomode == 5) 
		c->fd = open(c->p, O_DIRECT|O_WRONLY|O_TRUNC|O_CREAT, 0777);
	if (iomode == 8) {
	}
}

static void file_seek(conn_t *c, long pos) {
	if (iomode == 1) 
		lseek(c->fd, (c-conn)*32*1024*1024 + ((pos*zoom)&~4095), SEEK_SET);
	if (iomode == 0 || iomode == 6)
		fseek(c->f, pos*zoom, SEEK_SET);
	if (iomode == 2)
		fseek(c->f, (c-conn)*32*1024*1024 + pos*zoom, SEEK_SET);
	if (iomode == 3)
		fseek(c->f, (c-conn)*32*1024*1024 + ((pos*zoom)&~4095), SEEK_SET);
//	if (iomode == 4)
//		fseek(c->f, ((pos*zoom)&~4095), SEEK_SET);
//	if (iomode == 5)
//		lseek(c->fd, ((pos*zoom)&~4095), SEEK_SET);
}

static void write2(int fd, void *buf, int len) { 
	int r = write(fd, buf, len); 
	if (r != len) {
		printf("fucked write %d: %s fd=%d\n", 
				r, strerror(errno), fd);
		exit(10);
	}
}

static void file_write(conn_t *c, void *buf, int len) {
	int i;
	static char d[4096*2] __attribute__ ((aligned (8192))) ;
	for (i = 0; i < zoom; i++) {
		if (iomode == 1)
			write2(c->fd, d, sizeof(d));
		if (iomode == 0 || iomode == 2 || iomode == 8)	
			fwrite(buf, len, 1, c->f);
		if (iomode == 3 || iomode == 4) 
			fwrite(d, sizeof(d), 1, c->f);
		if (iomode == 5) 
			write2(c->fd, d, sizeof(d));
		if (iomode == 6) {
			if (0) {
				memcpy(c->buf+c->buflen, buf, len);
				c->buflen += len;
				if (c->buflen > IOBUFSIZ) {
					fwrite(c->buf, IOBUFSIZ, 1, c->f);
					memcpy(c->buf, c->buf+IOBUFSIZ, c->buflen-IOBUFSIZ);
					c->buflen -= IOBUFSIZ;
				}
			} else {
				fwrite(buf, 1, len, c->f);
			}
		}
		c->io += len;
	}
}

static void file_close(conn_t *c) {
	if (iomode == 1 || iomode == 5)
		close(c->fd);
	if (iomode == 0 || iomode == 2 || iomode == 3 || 
			iomode == 4 || iomode == 6 || iomode == 8)
		fclose(c->f);
}

static int hash(int a, int b, int c, int d) {
	return a*33+b*37+c*23+d*3;
}

static char *mymktemp(char *p) { return mktemp(p); }

static void fin_conn(conn_t *c, char s)
{
	//printf("fin %p f=%p\n", c, c->f);
	file_close(c);
	if (s == 'r') {
		char tmp[128] = "/c/C.XXXXXX";
		rename(c->p, mymktemp(tmp));
	}
	if (s == 'd') {
		unlink(c->p);
	}
	//printf("fin %p done\n", c);
	memset(c, 0, sizeof(*c));
	nrconn--;
}

static void timer(void)
{
	int i, active=0, io=0;
	double max=0, min=1e10, totio=0;
	for (i = 0; i < NR; i++) {
		if (!conn[i].h)
			continue;
		if (!conn[i].io) 
			fin_conn(&conn[i], 'r');
		else {
			conn[i].totio += conn[i].io;
			io += conn[i].io;
			totio += conn[i].totio;
			if (conn[i].totio > max)
				max = conn[i].totio;
			if (conn[i].totio < min)
				min = conn[i].totio;
			conn[i].io = 0;
			active++;
		}
	}
	//double mb = 1024*1024;
	printf("iomode=%d active %d io %.2fM max %d nrrsp %d\n",
				 iomode, active, io/1024./1024, maxconn, nrrsp
				);
	nrrsp = 0; maxconn = 0;
}

/*
static int fuck_detect(uint8_t *p)
{
	static int fn, fnn, finv, ninv;
	if (*(uint16_t*)(p+4) == *(uint16_t*)"it" &&
			*(uint16_t*)(p+10) == *(uint16_t*)"sh")
	{
		fn++;
		printf("shit\n");
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
		return 0;
	if (finv) {
		printf("inv fuck\n");
		fnn = 0; finv = 0;
		return 0;
	}
	return 1;
}
*/

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

	//fuck_detect(p);

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

	if (paylen > 3 && !strncmp(pay, "GET", 3)) {
		if (c)
			fin_conn(c, 'r');
		else {
			for (i = 0; i < NR; i++)
				if (!conn[i].h) {
					c = &conn[i];
					c->h = h;
					c->ack = (int)ack;
					strcpy(c->p, "/c/A.XXXXXX");
					mymktemp(c->p);
					file_open(c, c->p);
					file_write(c, pay, paylen);
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
		if (!c->rsp) { c->rsp++; nrrsp++; }
		file_seek(c, pos + c->reqlen);
		file_write(c, pay, paylen);
	}

	if (tcpf & 5) {
		if (c)
			fin_conn(c, 'r');
	}
}

static void sigalarm(int _D)
{
	if (!t_pcnt) {
		timer();
	} else {
		alrm++;
	}
	alarm(1);
	t_pcnt = 0;
}

void xcache_init(void)
{
	diskfp = fopen("/dev/sdd", "wb");
	char *s = getenv("iomode");
	iomode = atoi(s ? s : "6");
	signal(SIGALRM, sigalarm);
	alarm(1);
	setbuf(stdout, NULL); 
}

