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

#define TIMEOUT 1

void xcache_process_packet(uint8_t *p, int plen, int sec, int usec);
void xcache_init(void);

#define dbg printf

typedef struct {
//	struct list_head l;
	FILE *f, *f_iostat;
	int fd;
	int r206;
	char p[256];
	char p_iostat[256];
	char get[1600];
	int totio, io, ack, reqlen;
	double lastiotm, conntm;
	unsigned int h;
	int rsp;
	char s;
	int buflen;
} conn_t ;

#define NR 4800
conn_t conn[NR];
static int nrconn, maxconn;
static FILE *logf;

static int f_delsmall, f_got;
static int totio;
static int nrrsp;
static double curtm;

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
	c->lastiotm = curtm;
}

static void file_close(conn_t *c) {
	if (c->f)
		fclose(c->f);
}

static void dump_iostat(conn_t *c) {
	if (c->f_iostat)
		fprintf(c->f_iostat, "%.6lf %d\n", curtm, c->io);
}

static unsigned int hash(int a, int b, int c, int d) {
	int r = (a*33+b*37+c*23+d*3);
	return r;
}

static char *mymktemp(char *p) { return mktemp(p); }

static int opt_notdelsmall = 1;
static int opt_ziotimeout = 3;

static void fin_conn(conn_t *c, char *s, char C)
{
	dump_iostat(c);
	file_close(c);
	if (c->f_iostat)
		fclose(c->f_iostat);
	if (!opt_notdelsmall && c->totio < 1000*1024 && !c->r206) {
		f_delsmall++;
		unlink(c->p);
		unlink(c->p_iostat);
	} else {
		f_got++;
		char tmp[128];
		strcpy(tmp, c->p);
		tmp[3] = C;
		rename(c->p, tmp);
	}
	fprintf(logf, "fin %s %u %d %s\n",
		 	s, c->h, c->totio, c->r206 ? " r206" : "");
	memset(c, 0, sizeof(*c));
	nrconn--;
}

void get_socket_stat(int *, int *);

static void timer(void)
{
	int i, active = 0, n = 0;
	for (i = 0; i < NR; i++) {
		conn_t *c = &conn[i];
		if (!c->h)
			continue;
		if (c->io)
			active++;
		dump_iostat(c);
		if (opt_ziotimeout > 0 && (curtm - c->lastiotm) >= opt_ziotimeout)
			fin_conn(c, "zero-io", 'Z');
		c->io = 0;
		n++;
	}
	int skipped, drops;
	get_socket_stat(&skipped, &drops);

	double mb = 1024*1024;
	fprintf(logf, 
					"info %d %d %d %d %d %d "
					"A %d H %d io %.2fM max %d rsp %d drop %d/%d\n",
				 	totio, maxconn, active, f_delsmall, f_got, (int)time(0),
					active, n, totio/mb, maxconn, nrrsp, skipped, drops
					);
	nrrsp = maxconn = 0;
	f_delsmall = f_got = totio = 0;
}

static int fuck_detect(uint8_t *p)
{
	static int pcnt, shitcnt, fuckcnt;
	static int fuck_nr = 100;
	uint16_t a = *(uint16_t*)(p+4);
	uint16_t b = *(uint16_t*)(p+10);

	pcnt++;
	//fprintf(logf, "a=%.2x b=%.2x s=%d f=%d\n", a, b, shitcnt, fuckcnt);

	if (a == 0x7469 && b == 0x6873) {
		shitcnt++;
		//fprintf(logf, "shit\n");
	} else if (a == 0x6b63 && b == 0x7566) {
		fuckcnt++;
	} else {
	}
	if (fuckcnt >= 1) {
		if (shitcnt != fuck_nr - 1)
			fprintf(logf, 
					"fucked pcnt %d shit %d fuck %d\n", 
					pcnt, shitcnt, fuckcnt
					);
		shitcnt = fuckcnt = 0;
	}
	return 0;
}

void xcache_process_packet(uint8_t *p, int plen, int sec, int usec)
{
	char *pay;
	uint8_t *ip, *tcp, tcpf;
	uint32_t iplen, tcplen, totlen, paylen;
	uint32_t dip, sip,  seq, ack;
	uint16_t dport, sport;

	fuck_detect(p);

	static double lasttm;	
	curtm = sec + usec/1000000.;
	if (curtm - lasttm > TIMEOUT) {
		timer();
		lasttm = curtm;
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

	unsigned int h = (dport == 80) ? 
		hash(sip, dip, sport, dport) : 
		hash(dip, sip, dport, sport);

	int i, j;
	conn_t *c;

 	j = (h % NR);
	for (i = 0; i < NR; i++) {
		c = &conn[j];
		if (c->h == h || !c->h) 
			break;
		j = (j + 1) % NR;
	}
	if (i == NR)
		return ;

	if (paylen > 3 && !strncmp(pay, "GET", 3)) {
		if (c->h)
			fin_conn(c, "re-get", 'G');
		fprintf(logf, "get %u\n", h);
		c->h = h;
		c->ack = (int)ack;
		strcpy(c->p, "/c/A.XXXXXX");
		mymktemp(c->p);
		strcpy(c->p_iostat, c->p);
		c->p_iostat[3] = 'I';
		memcpy(c->get, pay, paylen);
		c->reqlen = paylen;
		c->conntm = curtm;
		nrconn++;
		if (nrconn > maxconn)
			maxconn = nrconn;
		return ;
	}

	if (c->h) {
		int pos = (int)seq - c->ack;
		if (!c->rsp) {
			c->rsp++; 
			nrrsp++; 
			c->f_iostat = fopen(c->p_iostat, "w+");
			file_open(c, c->p);
			file_write(c, c->get, c->reqlen);
		}
		if (pos == 0 && paylen > 16 && !strncmp(pay+9, "206", 3)) {
			c->r206++;
		}
		file_seek(c, pos + c->reqlen);
		file_write(c, pay, paylen);
	}

	if (tcpf & 5) {
		if (c->h)
			fin_conn(c, "fin-rst", 'F');
	}
}

void xcache_init(void)
{
	if (getenv("notdelsmall"))
		opt_notdelsmall = 1;
	logf = fopen("/l/cap", "w+");
	setbuf(logf, NULL); 
	setbuf(stdout, NULL); 
	fprintf(logf, 
			"\n"
			"====== starts ======\n" 
			"%s %s zio-timeout %d\n"
			"\n",
			opt_notdelsmall ? "notdelsmall" : "",
			getenv("pcap") ? "pcap" : "",
			opt_ziotimeout
			);
}

