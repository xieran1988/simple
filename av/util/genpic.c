#include <av/encdec/a.h>
#include <stdio.h>
#include "a.h"

int main(int argc, char *argv[]) 
{
	if (argc != 4) {
		printf("Usage: %s intput.mp4 /tmp/out 4\n", argv[0]);
		printf("  means pick 4 pictures from input.mp4 and \n");
		printf("  save them into /tmp/out0.jpg /tmp/out1.jpg ..\n");
		return 0;
	}

	char *input = argv[1];
	char *output = argv[2];
	int nr = atoi(argv[3]);

	void *m = mp4dec_open(input);
	mp4dec_loglevel(1);

	if (!m) 
		return 1;
	if (nr <= 1)
		return 1;

	int w = mp4dec_width(m);
	int h = mp4dec_height(m);
	float dur = mp4dec_dur(m);
	float step = dur/(nr-1);
	void *data[3];
	int line[3];

	printf("size: %dx%d\n", w, h);
	printf("nr: %d\n", nr);
	int i = 0;
	while (i < nr) {
		mp4dec_seek_precise(m, i*step);
		if (!mp4dec_read_frame(m, data, line, NULL, NULL)) {
			char path[256];
			snprintf(path, sizeof(path)-1, "%s%d.jpg", output, i);
			printf("write %s\n", path);
			yuv2jpg(path, w, h, data, line);
			printf("done\n");
		}
		i++;
	}

	return 0;
}

