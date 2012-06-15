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
static PyObject *py_mod;
static u_char mymac[6];
static int packet_nr, totsize;
static GHashTable *ht;

static void *hash(u_int srcip, u_int dstip, u_int srcport, u_int dstport) 
{
	return (void *)(srcip + dstip*123 + srcport*456 + dstport*789);
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
	u_int size_ip, size_tcp, totlen;
	int size_payload, get = 0, to_me = 0;
	u_int dstip, srcip, dstport, srcport, seq, ack;
	static int io_bytes;

	ip = (u_char *)p + 14;
	size_ip = (*ip&0x0f)*4;
	tcp = (u_char *)p + 14 + size_ip;
	totlen = htons(*(u_short*)(ip+2));
	size_tcp = (*(tcp+12)&0xf0)>>2;
	srcport = htons(*(u_short*)(tcp));
	dstport = htons(*(u_short*)(tcp+2));
	payload = (u_char *)(p + 14 + size_ip + size_tcp);
	srcip = *(u_int*)(ip+12);
	dstip = *(u_int*)(ip+16);
	seq = htonl(*(u_int*)(tcp+4));
	ack = htonl(*(u_int*)(tcp+8));
	tcpflags = *(tcp+13);
	size_payload = totlen - size_ip - size_tcp;

	if (!memcmp(mymac, p, 6))
		to_me = 1;
	if (size_payload > 30 && !strncmp("GET", payload, 3)) {
		int i = size_payload;
		char *s = (char *)payload;
		while (i--) {
			if (*s == '\r')
				break;
			s++;
		}
		if (i >= 0)
			*s = 0;
		char *exts[] = {"exe", "flv", "mp4", "mp3", "rar", "zip"};
//		char *exts[] = {"flv"};
		for (i = 0; i < sizeof(exts)/sizeof(exts[0]); i++) 
			if (strstr(payload, exts[i])) {
				get = 1;
				*s = '\r';
			}
	}
	void *h1 = hash(srcip, dstip, srcport, dstport);
	void *h2 = hash(dstip, srcip, dstport, srcport);
	void *r1 = g_hash_table_lookup(ht, h1);
	if (!r1 && get && !to_me) {
#if 1
		PyObject *r = PyObject_CallFunction(py_check_request, "s#k", payload, size_payload, ack);
		py_assert(r);
		if (r != Py_None) {
			g_hash_table_insert(ht, h1, r);
			g_hash_table_insert(ht, h2, r);
		}
#endif
	}
	if (r1) {
		io_bytes += size_payload;
		PyObject *r = PyObject_CallFunction(py_process_packet, "Os#k", r1, payload, size_payload, seq);
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

#if 0
	if (get) {
		PyObject *r = PyObject_CallFunction(pyfunc, "kkkkiiks#ii", 
				srcip, dstip, srcport, dstport, seq, ack, tcpflags, 
				payload, size_payload, get, tome
				);
		if (r)
			Py_DECREF(r);
		else {
			PyErr_Print();
			exit(1);
		}
	}
#endif 
	if (1) {
		packet_nr++;
		if (!(packet_nr % 40000)) {
			fprintf(stderr, "%d packets %.2fM io_bytes %.2fM ht %d\n", 
					packet_nr, totsize*1.0/1024/1024, io_bytes*1.0/1024/1024,
					g_hash_table_size(ht)
					);
			totsize = 0;
			io_bytes = 0;
		}
		totsize += size_payload;
		return;
	}
}

static void process_pcap(u_char *args, const struct pcap_pkthdr *hdr, const u_char *p)
{
	xcache_process_packet(p);
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
