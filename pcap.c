#include <pcap.h>
#include <stdio.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <Python.h>

static PyObject *pyfunc, *pymod;
static u_char mymac[6];

void process(u_char *args, const struct pcap_pkthdr *hdr, const u_char *p)
{
	u_char *payload, *ip, *tcp, tcpflags;
	u_int size_ip, size_tcp, totlen;
	int size_payload, get = 0, tome = 0;
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
		tome = 1;
	if (!strncmp("GET", payload, 3)) 
		get = 1;
	if (srcport == 80 || dstport == 80) {
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
}

int main(int argc, char *argv[])
{
	pcap_t *p;
	struct bpf_program fp;
	struct ifreq ifr;
	Py_Initialize();
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('/usr/lib/xcache')");
	pymod = PyImport_ImportModule("cap");
	if (!pymod) {
		PyErr_Print();
		exit(1);
	}
	pyfunc = PyObject_GetAttrString(pymod, "process_packet");
	p = pcap_open_live("eth0", BUFSIZ, 1, 1000, NULL);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(pcap_fileno(p), SIOCGIFHWADDR, &ifr);
	memcpy(mymac, ifr.ifr_hwaddr.sa_data, 6);
	pcap_compile(p, &fp, "tcp", 0, 0);
	pcap_setfilter(p, &fp);
	pcap_loop(p, -1, process, NULL);
	pcap_close(p);
	Py_Finalize();
	return 0;
}

