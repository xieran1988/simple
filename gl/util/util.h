#pragma once

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

void test(int w, int h, void (*_init_)(), void (*_disp_)());
void draw_quads(float x, float y, float z, float w, float h);

