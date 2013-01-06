#include "decode.h"
#include "encode.h"

int main()
{
	void *m = mp4_open("/Users/xieran/1.mp4");
	void *h = h264_new(mp4_width(m), mp4_height(m));
	void *data[3];
	int line[3];
	void *buf;
	int len;

	mp4_read_frame(m, data, line);
	int i;
	for (i = 0; i < 20; i++)
		h264_encode(h, data, line, &buf, &len);

	return 0;
}

