#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>


static char buf[512*1024*1024], *pt = buf;

#define QBUFSIZE 32*1024*1024
#define QSIZE 8
typedef struct {
	char buf[QBUFSIZE];
	int len;
} que_t ;
static que_t que[QSIZE];
int qt, qh, ql;
static sem_t sem;

void xcache_init();
void xcache_process_packet(void *, int);

static int mode;

static void *thread()
{
	while (!sem_wait(&sem)) {
		while (qh < qt) {
			printf("qh %d qt %d\n", qh, qt);
			que_t *q = &que[qh%QSIZE];
			char *p = q->buf;
			while (p < q->buf + q->len) {
				int len = *(int *)p;
				xcache_process_packet(p + sizeof(int), len);
				p += sizeof(int) + len;
			}
			q->len = 0;
			ql--;
			qh++;
		}
	}
}

static pthread_t th;

void xinit()
{
	if (getenv("mode")) 
		mode = atoi(getenv("mode"));
	
	switch (mode) {
	case 0: printf("mode=live\n"); break;
	case 1: printf("mode=dump\n"); break;
	case 2: printf("mode=queue\n"); break;
	default: break;
	}

	xcache_init();

	if (mode == 2) {
		sem_init(&sem, 0, 0);
		pthread_create(&th, NULL, thread, NULL);
	}
}

int xproc(void *p, int len)
{
	if (mode == 0)
		xcache_process_packet(p, len);

	if (mode == 1) {
		if (pt - buf > sizeof(buf)-1024*1024)
			return 1;
		if (len > 2000) {
			printf("error %d\n", len);
			exit(1);
		}
		*(int *)pt = len;
		memcpy(pt + sizeof(int), p, len);
		pt += sizeof(int) + len;
	}

	if (mode == 2) {
		if (ql == QSIZE) {
			printf("queue full\n");
			exit(1);
		}
		que_t *q = &que[qt%QSIZE];
		if (q->len > QBUFSIZE - 4096) {
			//printf("qt=%d\n", qt);
			qt++;
			ql++;
			sem_post(&sem);
		} else {
			int *pt = (int *)&q->buf[q->len];
			*pt = len;
			memcpy(pt + 1, p, len);
			q->len += sizeof(int) + len;
		}
	}

	return 0;
}

void xexit()
{
	if (mode == 1) {
		FILE *fp = fopen("/root/out.scap", "wb+");
		fwrite(buf, 1, pt - buf, fp);
		fflush(fp);
		fclose(fp);
	}
}

