#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

static char buf[64];

int main(int argc, char **argv)
{
	FILE *fp;
	//unsigned long sequence = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return -1;
	}

	fp = fopen(argv[1], "r");

	while (fgets(buf, 64-1, fp) != NULL) {
		uint64_t offset;
		uint64_t len;
		char type;

		if (isspace(buf[0]) || buf[0] == '#')
			continue;

		sscanf(buf, "%c %lu %lu", &type, &offset, &len);

		printf("%lu %lu %d 0\n",
			offset, len, type == 'R' ? 0 : 1);
	}
	if (ferror(fp))
		perror("fgets");

	fclose(fp);

	return 0;
}

