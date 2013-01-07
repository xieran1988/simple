#pragma once 

void *mp4dec_open(char *fname);
float mp4dec_dur(void *h);
float mp4dec_pos(void *h);
int mp4dec_height(void *h);
int mp4dec_width(void *h);
void mp4dec_seek_precise(void *h, float pos);
int mp4dec_read_frame(void *h, void **data, int *linesize, void **audio, int *audiocount);

