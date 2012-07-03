#include <Python.h>
#include <glib.h> 
#include <time.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <poll.h>

typedef struct {
	int start, end;
} sec_t;

typedef struct {
	int ack, hdrlen, id;
	int curpos, nextpos;
	int clen;
	sec_t sec[128];
	int nsec, io, cont;
	FILE *seq;
	char stat;
	char *_s;
	int s_io;
	int _used;
	void *_hash;
} conn_t ;

#define MAX_CONN 128
conn_t conns[MAX_CONN];

static int s_totlen, s_validlen, s_iolen;

static void *hash(uint32_t sip, uint32_t dip, uint32_t sport, uint32_t dport) 
{
	return (void *)(sip*31+ dip*97+ sport*13+ dport*67);
}

static void sec_add(conn_t *c, int start, int len)
{
	int i, end = start + len;
	c->io += len;
	c->s_io += len;
	for (i = 0; i < c->nsec; i++) {
		if (c->sec[i].start <= start && c->sec[i].end >= end)
			return ;
	}
	for (i = 0; i < c->nsec; i++) {
		if (c->sec[i].end == start) {
			c->sec[i].end = end;
			return ;
		}
		if (c->sec[i].start == end) {
			c->sec[i].start = start;
			return ;
		}
	}
	if (c->nsec >= sizeof(c->sec)/sizeof(c->sec[0])) 
		return ;
	c->sec[c->nsec].start = start;
	c->sec[c->nsec].end = end;
	c->nsec++;
}

static void sec_sumup(conn_t *c) 
{
	int i;
	for (i = 1; i < c->nsec; i++) {
		if (c->sec[i].start != c->sec[i-1].end)
			return ;
	}
	c->cont = 1;
}

static conn_t *conn_new(void)
{
	int i;
	for (i = 0; i < MAX_CONN; i++)
		if (!conns[i]._used) {
			conns[i]._used++;
			return &conns[i];
		}
	return NULL;
}

static conn_t *conn_lookup(void *h)
{
	int i;
	for (i = 0; i < MAX_CONN; i++) 
		if (conns[i]._used && conns[i]._hash == h)
			return &conns[i];
	return NULL;
}

static void conn_del(conn_t *c)
{
	c->_used = 0;
}

static int conn_count(void)
{
	int c = 0, i;
	for (i = 0; i < MAX_CONN; i++) 
		c += !!conns[i]._used;
	return c;
}

static void conn_finish(conn_t *c)
{
	if (c->stat == 'c') {
		static int cached;
		cached++;
		sec_sumup(c);
		printf("cached %d %d %d ", c->clen, c->io, conn_count());
		if (c->clen - c->nextpos == 0)
			printf("complete");
		else {
			if (c->io < c->clen) 
				printf("rst %d", c->io);
			else {
				if (c->cont) {
					if (c->sec[c->nsec-1].end >= c->clen)
						printf("complete ");
					else {
						printf("rst %d", c->sec[c->nsec-1].end);
					}
				} else
					printf("hole ");
				printf("%d %d %d", c->nsec, c->io-c->clen, c->sec[c->nsec-1].end);
			}
		}
		printf("\n");
		conn_del(c);
	}
}

static int valid_get(char *pay, int paylen)
{
	int i = paylen;
	char *s = (char *)pay;
	while (i--) {
		if (*s == '\r')
			break;
		s++;
	}
	if (i >= 0) {
		*s = 0;
		char *exts[] = {"exe", "flv", "mp4", "mp3", "rar", "zip"};
		//char *exts[] = {"flv", "mp4"};
		//if (strstr(pay, "youku")) {
		for (i = 0; i < sizeof(exts)/sizeof(exts[0]); i++) {
			if (strstr(pay, exts[i])) {
				return 1;
			}
		}
		*s = '\r';
	}
	return 0;
}

static int valid_rsp(char *pay, int paylen, conn_t *c)
{
	int valid = 0;
	char ch = pay[paylen-1];
	pay[paylen-1] = 0;
	if (!strstr(pay, "200")) 
		goto fail;
	char *s = strstr(pay, "Content-Length:");
	if (!s) 
		goto fail;
	s += strlen("Content-Length:");
	while (*s == ' ')
		s++;
	int clen = atoi(s);
	if (clen < 1000*1024) 
		goto fail;
	s = strstr(pay, "\r\n\r\n");
	if (!s) 
		goto fail;
	valid = 1;
	c->_s = s;
	c->clen = clen;
fail:
	pay[paylen-1] = ch;
	return valid;
}

void xcache_process_packet(uint8_t *p, int plen);
void xcache_process_packet(uint8_t *p, int plen)
{
	char *pay;
	uint8_t *ip, *tcp, tcpf;
	uint32_t iplen, tcplen, totlen, paylen;
	uint32_t dip, sip,  seq, ack;
	uint16_t dport, sport;

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

	int get = 
		(dport == 80 && paylen > 3 && !strncmp("GET", pay, 3)) ? 
		valid_get(pay, paylen) : 0;

	void *h1 = (dport == 80) ? 
		hash(sip, dip, sport, dport) : hash(dip, sip, dport, sport);
	conn_t *c = conn_lookup(h1);

	s_totlen += plen;
	if (c || get)
		s_validlen += plen;

	if (get) {
		if (c) 
			conn_finish(c);
		c = conn_new();
		if (!c)
			return ;
		c->_hash = h1;
		c->ack = (int)ack;
		c->stat = 'w';
		c->id = rand();
	}
	
	if (c && paylen > 0 && sport == 80) {
		s_iolen += paylen;
		int pos = (int)seq - c->ack;
		if (pos == 0) {
			if (valid_rsp(pay, paylen, c) ) {
				if (c->stat == 'w') {
					c->stat = 'c';
					c->hdrlen = c->_s + 4 - pay;
					c->curpos = 0;
					c->nextpos = paylen - c->hdrlen;
					sec_add(c, 0, paylen - c->hdrlen);
				}
			} else
				conn_del(c);
		} else {
			pos -= c->hdrlen;
			if (c->stat == 'c') {
				sec_add(c, pos, paylen);
				if (pos == c->nextpos) 
					c->nextpos += paylen;
			}
		}
	}

	if (c && (tcpf & 5)) {
		conn_finish(c);
	}

	if (s_iolen > 1024*1024*1) {
		printf("iostat %d %d %d\n", s_iolen, s_totlen, s_validlen);
		s_iolen = 0; s_totlen = 0; s_validlen = 0;
	}
}

void xcache_init(void)
{
	setbuf(stdout,NULL); 
}

