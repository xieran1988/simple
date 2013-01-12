
#include "a.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main(int argc, char *argv[])
{
	void *data[3];
	int line[3];
	void *sample[2];
	int cnt;

	mp4dec_loglevel(1);
	mp4enc_loglevel(1);

	if (argc != 2) 
		return ;

	int sel = -1;
	sscanf(argv[1], "%d", &sel);

	if (sel == 1) {
		void *dec;
		void *enc;

		dec = mp4dec_open("/vid/1.mp4");
		if (!dec)
			return 0;
		int w = mp4dec_width(dec);
		int h = mp4dec_height(dec);
		enc = mp4enc_openfile("/tmp/out.mp4", w, h);
		if (!enc)
			return 0;
		int n = 2500;
		while (n--) {
			if (mp4dec_read_frame(dec, data, line, sample, &cnt)) 
				break;
			mp4enc_write_frame(enc, data, line, sample, cnt);
		}
		mp4enc_close(enc);
	}

	if (sel == 2) {
		void *dec[4];
		void *enc;
		int i;

		mp4dec_loglevel(0);
		mp4enc_loglevel(0);
	
		for (i = 0; i < 4; i++) {
			char path[256];
			sprintf(path, "/vid/%d.mp4", i+1);
			dec[i] = mp4dec_open(path);
			if (!dec[i])
				return 0;
			enc = mp4enc_openfile("/tmp/out.mp4", 640, 360);
			if (!enc)
				return 0;
		}
		for (i = 0; i < 400; i++) {
			int j = rand() % 100;
			int k = rand() % 4;
			int n;
			mp4dec_seek_precise(dec[k], j);
			printf("i=%d j=%d k=%d\n", i, j, k);
			for (n = 0; n < 4; n++) {
				if (!mp4dec_read_frame(dec[k], data, line, sample, &cnt)) {
					printf("  cnt %d\n", cnt);
					if (cnt == 0)
						return 1;
					mp4enc_write_frame(enc, data, line, sample, cnt);
				} else 
					printf("  fail\n");
			}
		}
		mp4enc_close(enc);
	}

	return 0;
}
