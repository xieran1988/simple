#include "a.h"
#include <stdint.h>
#include <stdlib.h>

void sample_yuv(int w, int h, void **data, int *line, int factor)
{
	int i, j;

	for (i = 0; i < 3; i++) 
		data[i] = malloc(w*h);
	line[0] = w;
	line[1] = w/2;
	line[2] = w/2;

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			*((uint8_t*)data[0] + i*w + j) = factor;
			*((uint8_t*)data[1] + i*(w/2) + j/2) = factor;
			*((uint8_t*)data[2] + i*(w/2) + j/2) = factor;
		}
	}
}
	

