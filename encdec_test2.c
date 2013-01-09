#include <stdio.h>
#include "mp4enc.h"
#include "mp4dec.h"

int main()
{
	void *enc, *dec, *dec2;
	void *yuv[3];
	int line[3];
	void *sample[2];
	int cnt;
	int i, j;
 	
	mp4enc_loglevel(1);
	mp4dec_loglevel(1);

	enc = mp4enc_openfile(640, 360, "/tmp/out.mp4");
	dec = mp4dec_open("/vid/1.mp4");

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 25; j++) {
			mp4dec_read_frame(dec, yuv, line, sample, &cnt);
			mp4enc_write_frame(enc, yuv, line, sample, cnt);
		}
	}

	mp4enc_close(enc);

	return 0;
}

