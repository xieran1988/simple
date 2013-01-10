#include "a.h"

int main()
{
	void *data[3];
	int line[3];
	int w = 192;
	int h = 192;

	sample_yuv(w, h, data, line, 4);
	yuv2jpg("/tmp/out.jpg", w, h, data, line);

	return 0;
}

