#include "a.h"
#include <av/encdec/a.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	char *path = "/tmp/out.png";

	int w = 120, h = 40;
	uint32_t *data = (uint32_t *)malloc(w*h*4);
	memset(data, 0, w*h*4);

	float *samples[2];
	int cnt;

	void *dec = mp4dec_open("/vid/1.aac");

	mp4dec_set(dec, "novideo");

	float dur = 0;
	float max = -1e6, min = 1e6, v;
	while (1) {
		dur = mp4dec_pos(dec);
		if (mp4dec_read_frame(dec, NULL, NULL, (void **)samples, &cnt))
			break;
		if (cnt) {
			v = samples[0][0];
			if (v < min)
				min = v;
			if (v > max)
				max = v;
		}
	}

	mp4dec_seek_precise(dec, 0);
	printf("dur: %.2f [%.2f,%.2f]\n", dur, min, max);

	int x, y, i;
	while (1) {
		float pos = mp4dec_pos(dec);
		if (mp4dec_read_frame(dec, NULL, NULL, (void **)samples, &cnt))
			break;
		if (cnt) {
			v = samples[0][0];
			x = w*pos/dur;
			float fh = (v-min)/(max-min);
			y = h*fh;
			//printf("x,y=%d,%d pos,dur=%.2f/%.2f\n", x, y, pos, dur);
			for (i = y; i < h; i++)
				data[i*w+x] = 0xffffffff;
		}
	}

	FILE *fp = fopen("/tmp/w.rgba", "wb+");
	fwrite(data, 1, w*h*4, fp);
	fclose(fp);

	char cmd[1024];
	sprintf(cmd, "convert -depth 8 -size %dx%d /tmp/w.rgba /tmp/w.png", w, h);
	system(cmd);

	return 0;
}

