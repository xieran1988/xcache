#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <poll.h>

int main(int argc, char *argv[])
{
	int fd;
	struct sockaddr_ll sa;
	struct ifreq ifr;
	struct tpacket_req req;
	int i;
	void *p;
	struct pollfd pfd;

	fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	memset(&sa, 0, sizeof(sa));

	strncpy(ifr.ifr_name, "eth1", sizeof(ifr.ifr_name));
	ioctl(fd, SIOCGIFINDEX, &ifr);
	sa.sll_family = PF_PACKET;
	sa.sll_protocol = htons(ETH_P_ALL);
	sa.sll_ifindex = ifr.ifr_ifindex;
	printf("binding to ifidx %d\n", ifr.ifr_ifindex);
	i = bind(fd, (struct sockaddr *)&sa, sizeof(sa));
	printf("bind=%d\n", i);

	req.tp_block_size = 1024*1024*8;
	req.tp_frame_size = 2048;
	req.tp_block_nr = 1;
	req.tp_frame_nr = req.tp_block_size*req.tp_block_nr/req.tp_frame_size;
//	setsockopt(fd, SOL_PACKET, PACKET_RX_RING, (char *)&req, sizeof(req));
//	p = mmap(0, 1024*1024*8, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	
	struct packet_mreq mr;
	memset(&mr,0,sizeof(mr));
	mr.mr_ifindex = ifr.ifr_ifindex;
	mr.mr_type = PACKET_MR_PROMISC;
	i = setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,&mr,sizeof(mr));
	printf("setsockopt=%d\n", i);

	int n = 0;
	while (1) {
		char buf[8192];
		int len = recv(fd, buf, sizeof(buf), MSG_TRUNC);
		if (len > 1514) {
			printf("error %d\n", len);
			exit(1);
		}
		if (*(unsigned short *)(buf + 12) != 0x8) 
			continue;
		if (buf[23] == 0x84) {
			if (*(unsigned short *)(buf + 20) & 0xff1f) {
				continue;
			} else {
				int x = (buf[14]&0xf)*4;
				printf("%x\n", *(unsigned short *)(buf + 14));
			}
		} 
		n++;
		if (!(n % 150000)) {
			printf("n=%d\n", n);
		}

	//	printf("%d\n", len);
		/*
		pfd.fd = fd;
		pfd.revents = 0;
		pfd.events = POLLIN|POLLRDNORM|POLLERR;
		i = poll(&pfd, 1, -1);
		for (i = 0; ; i++) {
			struct tpacket_hdr *t = (typeof(t))((char *)p + i*req.tp_frame_size);
			if (t->tp_len < 34 || t->tp_len > 1614)
				break;
			//printf("%d,", t->tp_len);
		}
		*/
	}

	return 0;
}

