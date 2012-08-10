#include <sys/inotify.h>
#include <stdio.h>

int main()
{
	struct inotify_event *ie;
	char buf[4096];
	int fd = inotify_init();
	inotify_add_watch(fd, (char *)getenv("dir"), IN_MOVED_TO);
	setbuf(stdout, NULL);
	while (1) {
		read(fd, buf, sizeof(buf));
		ie = (typeof(ie))buf;
		printf("%s\n", ie->name);
	}
	return 0;
}

