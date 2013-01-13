#pragma once 

int yuv2jpg(char *path, int w, int h, void **data, int *line);
void sample_yuv(int w, int h, void **data, int *line, int factor);

float tm_elapsed();

