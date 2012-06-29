#include <pcap.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

static int cap;
static int direct;

void xinit();
int xproc(void *, int);
void xexit();

static void process_pcap(
		uint8_t *args, 
		const struct pcap_pkthdr *hdr, 
		const uint8_t *p
		)
{
	if (direct)
		xcache_process_packet((uint8_t *)p);
	else if (xproc((void *)p, hdr->len)) {
		xexit();
		exit(0);
	}
}

void raw_loop();

int main(int argc, char *argv[])
{
	pcap_t *p;
	struct bpf_program fp;

	if (argc > 1 && strstr(argv[1], ".scap")) {
		FILE *fp = fopen(argv[1], "rb");
		int len;
		static char packet[1*1024*1024];
		xcache_init();
		printf("replay %s\n", argv[1]);
		while (fread(&len, 1, sizeof(len), fp) == sizeof(len)) {
			fread(packet, 1, len, fp);
			xcache_process_packet(packet);
		}
		return 0;
	}

	if (argc > 1 && strstr(argv[1], ".pcap")) {
		xcache_init();
		printf("replay %s\n", argv[1]);
		p = pcap_open_offline(argv[1], NULL);
		direct = 1;
		pcap_loop(p, -1, process_pcap, NULL);
		return 0;
	}

	if (getenv("cap"))
		cap = atoi(getenv("cap"));

	if (cap == 0) {
		printf("capture=pcap\n");
		xinit();
		p = pcap_open_live("eth1", BUFSIZ, 1, 1000, NULL);
		pcap_compile(p, &fp, "port 80", 0, 0);
		pcap_setfilter(p, &fp);
		pcap_loop(p, -1, process_pcap, NULL);
		return 1;
	}

	if (cap == 1) {
		printf("capture=raw\n");
		xinit();
		raw_loop();
		xexit();
	}

	printf("unknown mode\n");

	return 1;
}

