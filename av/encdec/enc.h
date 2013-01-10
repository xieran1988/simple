#pragma once

void *mp4enc_openfile(char *filename, int w, int h);
void mp4enc_loglevel(int lev);
void mp4enc_write_frame(void *m, void **yuv, int *linesize, void **sample, int cnt);
void mp4enc_close(void *m);

