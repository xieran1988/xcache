#ifdef _TEST_
#include <stdio.h>
int main()
{
	int s, e, hl;
	char path[1024];
	FILE *f = fopen((char *)getenv("RS"), "r");
	int rs = atoi(getenv("crs"));
	int re = atoi(getenv("cre"));
	int clen = atoi(getenv("clen"));
	int from = rs;
	printf("range [%d,%d]\n", rs, re);
	while (~fscanf(f, "%d%d%s%d", &s, &e, path, &hl)) {
		printf("[%d,%d] ", s, e);
		if (s <= from && from <= e && from <= re) {
			int to = e < re ? e : re;
			printf("write [%d,%d]\n", from, to);
			from = to + 1;
		} else {
			printf("skip\n");
		}
	}
	if (from == re + 1) {
		printf("ok\n");
	} else {
		printf("lost\n");
	}
	return 0;
}
#endif
