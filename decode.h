#pragma once 

void *mp4_open(char *fname);
void mp4_data(void **yuv, int *linesize);
float mp4_dur(void *h);
float mp4_pos(void *h);
int mp4_height(void *h);
int mp4_width(void *h);
void mp4_seek(void *h, float pos);
void mp4_seek_precise(void *h, float pos);
int mp4_read_frame(void *h, void **data, int *linesize);

