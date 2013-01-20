#pragma once

#include <stdint.h>

void mp4enc_loglevel(int lev);

void *mp4enc_openfile(char *filename, int w, int h);
void mp4enc_write_frame(void *m, void **yuv, int *linesize, void **sample, int cnt);

void *mp4enc_opents(int w, int h);

void *mp4enc_openrtmp(char *filename, int w, int h);
void mp4enc_write_frame_rtmp(void *m, void **yuv, int *linesize, void **sample, int cnt);
void mp4enc_getdelta(void *m, float *vpos, float *apos);

void *mp4enc_openhls(char *dir, int w, int h, float dur);

void mp4enc_close(void *m);

