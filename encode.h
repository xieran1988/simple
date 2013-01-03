#include <stdio.h>

void *h264_new(int w, int h);
void h264_encode(void *h, void **data, int *linesize, void **buf, int *size);

