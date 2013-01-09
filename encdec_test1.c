
#include "mp4dec.h"
#include "x264enc.h"

int main(int argc, char *argv[])
{
	void *dec;
	void *enc;
	void *data[3];
	int line[3];
	void *buf;
	int len;
	void *sample;
	int cnt;
	int i;
	
	mp4dec_loglevel(1);

	dec = mp4dec_open(argv[1]);

	mp4dec_seek_precise(dec, 0.0);
	for (i = 0; i < 100; i++) {
		mp4dec_read_frame(dec, data, line, &sample, &cnt);
		printf("samp: %d\n", cnt);
	}

	/*
	enc = x264enc_new(mp4dec_width(dec), mp4dec_height(dec));
	int i;
	for (i = 0; i < 20; i++)
		x264enc_encode(enc, data, line, &buf, &len);
		*/

	return 0;
}

