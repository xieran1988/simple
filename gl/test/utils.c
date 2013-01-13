#include "a.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *mmap_file(char *filename, int *len)
{
	int fd;
	struct stat sb;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return NULL;
	if (fstat(fd, &sb) == -1)
		return NULL;

	*len = (int)sb.st_size;
	void *addr = mmap(NULL, sb.st_size, PROT_READ,
			MAP_PRIVATE, fd, 0);
	close(fd);

	return addr;
}

void munmap_file(void *p, int len)
{
	munmap(p, len);
}

void dump_file(char *filename, void *data, int len)
{
	FILE *fp = fopen("/tmp/t", "wb+");
	if (fp) {
		fwrite(data, 1, len, fp);
		fclose(fp);
		rename("/tmp/t", filename);
	}
}

