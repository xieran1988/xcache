#include <Python.h>
#include <glib.h> 
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

static PyObject *py_check_request;
static PyObject *py_check_finish;
static PyObject *py_process_packet;
static PyObject *py_stat_conn;
static PyObject *py_mod;
static GHashTable *ht;
char mymac[6];

static void *hash(
		uint32_t sip, uint32_t dip, 
		uint32_t sport, uint32_t dport
		) 
{
	return (void *)(sip*31+ dip*97+ sport*13+ dport*67);
}

static void py_assert(void *p)
{
	if (!p) {
		PyErr_Print();
		exit(1);
	}
}

typedef struct {
	int ack, hdrlen, id;
	int pos;
	FILE *seq;
	char stat;
} conn_t ;

static void conn_del(conn_t *c, void *h1)
{
	g_hash_table_remove(ht, h1);
	fclose(c->seq);
	free(c);
}

void xcache_process_packet(uint8_t *p, int plen)
{

	uint8_t *pay, *ip, *tcp, tcpf;
	uint32_t iplen, tcplen, totlen, paylen;
	uint32_t dip, sip,  seq, ack;
	uint16_t dport, sport;

//	struct iphdr *ih;
//	struct tcphdr *th;
//	struct ether_header *ethhdr;

//	if (plen < sizeof(struct ether_header))
//		return;
//
//	ethhdr = (struct ether_header*)p;
//	if (ethhdr->ether_type != htons(ETHERTYPE_IP))
//		return;	
//
//	p += sizeof(struct ether_header);
//	plen -= sizeof(struct ether_header);
//
//	if (plen < sizeof(struct iphdr))
//		return;
//
//	ih = (struct iphdr*)p;
//	if (ih->proto != IPPROTO_IP)
//		return;
//
//	p += ih->ihl << 2;
//	plen -= ih->ihl << 2;
//
//	if (plen < sizeof(struct tcphdr))
//		return;
//
//	th = (struct tcphdr*)p
	

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
		static uint32_t monip = 0x22a9fd77;
	if (0) {
		if (dport == 80 && !monip) {
			monip = sip;
		}
		if (sip != monip && dip != monip) 
			return ;
	}

	static int iolen, pktnr, syn, rst;
	if (1) {
		if (tcpf & 0x02)
			syn++;
		if (tcpf & 0x05)
			rst++;
		pktnr++;
		if (!(pktnr % 10000)) {
			//FILE *fp = fopen("/var/lib/xcache-log/cap-stat", "w+");
			fprintf(stdout, 
					"%d pts syn %d rst %d io %.2fM ht %d\n", 
					pktnr, syn, rst, iolen*1.0/1024/1024,
					g_hash_table_size(ht)
					);
			//fclose(fp);
			iolen = 0; syn = 0; rst = 0;
//			PyObject *r = PyObject_CallFunction(py_stat_conn, "");
//			py_assert(r);
		}
	}

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

#if 1
	do {
		int i;
		void *h1 = dport == 80 ? 
			hash(sip, dip, sport, dport) : hash(dip, sip, dport, sport);
		conn_t *c = (conn_t *)g_hash_table_lookup(ht, h1);
		char *cache = "/var/lib/xcache";
		char path[512];
		if (get) {
			if (c)
				conn_del(c, h1);
			c = (conn_t *)malloc(sizeof(*c));
			c->ack = (int)ack;
			c->stat = 'w';
			c->id = rand();
			sprintf(path, "%s/%d.txt", cache, c->id);
			c->seq = fopen(path, "w+");
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
					fprintf(c->seq, "%d\n", clen); 
					fprintf(c->seq, "%d %d\n", 0, paylen - c->hdrlen); 
					fflush(c->seq);
				}
			} else {
				if (c->stat == 'c') {
					fprintf(c->seq, "%d %d\n", pos - c->hdrlen, paylen); 
					fflush(c->seq);
				}
			}
		}
		static int cached;
		if (c && (tcpf & 5)) {
			if (c->stat == 'c') {
				fprintf(c->seq, "-1 -1\n");
				fflush(c->seq);
				conn_del(c, h1);
				cached++;
				//if (cached >= 15)
				//	exit(1);
				fprintf(stdout, "cached %d\n", cached);
				char cmd[512];
				sprintf(cmd, "seq2=1 /root/xcache/seq %s/%d.txt", cache, c->id);
				system(cmd);
			}
		}
	} while (0);
	return ;
#endif

	if (1) {
		void *h1 = hash(sip, dip, sport, dport);
		void *h2 = hash(dip, sip, dport, sport);
		void *r1 = g_hash_table_lookup(ht, h1);
		if (get && dport == 80) {
			if (r1) {
				g_hash_table_remove(ht, h1);
				g_hash_table_remove(ht, h2);
			}
			PyObject *r = PyObject_CallFunction(
					py_check_request, "Os#k", 
					r1 ? r1 : Py_None, pay, paylen, ack
					);
			py_assert(r);
			if (r != Py_None) {
				g_hash_table_insert(ht, h1, r);
				g_hash_table_insert(ht, h2, r);
			}
		} else if (r1 && paylen > 0 && sport == 80) {
			iolen += paylen;
			PyObject *r = PyObject_CallFunction(
					py_process_packet, "Os#kk", 
					r1, pay, paylen, seq, tcpf&0x5
					);
			py_assert(r);
			if (r == Py_True) {
				g_hash_table_remove(ht, h1);
				g_hash_table_remove(ht, h2);
			}
		}
	}
}

void xcache_init()
{
	ht = g_hash_table_new(g_direct_hash, g_direct_equal);

	struct ifreq ifr;
	int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFHWADDR, &ifr);
	memcpy(mymac, ifr.ifr_hwaddr.sa_data, 6);

	Py_Initialize();
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('/usr/lib/xcache')");
	py_mod = PyImport_ImportModule("cap");
	py_assert(py_mod);
	py_check_request = PyObject_GetAttrString(py_mod, "check_request");
	py_process_packet = PyObject_GetAttrString(py_mod, "process_packet");
	py_check_finish = PyObject_GetAttrString(py_mod, "check_finish");
	py_stat_conn = PyObject_GetAttrString(py_mod, "stat_conn");
}

