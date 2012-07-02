#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int clen;
	static FILE *fp, *fstat, *fholes;

	if (getenv("seq2")) {
		if (argc != 2) {
			printf("argc!=2\n");
			return 1;
		}
		fp = fopen(argv[1], "r");
	} else {
		if (argc != 4)
			return 1;
		fp = fopen(argv[1], "r");
		fstat= fopen(argv[2], "w+");
		fholes = fopen(argv[3], "w+");
	}

	if (!fp)
		return 1;
	printf("open %s\n", argv[1]);

	if (fscanf(fp, "%d", &clen) == EOF) {
		return 1;
	}
	if (clen < 0) {
		return 1;
		if (getenv("seq2")) {
//			printf("tot rst\n");
			return 1;
		}
	}
	printf("clen %d\n", clen);

	char *s = (char *)malloc(clen);
	memset(s, 0, clen);
	static int start, len, err, tot, empty, rst;
	int i;
	while (fscanf(fp, "%d%d", &start, &len) != EOF) {
		if (start == -1 && len == -1) {
			rst = 1;
			break;
		}
		//printf("%d %d\n", start, len);
		if (start + len <= clen) {
			tot += len;
			for (i = start; i < start + len; i++)
				s[i]++;
		} else {
			err++;
			if (fholes)
				fprintf(fholes, "(E) %d %d\n", start, len);
		}
	}
	for (i = 0; i < clen; i++) {
		if (s[i] != 1) {
			int ss = i;
			int p = s[i];
			while (s[i] == p && i < clen)
				i++;
			if (fholes)
				fprintf(fholes, "(%d) %d %d\n", p, ss, i-ss);
			if (p == 0)
				empty += i-ss;
			i--;
		}
	}
	
	if (getenv("seq2")) {
		printf("tot %s %d %d %.2lf%%\n", rst ? "rst" : "non-rst", clen, empty, (clen-empty)*100./clen);
	}

	if (fstat) {
		fprintf(fstat, "%.2lf%%", (clen-empty)*100./clen);
		if (err) 
			fprintf(fstat, " (%d E)\n", err);
		fclose(fstat);
	}
	fclose(fp);
	if (fholes)
		fclose(fholes);

	return 0;
}

