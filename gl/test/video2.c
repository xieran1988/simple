#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <av/encdec/a.h>
#include <av/util/a.h>

#include "a.h"

#define VIDEO_W 640
#define VIDEO_H 360

#define FBO_W 128
#define FBO_H 128

static GLuint rgbtex, pictex;
void *yuvtex, *fbotex;
void *dec[4], *enc;
int win_w, win_h;

void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glDepthRange(0, 1);
	glDepthFunc(GL_LEQUAL);
	
	yuvtex = yuvtex_new(VIDEO_W, VIDEO_H);
	fbotex = fbotex_new(VIDEO_W, VIDEO_H);
	rgbtex = sample_rgb_tex(64, 64);
	pictex = load_rgb_tex("128x128.rgb", 128, 128);

	dec[0] = mp4dec_open("/vid/1.mp4");
	dec[1] = mp4dec_open("/vid/2.mp4");
	dec[2] = mp4dec_open("/vid/3.mp4");
	dec[3] = mp4dec_open("/vid/4.mp4");
	enc = mp4enc_openfile("/tmp/out.mp4", VIDEO_W, VIDEO_H);
}

static void dump(char *name, void *data, int len)
{
	FILE *fp = fopen("/tmp/t", "wb+");
	fwrite(data, 1, len, fp);
	fclose(fp);
	rename("/tmp/t", name);
}

struct {
	char stat;
	float pos;
	int idx;
	int li, ri;
	float j;
} ani;

struct {
	void *data[3];
	int line[3];
} mov[4];

struct {
	char stat;
} rec;

static int fps, totfps;

void display()
{
	fps++;
	totfps++;

	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-0.5, 0.5, -0.5, 0.5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

	int i;
	for (i = 0; i < 4; i++) {
		if (mp4dec_read_frame(dec[i], mov[i].data, mov[i].line, NULL, NULL)) {
			mp4dec_seek_precise(dec[i], 0);
		}
	}

	if (ani.stat) {
		if (ani.j < 1) {
			if (ani.stat == 'r') {
				ani.pos = sinf(ani.j*3.14/2)*win_w;
				ani.li = (ani.idx+3)%4;
				ani.ri = ani.idx;
			}
			if (ani.stat == 'l') {
				ani.pos = (1-sinf(ani.j*3.14/2))*win_w;
				ani.li = ani.idx;
				ani.ri = (ani.idx+5)%4;
			}
			ani.j += 1./12;
		} else {
			ani.j = 0;
			ani.pos = 0;
			if (ani.stat == 'r') 
				ani.idx = ani.li;
			if (ani.stat == 'l') 
				ani.idx = ani.ri;
			ani.stat = 0;
		}
	}

	fbotex_render_start(fbotex);
	if (ani.stat) {
		yuvtex_bind(yuvtex, mov[ani.li].data, mov[ani.li].line);
		gl_draw_quads(ani.pos-fbotex_w(fbotex), 0, 0, 
				fbotex_w(fbotex), fbotex_h(fbotex));
		yuvtex_unbind();
		yuvtex_bind(yuvtex, mov[ani.ri].data, mov[ani.ri].line);
		gl_draw_quads(ani.pos, 0, 0, 
				fbotex_w(fbotex), fbotex_h(fbotex));
		yuvtex_unbind();
	} else {
		yuvtex_bind(yuvtex, mov[ani.idx].data, mov[ani.idx].line);
		gl_draw_quads(0, 0, 0, fbotex_w(fbotex), fbotex_h(fbotex));
		yuvtex_unbind();
	}
	fbotex_render_end(fbotex);

	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, win_w, 0, win_h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, fbotex_tex(fbotex));
	gl_draw_quads(win_w/8, win_h/8, 0, win_w/2, win_h/2);

	if (rec.stat == 'r') {
		void *data[3];
		int line[3];
		fbotex_getyuv(fbotex, data, line);
		mp4enc_write_frame(enc, data, line, NULL, 0);
	}
//	dump("/tmp/Y.gray", data2[0], fbotex_h(fbotex)*line2[0]);
//	yuv2jpg("/tmp/c.jpg", fbotex_w(fbotex), fbotex_h(fbotex), data2, line2);

	glutSwapBuffers();
}

void idle()
{
	GLuint tm = glutGet(GLUT_ELAPSED_TIME);
	static GLuint lasttm, lasttm2;

	if (tm - lasttm > 1000./24) {
		glutPostRedisplay();
		lasttm = tm;
	}
	if (tm - lasttm2 > 1000) {
		printf("fps: %d\n", fps);
		fps = 0;
		lasttm2 = tm;
	}
}

void reshape(int w, int h)
{
	win_w = w;
	win_h = h;
}

void keyboard_cb(unsigned char key, int x, int y)
{
	switch (key) {
		case 'a':
			printf("key: a\n");
			if (!ani.stat) 
				ani.stat = 'l';
			break;
		case 's':
			printf("key: s\n");
			if (!ani.stat)
				ani.stat = 'r';
			break;
		case 'r':
			printf("key: r\n");
			if (!rec.stat) {
				rec.stat = 'r';
				printf("rec: start\n");
				enc = mp4enc_openfile("/tmp/out.mp4", VIDEO_W, VIDEO_H);
			} else {
				printf("rec: stop\n");
				mp4enc_close(enc);
			}
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

