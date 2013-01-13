#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <av/encdec/a.h>
#include <av/util/a.h>
#include <gl/util/a.h>

void *yuvtex_new(int w, int h);
void yuvtex_bind(void *m, void **data, int *line);
void yuvtex_unbind();

GLuint load_rgb_tex(char *filename, int w, int h);
GLuint sample_rgb_tex(int w, int h);
GLuint empty_rgb_tex(int w, int h);
GLuint load_frag_shader(char *filename);

GLuint fontex_new(wchar_t *s, int h);

void *fbotex_new(int w, int h);
void fbotex_render_start(void *_m);
void fbotex_render_end(void *_m);
int fbotex_w(void *_m);
int fbotex_h(void *_m);
GLuint fbotex_tex(void *_m);
GLuint fbotex_tex2(void *_m);
void fbotex_getyuv(void *_m, void **data, int *line);
void fbotex_set(void *_m, char *fmt, ...);

void *mmap_file(char *filename, int *len);
void munmap_file(void *p, int len);
void dump_file(char *filename, void *data, int len);

void all_init(int w, int h);
void all_render();
void all_ctrl(char *fmt, ...);

