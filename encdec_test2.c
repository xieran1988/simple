#include <stdio.h>
#include "mp4enc.h"
#include "mp4dec.h"

int main()
{
	void *enc, *dec, *dec2;
	void *yuv[3];
	int linesize[3];
	void *sample;
	int cnt;
	int i, j;
 	
	mp4enc_loglevel(1);
	mp4dec_loglevel(1);

	enc = mp4enc_openfile(640, 360, "/tmp/out.mp4");
	dec = mp4dec_open("/vid/1.mp4");
	dec2 = mp4dec_open("/vid/1.mp4");

	mp4dec_seek_precise(dec, 15.4);
	mp4dec_seek_precise(dec2, 30.4);

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 24; j++) {
			mp4dec_read_frame(dec, yuv, linesize, &sample, &cnt);
			mp4enc_write_frame(enc, yuv, linesize, sample, cnt);
		}
	}

	return 0;
}

