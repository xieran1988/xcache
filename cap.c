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

static GHashTable *ht;
char mymac[6];

static void *hash(uint32_t sip, uint32_t dip, uint32_t sport, uint32_t dport) 
{
	return (void *)(sip*31+ dip*97+ sport*13+ dport*67);
}

typedef struct {
	int start, end;
} sec_t;

typedef struct {
	int ack, hdrlen, id;
	int curpos, nextpos, dup, hole;
	int clen;
	sec_t sec[128];
	int nsec, toomanysec, io, cont;
	FILE *seq;
	char stat;
	int rank;
	int s_io;
} conn_t ;

static void conn_del(conn_t *c, void *h1)
{
	g_hash_table_remove(ht, h1);
	free(c);
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
	if (c->nsec >= sizeof(c->sec)/sizeof(c->sec[0])) {
		c->toomanysec = 1;
		return ;
	}
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

static int ptotlen, validlen, iolen, io0;

static void checkio(void *key, void *val, void *user)
{
	conn_t *c = (conn_t *)val;
	if (c->s_io == 0)
		io0++;
	c->s_io = 0;
}

static void conn_finish(conn_t *c, void *h1)
{
	static int cached;
	io0 = 0;
	g_hash_table_foreach(ht, checkio, 0);
	if (c->stat == 'c') {
		//fprintf(c->seq, "-1 -1\n");
		//fflush(c->seq);
		cached++;
		sec_sumup(c);
		printf("cached %.2lfM ht=%d valid=%.2lf%%\t", 
					c->clen*1./1024/1024, g_hash_table_size(ht), validlen*100./ptotlen
					);
		iolen = 0;
		validlen = ptotlen = 0;
		if (c->clen - c->nextpos == 0) 
			printf("complete\n");
		else {
			if (c->io < c->clen) 
				printf("rst after %.2lfM", c->io/1024./1024);
			else {
				if (c->cont) {
					if (c->sec[c->nsec-1].end >= c->clen)
						printf("complete ");
					else {
						printf("rst after %.2lfM ", c->sec[c->nsec-1].end/1024./1024);
					}
				} else
					printf("hole ");
				printf("nsec=%d dup=%d end=%.2lfM", c->nsec, c->io - c->clen, c->sec[c->nsec-1].end/1024./1024);
			}
		}
		printf("\n");
		conn_del(c, h1);
	}
}

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
	pay = (uint8_t *)(p + 14 + iplen + tcplen);
	sip = *(uint32_t*)(ip+12);
	dip = *(uint32_t*)(ip+16);
	seq = htonl(*(uint32_t*)(tcp+4));
	ack = htonl(*(uint32_t*)(tcp+8));
	tcpf = *(tcp+13);
	paylen = totlen - iplen - tcplen;

	if (!memcmp(mymac, p, 6))
		return ;

	int get = 0;
	if (1) {
		if (dport == 80 && paylen > 3 && !strncmp("GET", pay, 3)) {
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
						get = 1;
						break;
					}
				}
				*s = '\r';
			}
		}
	}

	do {
		int i;
		void *h1 = dport == 80 ? 
			hash(sip, dip, sport, dport) : hash(dip, sip, dport, sport);
		conn_t *c = (conn_t *)g_hash_table_lookup(ht, h1);
		char *cache = "/var/lib/xcache";
		char path[512];
		ptotlen += plen;
		if (c || get)
			validlen += plen;
		if (get) {
			if (c) 
				conn_finish(c, h1);
			c = (conn_t *)malloc(sizeof(*c));
			memset(c, 0, sizeof(*c));
			c->ack = (int)ack;
			c->stat = 'w';
			c->id = rand();
			//sprintf(path, "%s/%d.txt", cache, c->id);
			//c->seq = fopen(path, "w+");
			g_hash_table_insert(ht, h1, c);
		}
		if (c && paylen > 0 && sport == 80) {
			int pos = (int)seq - c->ack;
			uint8_t ch = pay[paylen-1];
			iolen += paylen;
			if (pos == 0) {
				pay[paylen-1] = 0;
				if (!strstr(pay, "200")) {
					conn_del(c, h1);
					break ;
				}
				char *s = strstr(pay, "Content-Length:");
				if (!s) {
					conn_del(c, h1);
					break ;
				}
				s += strlen("Content-Length:");
				while (*s == ' ')
					s++;
				int clen = atoi(s);
				if (clen < 1000*1024) {
					conn_del(c, h1);
					break;
				}
				s = strstr(pay, "\r\n\r\n");
				if (!s) {
					conn_del(c, h1);
					break;
				}
				if (c->stat == 'w') {
					c->stat = 'c';
					c->hdrlen = s + 4 - (char *)pay;
					pay[paylen-1] = ch;
					c->clen = clen;
					c->curpos = 0;
					c->nextpos = paylen - c->hdrlen;
					sec_add(c, 0, paylen - c->hdrlen);
					//fprintf(c->seq, "%d\n", clen); 
					//fprintf(c->seq, "%d %d\n", 0, paylen - c->hdrlen); 
					//fflush(c->seq);
				}
			} else {
				pos -= c->hdrlen;
				if (c->stat == 'c') {
					sec_add(c, pos, paylen);
					if (pos != c->nextpos) {
						c->hole++;
					} else 
						c->nextpos += paylen;
					//c->curpos = pos;
					//c->nextpos = pos + paylen;
					//fprintf(c->seq, "%d %d\n", pos - c->hdrlen, paylen); 
					//fflush(c->seq);
				}
			}
		}
		if (c && (tcpf & 5)) {
			conn_finish(c, h1);
		}
	} while (0);

	if (iolen > 1024*1024*20) {
	}
}

void xcache_init()
{
	ht = g_hash_table_new(g_direct_hash, g_direct_equal);

	setbuf(stdout,NULL); 

	struct ifreq ifr;
	int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFHWADDR, &ifr);
	memcpy(mymac, ifr.ifr_hwaddr.sa_data, 6);
}

