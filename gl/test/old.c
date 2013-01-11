#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <av/encdec/a.h>
#include <av/util/a.h>
//#include <gl/font/font.h>

#include "a.h"

//#include <freetype-gl.h>
//#include <vertex-buffer.h>

GLuint t1, t2, t3;
GLuint t1loc, t2loc, t3loc;
GLuint program;
extern const GLchar* source;

int win_w, win_h;

GLuint fb, fbt1, fbt2, masktex, tex2, texstr;

/*
static void *mp4[4];
void *data[3];
int linesize[3];
void *alldata[4][3];
int alllinesize[4][3];
float allpos[4];
float allstart[4];
*/
void *yuvtex;

#define VIDEO_H 360
#define VIDEO_W 640

#define	checkImageWidth 64
#define	checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];

#ifdef GL_VERSION_1_1
static GLuint texName;
#endif

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

void init_mp4() 
{
	/*
	int i;
	for (i = 0; i < 4; i++) {
		char path[256];
		sprintf(path, "/vid/%d.mp4", i+1);
		mp4[i] = mp4dec_open(path);
		mp4dec_seek_precise(mp4[i], 2.3);
		mp4dec_read_frame(mp4[i], data, linesize, NULL, NULL);
		allpos[i] = mp4dec_pos(mp4[i]);
		allstart[i] = -mp4dec_pos(mp4[i]);
	}
	*/
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

	
//	init_mp4();
	init_fbo(32, 32);
	yuvtex = yuvtex_new(VIDEO_W, VIDEO_H);
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
	/*
	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-0.5, 0.5, -0.5, 0.5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	int i;
	float pos[4][2] = {
		{-1,1}, {-1,0}, {0,1}, {0,0}
	};
	for (i = 0; i < 4; i++) {
		memcpy(data, alldata[i], sizeof(alldata[i]));
		memcpy(linesize, alllinesize[i], sizeof(alllinesize[i]));
		//draw_video(pos[i][0], pos[i][1], 0, 1, 1);
	}

	if (linesize[0])
		draw_video(-0.5, 0.5, 0.0, 1, 1);

	if (linesize[0])
		draw_video(-0.2, 0.2, 0.1, 0.3, 0.3);
		*/

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

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, 32, 32);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 32.0, 0.0, 32.0, -1.0, 1.0); 
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

	glColor3f(1, 0, 0);
	gl_draw_quads(0, 16, 0, 16, 16);
	glColor3f(0, 1, 0);
	gl_draw_quads(0, 0, 0, 16, 16);
	glColor3f(0, 0, 1);
	gl_draw_quads(16, 16, 0, 16, 16);
	glColor3f(1, 0, 1);
	gl_draw_quads(16, 0, 0, 16, 16);
	glColor3f(0.4, 0.4, 0.3);
	gl_draw_quads(8, 8, 0, 16, 16);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbotex[0]);
	GLuint loc = glGetUniformLocation(mrtprog, "t");
	glUniform1i(loc, 0);

	glUseProgram(mrtprog);
	gl_draw_quads(0, 0, 0, 32, 32);
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

	static char buf2[32*32*4];
	void *data2[3] = {buf2, buf2+32*32, buf2+32*32+32*32/4};
	int line2[3] = {32, 16, 16};

	glBindTexture(GL_TEXTURE_2D, fbotex[1]);
	gl_draw_quads(0.1, 0.4, 0, 0.2, 0.2);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf2);
	dump("/tmp/Y", buf2, 32*32);
	yuv2jpg("/tmp/t.jpg", 32, 32, data2, line2);

	/*
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

//	glEnable(GL_ALPHA_TEST);
//	glAlphaFunc(GL_GREATER, 0.0);

	glLoadIdentity();
  glBindTexture(GL_TEXTURE_2D, texstr);

	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
	float d2 = tw/th;
	float d1 = win_w*1./win_h;
	x = -0.3; y = 0; z = 0.1; h = 0.1; w = h*d2/d1;
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(x, y-h, z);
		glTexCoord2f(0, 0); glVertex3f(x, y, z);
		glTexCoord2f(1, 0); glVertex3f(x+w, y, z);
		glTexCoord2f(1, 1); glVertex3f(x+w, y-h, z);
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glLoadIdentity();
  glBindTexture(GL_TEXTURE_2D, masktex);
	draw_quads(0, 0, 0.1, 1, 1, 1);
	*/

	//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
	//glColor4f(1,1,1,1);
	//glScalef(1./50, 1./50, 1./50);
	//vertex_buffer_render( buffer, GL_TRIANGLES, "vtc" );
	//glDisable(GL_BLEND);
	//glDisable(GL_TEXTURE_2D);
//	static char buf[1024];
//	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
//	dump("/tmp/mytex", buf, 4*3*4);


	glutSwapBuffers();
}

void idle()
{
	/*
	float cur = (float)glutGet(GLUT_ELAPSED_TIME)/1000;
	int i;
	for (i = 0; i < 4; i++) {
		float next = allstart[i] + allpos[i];
		if (cur > next) {
			int r = mp4dec_read_frame(mp4[i], alldata[i], alllinesize[i], NULL, NULL);
			if (r) {
				mp4dec_seek_precise(mp4[i], 0);
				float pos = mp4dec_pos(mp4[i]);
				allstart[i] = cur - pos;
				allpos[i] = pos;
			} else {
				allpos[i] = mp4dec_pos(mp4[i]);
			}
		}
	}
	*/
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
