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
	printf("idx %d\n", ifr.ifr_ifindex);
	sa.sll_family = PF_PACKET;
	sa.sll_protocol = htons(ETH_P_ALL);
	sa.sll_ifindex = ifr.ifr_ifindex;
	bind(fd, (struct sockaddr *)&sa, sizeof(sa));
	req.tp_block_size = 1024*1024*8;
	req.tp_frame_size = 2048;
	req.tp_block_nr = 1;
	req.tp_frame_nr = req.tp_block_size*req.tp_block_nr/req.tp_frame_size;
	setsockopt(fd, SOL_PACKET, PACKET_RX_RING, (char *)&req, sizeof(req));
	p = mmap(0, 1024*1024*8, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	while (1) {
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
	}

	return 0;
}

