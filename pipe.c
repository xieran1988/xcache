#include <stdio.h>
#include <unistd.h>
#include <linux/fcntl.h>

int main()
{
	int a[2], b[2], i, r;
	char c;

	pipe(a);
	pipe2(b, O_NONBLOCK);
	if (!fork()) {
		for (i = 0; i < 1; i++) {
			r = read(a[0], &c, 1);
			printf("r=%c\n", c);
		}
		write(b[1], "k", 1);
		sleep(1000);
		printf("c exit\n");
		return 0;
	}
	write(a[1], "a", 1);
	write(a[1], "b", 1);
	sleep(1);
	read(b[0], &c, 1);
	printf("pr=%c\n", c);
	read(b[0], &c, 1);

	return 0;
}

