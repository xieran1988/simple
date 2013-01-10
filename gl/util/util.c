#include "util.h"
#include <stdio.h>

static int win_w, win_h;
static void (*_disp)();
static void (*_init)();

static void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glDepthRange(0, 1);

	if (_init)
		_init();
}

void gl_draw_quads(float x, float y, float z, float w, float h)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(x, y-h, z);
		glTexCoord2f(0, 1); glVertex3f(x, y, z);
		glTexCoord2f(1, 1); glVertex3f(x+w, y, z);
		glTexCoord2f(1, 0); glVertex3f(x+w, y-h, z);
	glEnd();
}

void gl_checkerr() 
{
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = gluErrorString(errCode);
		fprintf (stderr, "OpenGL Error: %s\n", errString);
	}
}

static void display()
{
	glViewport(0, 0, win_w, win_h);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	_disp();
	glutSwapBuffers();
}

static void idle()
{
	glutPostRedisplay();
}

static void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	win_w = w;
	win_h = h;
}

static void keyboard_cb(unsigned char key, int x, int y)
{
	switch (key) {
		case GLUT_KEY_LEFT:
			break;
		case GLUT_KEY_RIGHT:
			break;
	}
}

void test(int w, int h, void (*_init_)(), void (*_disp_)())
{
	int argc = 0;
	char *argv;

	glutInit(&argc, &argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

	win_w = w;
	win_h = h;
	_init = _init_;
	_disp = _disp_;

	glutInitWindowSize(win_w, win_h);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("Pixies");

	printf("test %d %d\n", w, h);

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard_cb);

	init();

	glutMainLoop();
}

