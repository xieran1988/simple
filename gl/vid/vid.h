#pragma once

void *video_new(int w, int h);
void video_render_start(void *v, void **data, int *line);
void video_render_end(void *v);

