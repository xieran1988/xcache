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

PyObject *py_check_request;
PyObject *py_check_finish;
PyObject *py_process_packet;
PyObject *py_mod;
u_char mymac[6];
int packet_nr, totsize;
GHashTable *ht;

void *hash(u_int srcip, u_int dstip, u_int srcport, u_int dstport) 
{
	return (void *)(srcip + dstip*123 + srcport*456 + dstport*789);
}

void py_assert(void *p)
{
	if (!p) {
		PyErr_Print();
		exit(1);
	}
}

void process(u_char *args, const struct pcap_pkthdr *hdr, const u_char *p)
{
	u_char *payload, *ip, *tcp, tcpflags;
	u_int size_ip, size_tcp, totlen;
	int size_payload, get = 0, to_me = 0;
	u_int dstip, srcip, dstport, srcport, seq, ack;

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
		for (i = 0; i < sizeof(exts)/sizeof(exts[0]); i++) 
			if (strstr(payload, exts[i])) {
				get = 1;
				*s = '\r';
			}
	}
	void *r1 = g_hash_table_lookup(ht, hash(srcip, dstip, srcport, dstport));
	void *r2 = g_hash_table_lookup(ht, hash(dstip, srcip, dstport, srcport));
	if (!r1 && get && !to_me) {
		PyObject *r = PyObject_CallFunction(py_check_request, "s#k", payload, size_payload, ack);
		py_assert(r);
		if (r != Py_None) 
			g_hash_table_insert(ht, hash(srcip, dstip, srcport, dstport), r);
	}
	if (r2) {
		PyObject *r = PyObject_CallFunction(py_process_packet, "Os#k", r2, payload, size_payload, seq);
		py_assert(r);
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
			fprintf(stderr, "%d packets %.2fM\n", packet_nr, totsize*1.0/1024/1024);
			totsize = 0;
		}
		totsize += size_payload;
	}
}

int main(int argc, char *argv[])
{
	pcap_t *p;
	struct bpf_program fp;
	struct ifreq ifr;

	ht = g_hash_table_new(g_direct_hash, g_direct_equal);

	Py_Initialize();
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('/usr/lib/xcache')");
	py_mod = PyImport_ImportModule("cap");
	py_assert(py_mod);
	py_check_request = PyObject_GetAttrString(py_mod, "check_request");
	py_process_packet = PyObject_GetAttrString(py_mod, "process_packet");
	py_check_finish = PyObject_GetAttrString(py_mod, "check_finish");

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
	pcap_loop(p, -1, process, NULL);

	return 0;
}

