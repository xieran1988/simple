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

GLuint fb, fbt1, fbt2, masktex, tex2, texstr;

void *yuvtex;

#define VIDEO_W 640
#define VIDEO_H 360

#define FBO_W 128
#define FBO_H 128

#define	checkImageWidth 64
#define	checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];
static GLuint texName, texpic;

void makeCheckImage(void)
{
	int i, j, c;

	for (i = 0; i < checkImageHeight; i++) {
		for (j = 0; j < checkImageWidth; j++) {
			c = ((((i&0x8)==0)^((j&0x8))==0))*255;
			checkImage[i][j][0] = (GLubyte) c;
			checkImage[i][j][1] = (GLubyte) c;
			checkImage[i][j][2] = (GLubyte) c;
			checkImage[i][j][3] = (GLubyte) 255;
		}
	}
}

GLuint fbotex[4], fbodph, fb, mrtprog;

void init_fbo(int w, int h)
{
	int i;

	glGenTextures(4, fbotex);

	glBindTexture(GL_TEXTURE_2D, fbotex[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	gl_checkerr("fbo1.1");

	glBindTexture(GL_TEXTURE_2D, fbotex[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	gl_checkerr("fbo1.2");

	glBindTexture(GL_TEXTURE_2D, fbotex[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	gl_checkerr("fbo1.3");

	glBindTexture(GL_TEXTURE_2D, fbotex[3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	gl_checkerr("fbo1.4");

	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbotex[0], 0);
	gl_checkerr("fbo2.1");
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, fbotex[1], 0);
	gl_checkerr("fbo2.2");
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_TEXTURE_2D, fbotex[2], 0);
	gl_checkerr("fbo2.3");
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT3_EXT, GL_TEXTURE_2D, fbotex[3], 0);
	gl_checkerr("fbo2.4");

	glGenRenderbuffersEXT(1, &fbodph);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbodph);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
	gl_checkerr("fbo3");

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbodph);
	gl_checkerr("fbo4");

	GLenum status;
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
		printf("fbo init failed\n");
	}

	mrtprog = load_frag_shader("/shader/mrt.frag");
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glDepthRange(0, 1);
	glDepthFunc(GL_LEQUAL);							// The Type Of Depth Test To Do

	makeCheckImage();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#ifdef GL_VERSION_1_1
	glGenTextures(1, &texName);
	glBindTexture(GL_TEXTURE_2D, texName);
#endif

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef GL_VERSION_1_1
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth, checkImageHeight, 
			0, GL_RGBA, GL_UNSIGNED_BYTE, checkImage);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, 4, checkImageWidth, checkImageHeight, 
			0, GL_RGBA, GL_UNSIGNED_BYTE, checkImage);
#endif
	
	init_fbo(FBO_W, FBO_H);
	yuvtex = yuvtex_new(VIDEO_W, VIDEO_H);

	texpic = load_rgb_tex("128x128.rgb", FBO_W, FBO_H);
}

static void dump(char *name, void *data, int len)
{
	FILE *fp = fopen("/tmp/t", "wb+");
	fwrite(data, 1, len, fp);
	fclose(fp);
	rename("/tmp/t", name);
}

/*
void draw_video(float x, float y, float z, float w, float h) 
{
	yuvtex_bind(yuvtex, data, linesize);
	gl_draw_quads(x, y, z, w, h);
	yuvtex_unbind();
}
*/

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
	glBindTexture(GL_TEXTURE_2D, texName);

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

	GLuint loc[2];
 	loc[0] = glGetUniformLocation(mrtprog, "t");
	loc[1] = glGetUniformLocation(mrtprog, "s");
	loc[2] = glGetUniformLocation(mrtprog, "w");
	loc[3] = glGetUniformLocation(mrtprog, "h");

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, FBO_W, FBO_H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, FBO_W, 0.0, FBO_H, -1.0, 1.0); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GLenum buffers[] = { 
		GL_COLOR_ATTACHMENT0_EXT, 
		GL_COLOR_ATTACHMENT1_EXT,
		GL_COLOR_ATTACHMENT2_EXT,
		GL_COLOR_ATTACHMENT3_EXT,
	}; 
	glDrawBuffers(4, buffers); 

	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, texpic);
	gl_draw_quads(0, 0, 0, FBO_W, FBO_H);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbotex[0]);

	glUseProgram(mrtprog);
	glUniform1i(loc[0], 0);
	glUniform1f(loc[1], 0.9);
	glUniform1i(loc[2], FBO_W);
	glUniform1i(loc[3], FBO_H);
	gl_draw_quads(0, 0, 0, FBO_W, FBO_H);
	glUseProgram(0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, fbotex[0]);
	gl_draw_quads(0.1, 0.1, 0, 0.2, 0.2);

	static char buf2[FBO_W*FBO_H*4];
	void *data2[3] = {buf2, buf2+FBO_W*FBO_H, buf2+FBO_W*FBO_H+FBO_W*FBO_H/4};
	int line2[3] = {FBO_W, FBO_W/2, FBO_W/2};

	glBindTexture(GL_TEXTURE_2D, fbotex[1]);
	gl_draw_quads(0.1, 0.4, 0, 0.2, 0.2);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf2);

	dump("/tmp/Y.gray", buf2, FBO_H*FBO_W);
	dump("/tmp/U.gray", buf2+FBO_H*FBO_W, FBO_H*FBO_W/4);
	dump("/tmp/V.gray", buf2+FBO_H*FBO_W+FBO_H*FBO_W/4, FBO_H*FBO_W/4);
	yuv2jpg("/tmp/t.jpg", FBO_W, FBO_H, data2, line2);


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
