#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int clen;
	if (argc != 3)
		return 1;
	FILE *fp = fopen(argv[1], "r");
	FILE *fstat= fopen(argv[2], "w+");
	FILE *fholes = fopen(argv[3], "w+");
	if (fscanf(fp, "%d", &clen) == EOF)
		return 1;
	printf("clen=%d\n", clen);
	char *s = (char *)malloc(clen);
	memset(s, 0, clen);
	int start, len, tot = 0, empty = 0;
	int i;
	while (fscanf(fp, "%d%d", &start, &len) != EOF) {
		tot += len;
		for (i = start; i < start + len; i++)
			s[i]++;
	}
	for (i = 0; i < clen; i++) {
		if (s[i] != 1) {
			int ss = i;
			int p = s[i];
			while (s[i] == p && i < clen)
				i++;
			fprintf(fholes, "(%d) %d %d\n", p, ss, i-ss);
			i--;
			if (p == 0)
				empty += i-ss;
		}
	}
	fprintf(fstat, "%.2lf\n", (tot-empty)*1./tot);
	fclose(fp);
	fclose(fstat);
	fclose(fholes);

	return 0;
}

