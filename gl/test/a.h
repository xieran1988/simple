#pragma once

#include <gl/util/util.h>

void *yuvtex_new(int w, int h);
void yuvtex_bind(void *m, void **data, int *line);
void yuvtex_unbind();

GLuint load_rgb_tex(char *filename, int w, int h);
GLuint empty_rgb_tex(int w, int h);

GLuint fontex_new(wchar_t *s, int h);

