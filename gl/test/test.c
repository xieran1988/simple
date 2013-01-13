#include "a.h"

static void display()
{
	all_render();
	glutSwapBuffers();
}

static void idle()
{
	GLuint tm = glutGet(GLUT_ELAPSED_TIME);
	static GLuint lasttm, lasttm2, fps;

	if (tm - lasttm > 1000./30) {
		glutPostRedisplay();
		fps++;
		lasttm = tm;
	}
	if (tm - lasttm2 > 1000) {
		printf("fps: %d tm: %.2f\n", fps, tm/1000.);
		fps = 0;
		lasttm2 = tm;
	}
}

static void keyboard_cb(unsigned char key, int x, int y)
{
	char s = key;
	all_ctrl(&s);
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

	int w = 640;
	int h = 360;

	glutInitWindowSize(w, h);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("Pixies");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard_cb);

	all_init(w, h);

	glutMainLoop();

	return 0;
}

