#pragma once

void *mp4enc_new(int w, int h);
void mp4enc_put_frame(void **data, int *linesize, void *audiodata, int audiolen);

