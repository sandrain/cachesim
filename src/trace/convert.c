#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

static char buf[64];

int main(int argc, char **argv)
{
	FILE *fp;
	double time = 0.05;
	//unsigned long sequence = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return -1;
	}

	fp = fopen(argv[1], "r");

	while (fgets(buf, 64-1, fp) != NULL) {
		uint64_t offset, count, tmp1, tmp2;

		sscanf(buf, "%lu %lu %lu %lu", &offset, &count, &tmp1, &tmp2);
		printf("0,%lu,%lu,r,%.2f\n", offset, count*512, time);
		time += 0.05;
#if 0
		uint64_t offset;
		uint64_t len;
		char type;

		if (isspace(buf[0]) || buf[0] == '#')
			continue;

		sscanf(buf, "%c %lu %lu", &type, &offset, &len);

		printf("%lu %lu %c 0\n",
			offset, len, type);
#endif
	}
	if (ferror(fp))
		perror("fgets");

	fclose(fp);

	return 0;
}

