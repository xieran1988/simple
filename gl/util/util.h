#pragma once

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

void test(int w, int h, void (*_init_)(), void (*_disp_)());
void gl_draw_quads(float x, float y, float z, float w, float h);
void gl_draw_quads_tex(float x, float y, float z, 
		float w, float h, float texw, float texh);
void gl_checkerr(char *msg);

