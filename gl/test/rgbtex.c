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

GLuint empty_rgb_tex(int w, int h)
{
	GLuint tex;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 
			0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	return tex;
}

GLuint load_rgb_tex(char *filename, int w, int h)
{
	GLuint tex;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	int len;
	void *data = mmap_file(filename, &len);
	if (!data) {
		printf("load_rgb_tex: %s not found\n", filename);
		return 0;
	}
	if (len < w*h*3) {
		printf("load_rgb_tex: size too small %d\n", len);
		return 0;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 
			0, GL_RGB, GL_UNSIGNED_BYTE, data);
	munmap_file(data, len);

	return tex;
}

