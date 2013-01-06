#pragma once

void *mp4enc_new(int w, int h);
void mp4enc_put_frame(void *h264, int h264_len, void *aac, int aac_len);

