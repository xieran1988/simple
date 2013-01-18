#include "a.h"

int main(int argc, char *argv[])
{
	char *path = "/tmp/out.png";

	int w = 320, h = 240;
	uint8_t *data = (uint8_t *)malloc(w*h*4);

	void *samples[2];
	int cnt;

	void *dec = mp4dec_open("/vid/1.aac");
	float dur = mp4dec_dur(dec);
	printf("dur: %.2f\n", dur);

	int n;
	mp4dec_read_frame(dec, NULL, NULL, samples, &cnt);

	return 0;
}

