#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int clen;
	if (argc != 4)
		return 1;
	FILE *fp = fopen(argv[1], "r");
	FILE *fstat= fopen(argv[2], "w+");
	FILE *fholes = fopen(argv[3], "w+");
	if (fscanf(fp, "%d", &clen) == EOF)
		return 1;
	printf("clen=%d\n", clen);
	char *s = (char *)malloc(clen);
	memset(s, 0, clen);
	static int start, len, err, tot, empty;
	int i;
	while (fscanf(fp, "%d%d", &start, &len) != EOF) {
		if (start + len <= clen) {
			tot += len;
			for (i = start; i < start + len; i++)
				s[i]++;
		} else {
			err++;
			fprintf(fholes, "(E) %d %d\n", start, len);
		}
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
	fprintf(fstat, "%.2lf%%", (tot-empty)*100./tot);
	if (err) 
		fprintf(fstat, " (%d E)\n", err);
	fclose(fp);
	fclose(fstat);
	fclose(fholes);

	return 0;
}

