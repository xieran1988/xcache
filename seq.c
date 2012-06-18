#include <stdio.h>

int main(int argc, char *argv[])
{
	int clen;
	if (argc != 1)
		return 1;
	FILE *fp = fopen("/tmp/seq", "r");
	fscanf(fp, "%d", &clen);
	printf("clen=%d\n", clen);
	char *s = (char *)malloc(clen);
	memset(s, 0, clen);
	int start, len, tot = 0;
	int i;
	while (fscanf(fp, "%d%d", &start, &len) != EOF) {
		tot += len;
		for (i = start; i < start + len; i++)
			s[i]++;
	}
	printf("tot=%d\n", tot);
	for (i = 0; i < clen; i++) {
		if (s[i] != 1) {
			int ss = i;
			int p = s[i];
			while (s[i] == p)
				i++;
			printf("(%d) %d %d\n", p, ss, i-ss);
			i--;
		}
	}

	return 0;
}

