
#include "a.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <av/util/a.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/dict.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	void *data[3];
	int line[3];
	void *sample[2];
	int cnt;

	if (argc != 2) 
		return 0;

	int sel = -1;
	sscanf(argv[1], "%d", &sel);

	if (sel == 1) {
		void *dec;
		void *enc;

		mp4dec_loglevel(1);

		dec = mp4dec_open("/vid/1.mp4");
		if (!dec)
			return 0;
		int w = mp4dec_width(dec);
		int h = mp4dec_height(dec);
		printf("w=%d, h=%d\n", w, h);
		enc = mp4enc_openfile("/tmp/out.mp4", w, h);
		if (!enc)
			return 0;
		int n = 200;
		while (n--) {
			printf("copy %d\n", n);
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

	if (sel == 3) {
		void *dec, *dec2;
		void *enc;

		mp4enc_loglevel(1);

		dec = mp4dec_open("/vid/1.mp4");
		if (!dec)
			return 0;

		dec2 = mp4dec_open("/vid/2.mp4");
		if (!dec2)
			return 0;

		int w = mp4dec_width(dec);
		int h = mp4dec_height(dec);
		printf("w=%d, h=%d\n", w, h);

		enc = mp4enc_openrtmp("rtmp://localhost/myapp/1", w, h);
		if (!enc)
			return 0;

		int i, j;
		i = 0;
		while (1) {

			while (1) {
				float vd, ad;

				mp4enc_getdelta(enc, &vd, &ad);
				if (vd > 2)
					break;

				printf("write %d\n", i);
				for (j = 0; j < 10; j++) {
					if (!mp4dec_read_frame(dec, data, line, sample, &cnt)) 
						break;
				}
				if (j == 10)
					return 0;
				mp4enc_write_frame_rtmp(enc, data, line, sample, cnt);
				i++;
			}
			fsleep(0.5);

		}
		mp4enc_close(enc);
	}

	if (sel == 4) {
		mp4dec_loglevel(1);

		void *data[3];
		int line[3];
		sample_yuv(320, 240, data, line, 44);

		void *dec = mp4dec_open("/vid/1.mp3");
		void *enc = mp4enc_openfile("/tmp/out.mp4", 320, 240);

		while (1) {
			void *sample[2];
			int cnt;
			if (mp4dec_read_frame(dec, NULL, NULL, sample, &cnt))
				break;
			mp4enc_write_frame(enc, data, line, sample, cnt);
		}
		mp4enc_close(enc);
	}

	if (sel == 5) {

	}

	return 0;
}

