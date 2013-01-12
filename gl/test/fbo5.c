#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <av/encdec/a.h>
#include <av/util/a.h>

#include "a.h"

int win_w, win_h;

void *yuvtex, *fbotex;

#define VIDEO_W 640
#define VIDEO_H 360

#define FBO_W 128
#define FBO_H 128

static GLuint rgbtex, pictex;
static GLuint mrtprog;

void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glDepthRange(0, 1);
	glDepthFunc(GL_LEQUAL);
	
	yuvtex = yuvtex_new(VIDEO_W, VIDEO_H);
	fbotex = fbotex_new(FBO_W, FBO_H);
	rgbtex = sample_rgb_tex(64, 64);
	pictex = load_rgb_tex("128x128.rgb", FBO_W, FBO_H);
}

static void dump(char *name, void *data, int len)
{
	FILE *fp = fopen("/tmp/t", "wb+");
	fwrite(data, 1, len, fp);
	fclose(fp);
	rename("/tmp/t", name);
}

void display()
{
	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-0.5, 0.5, -0.5, 0.5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D, rgbtex);

	glLoadIdentity();
	glScalef(0.1, 0.1, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-2.0, -1.0, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-2.0, 1.0, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(0.0, 1.0, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(0.0, -1.0, 0.0);

	glTexCoord2f(0.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(1.0, 1.0, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(2.41421, 1.0, -1.41421);
	glTexCoord2f(1.0, 0.0); glVertex3f(2.41421, -1.0, -1.41421);
	glEnd();
	glFlush();

	static void *data[3];
	static int line[3];

	if (!data[0]) {
		sample_yuv(VIDEO_W, VIDEO_H, data, line, 53);
	}
	glLoadIdentity();
	yuvtex_bind(yuvtex, data, line);
	gl_draw_quads(-0.3, 0.3, 0.1, 0.3, 0.3);
	yuvtex_unbind();

	fbotex_render_start(fbotex);
	glBindTexture(GL_TEXTURE_2D, pictex);
	gl_draw_quads(0, 0, 0, fbotex_w(fbotex), fbotex_h(fbotex));
	fbotex_render_end(fbotex);

	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, fbotex_tex(fbotex));
	gl_draw_quads(0.1, 0.1, 0, 0.2, 0.2);

	glBindTexture(GL_TEXTURE_2D, fbotex_tex2(fbotex));
	gl_draw_quads(0.1, 0.4, 0, 0.2, 0.2);

	void *data2[3];
	int line2[3];
	fbotex_getyuv(fbotex, data2, line2);
	yuv2jpg("/tmp/c.jpg", FBO_W, FBO_H, data2, line2);

	glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-0.5, 0.5, -0.5, 0.5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	win_w = w;
	win_h = h;
}

void keyboard_cb(unsigned char key, int x, int y)
{
	switch (key) {
		case GLUT_KEY_LEFT:
			break;
		case GLUT_KEY_RIGHT:
			break;
	}
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

	win_w = VIDEO_W;
	win_h = VIDEO_H;

	glutInitWindowSize(win_w, win_h);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("Pixies");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard_cb);

	init();

	glutMainLoop();

	return 0;
}

