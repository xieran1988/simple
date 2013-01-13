
#include <av/encdec/a.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "a.h"

int main(int argc, char *argv[]) 
{
	if (argc != 3) {
		printf("Usage: %s intput.mp4 output.png\n", argv[0]);
		printf("  generate thumbnail\n");
		return 0;
	}

	char *input = argv[1];
	char *output = argv[2];
	int nr = 6;
	int i;

	void *m = mp4dec_open(input);
//	mp4dec_loglevel(1);

	if (!m) 
		return 1;

	int w = mp4dec_width(m);
	int h = mp4dec_height(m);
	float dur = mp4dec_dur(m);
	float step = dur/(nr+1);
	void *data[3];
	int line[3];

	static char cmd[65536];

	strcat(cmd, "montage ");

	printf("size: %dx%d\n", w, h);
	printf("nr: %d\n", nr);
	printf("dur: %f\n", dur);

	for (i = 0; i < nr; i++) {
		mp4dec_seek_precise(m, i*step);
		printf("seek: %f\n", mp4dec_pos(m));
		if (!mp4dec_read_frame(m, data, line, NULL, NULL)) {
			char path[256];
			snprintf(path, sizeof(path)-1, "%s%d.jpg", output, i);
			printf("pos: %f %s\n", mp4dec_pos(m), path);
			yuv2jpg(path, w, h, data, line);
			strcat(cmd, path);
			strcat(cmd, " ");
		}
	}

	strcat(cmd, "-geometry +1+0 -tile 6x1 -resize 800x70 ");	
	strcat(cmd, output);
	system(cmd);

	for (i = 0; i < nr; i++) {
		char path[256];
		snprintf(path, sizeof(path)-1, "%s%d.jpg", output, i);
		unlink(path);
	}

	return 0;
}

