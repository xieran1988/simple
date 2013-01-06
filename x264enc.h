#include <stdio.h>

void *x264enc_new(int w, int h);
void x264enc_encode(void *h, void **data, int *linesize, void **buf, int *size);

