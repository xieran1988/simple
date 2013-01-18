#pragma once 

int yuv2jpg(char *path, int w, int h, void **data, int *line);
void sample_yuv(int w, int h, void **data, int *line, int factor);

int rgb2png(char *path, int w, int h, void *data);

float tm_elapsed();
void fsleep(float);

