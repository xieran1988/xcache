#include <Python.h>
#include <pcap.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <glib.h> 

static PyObject *py_check_request;
static PyObject *py_check_finish;
static PyObject *py_process_packet;
static PyObject *py_stat_conn;
static PyObject *py_mod;
static u_char mymac[6];
static GHashTable *ht;

static void *hash(u_int sip, u_int dip, u_int sport, u_int dport) 
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

void xcache_process_packet(u_char *p)
{
	u_char *payload, *ip, *tcp, tcpflags;
	u_int iplen, tcplen, totlen, plen;
	u_int dip, sip, dport, sport, seq, ack;

	ip = (u_char *)p + 14;
	iplen = (*ip&0x0f)*4;
	tcp = (u_char *)p + 14 + iplen;
	totlen = htons(*(u_short*)(ip+2));
	tcplen = (*(tcp+12)&0xf0)>>2;
	sport = htons(*(u_short*)(tcp));
	dport = htons(*(u_short*)(tcp+2));
	payload = (u_char *)(p + 14 + iplen + tcplen);
	sip = *(u_int*)(ip+12);
	dip = *(u_int*)(ip+16);
	seq = htonl(*(u_int*)(tcp+4));
	ack = htonl(*(u_int*)(tcp+8));
	tcpflags = *(tcp+13);
	plen = totlen - iplen - tcplen;

	static u_int monip = 0x22a9fd77;
	if (0) {
		if (dport == 80 && !monip) {
			monip = sip;
		}
		if (sip != monip && dip != monip) 
			return ;
	}

	static int iolen, packet_nr, syn, rst;
	if (1) {
		if (tcpflags & 0x02)
			syn++;
		if (tcpflags & 0x05)
			rst++;
		packet_nr++;
		if (!(packet_nr % 10000)) {
			fprintf(stderr, 
					"%d pts syn %d rst %d io %.2fM ht %d\n", 
					packet_nr, syn, rst, iolen*1.0/1024/1024,
					g_hash_table_size(ht)
					);
			iolen = 0; syn = 0; rst = 0;
			PyObject *r = PyObject_CallFunction(py_stat_conn, "");
			py_assert(r);
		}
	}

	if (!memcmp(mymac, p, 6))
		return ;

	int get = 0;
	if (1) {
		if (plen > 30 && !strncmp("GET", payload, 3)) {
			int i = plen;
			char *s = (char *)payload;
			while (i--) {
				if (*s == '\r')
					break;
				s++;
			}
			if (i >= 0)
				*s = 0;
			//printf("%s\n", payload);
			char *exts[] = {"exe", "flv", "mp4", "mp3", "rar", "zip"};
			for (i = 0; i < sizeof(exts)/sizeof(exts[0]); i++) 
				if (strstr(payload, exts[i])) {
					get = 1;
					*s = '\r';
				}
		}
	}

	if (0) {
		void *h1 = hash(sip, dip, sport, dport);
		void *h2 = hash(dip, sip, dport, sport);
		void *r1 = g_hash_table_lookup(ht, h1);
		if ((tcpflags & 0x02) && get && !r1) {
			g_hash_table_insert(ht, h1, (void *)1);
			g_hash_table_insert(ht, h2, (void *)1);
		}
		if (tcpflags & 0x05) {
			g_hash_table_remove(ht, h1);
			g_hash_table_remove(ht, h2);
		}
	}

	if (1) {
		PyObject_CallFunction();
		if (get && dport == 80) {
			PyObject *r = PyObject_CallFunction(
					py_check_request, "Os#k", 
					r1 ? r1 : Py_None, payload, plen, ack
					);
			if (r != Py_None && !r1) {
				g_hash_table_insert(ht, h1, r);
				g_hash_table_insert(ht, h2, r);
			}
		}
		if (!get && r1 && plen > 0 && sport == 80) {
			iolen += plen;
			PyObject *r = PyObject_CallFunction(
					py_process_packet, "Os#k", 
					r1, payload, plen, seq
					);
			py_assert(r);
			if (r == Py_None) {
				g_hash_table_remove(ht, h1);
				g_hash_table_remove(ht, h2);
				r1 = NULL;
			}
		}
		if ((tcpflags & 5) && r1) {
			PyObject *r = PyObject_CallFunction(py_check_finish, "O", r1);
			py_assert(r);
			g_hash_table_remove(ht, h1);
			g_hash_table_remove(ht, h2);
			Py_DECREF(r1);
		}
	}
}

static void process_pcap(u_char *args, 
		const struct pcap_pkthdr *hdr, const u_char *p)
{
	xcache_process_packet((u_char *)p);
}

void xcache_init()
{
	ht = g_hash_table_new(g_direct_hash, g_direct_equal);

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

#ifdef XCACHE

int main(int argc, char *argv[])
{
	pcap_t *p;
	struct bpf_program fp;
	struct ifreq ifr;

	xcache_init();

	if (argc > 1 && strstr(argv[1], ".scap")) {
		FILE *fp = fopen(argv[1], "rb");
		int len;
		static char packet[1*1024*1024];
		printf("opend %s\n", argv[1]);
		while (fread(&len, 1, sizeof(len), fp) == sizeof(len)) {
			fread(packet, 1, len, fp);
			//printf("process %d\n", len);
			xcache_process_packet(packet);
		}
		exit(0);
	}

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
	ioctl(pcap_fileno(p), SIOCGIFHWADDR, &ifr);
	memcpy(mymac, ifr.ifr_hwaddr.sa_data, 6);

	if (argc > 1) {
		p = pcap_open_offline(argv[1], NULL);
	} else 
		p = pcap_open_live("eth1", BUFSIZ, 1, 1000, NULL);
	pcap_compile(p, &fp, "port 80", 0, 0);
	pcap_setfilter(p, &fp);
	pcap_loop(p, -1, process_pcap, NULL);

	return 0;
}

#endif
