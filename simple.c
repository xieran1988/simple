#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include "decode.h"

#include <freetype-gl.h>
#include <vertex-buffer.h>

GLuint t1, t2, t3;
GLuint t1loc, t2loc, t3loc;
GLuint program;
extern const GLchar* source;

int win_w, win_h;

GLuint fb, fbt1, fbt2, masktex, tex2, texstr;

static void *mp4[4];
void *data[3];
int linesize[3];
void *alldata[4][3];
int alllinesize[4][3];
float allpos[4];
float allstart[4];

typedef struct {
    float x, y, z;    // position
    float s, t;       // texture
    float r, g, b, a; // color
} vertex_t;
vertex_buffer_t *buffer;
texture_atlas_t *atlas;

#define VIDEO_H 360
#define VIDEO_W 640

GLuint makeDataTex(void *data, int w, int h) {
	GLuint texName;

	glGenTextures(1, &texName);    
	glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

	return texName;
}

GLuint makeFileTex(const char *fname, int w, int h) {
	static char buf[1024*1024*2];
	FILE *fp = fopen(fname, "rb");
	int size = fread(buf, 1, w*h, fp);
	fclose(fp);
	return makeDataTex(buf, w, h);
}

void loadShader() {
	program = glCreateProgram();
	GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint logLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		GLchar* log = (GLchar*)malloc(logLength);
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		printf("Shader compile log:\n%s\n", log);
		free(log);
	}

	glAttachShader(program, shader);  
	glLinkProgram(program);

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		GLchar* log = (GLchar*)malloc(logLength);
		glGetProgramInfoLog(program, logLength, &logLength, log);
		printf("Program link log:\n%s\n", log);
		free(log);
	}

	t1loc = glGetUniformLocation(program, "tex1");
	t2loc = glGetUniformLocation(program, "tex2");
	t3loc = glGetUniformLocation(program, "tex3");
}

void init_tex()
{
	uint8_t data[][4] = {
		{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
		{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
		{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}
	};

	glGenTextures(1, &masktex);    
	glBindTexture(GL_TEXTURE_2D, masktex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	uint8_t data2[][4] = {
		{0,0,0}, {0,0,0}, {0,0,0,64}, {0,0,0,128},
		{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
		{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}
	};

	uint8_t data3[] = {
		0,0,64,128,
		0,0,192,0,
		0,0,0,0,
	};

	glGenTextures(1, &tex2);    
	glBindTexture(GL_TEXTURE_2D, tex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 4, 3, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data3);

	glGenTextures(1, &texstr);
	glBindTexture(GL_TEXTURE_2D, texstr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	ft_fill_texture(L"测试一个句子", 50);

	int w, h;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	printf("strtex: %dx%d\n", w, h);
}

void init_mp4() 
{
	int i;
	for (i = 0; i < 4; i++) {
		char path[256];
		sprintf(path, "/vid/%d.mp4", i+1);
		mp4[i] = mp4_open(path);
		mp4_seek_precise(mp4[i], 2.3);
		mp4_read_frame(mp4[i], data, linesize);
		allpos[i] = mp4_pos(mp4[i]);
		allstart[i] = -mp4_pos(mp4[i]);
		int h = VIDEO_H;
		t1 = makeDataTex(data[0], linesize[0], h);
		t2 = makeDataTex(data[1], linesize[1], h/2);
		t3 = makeDataTex(data[2], linesize[2], h/2);
	}
}

void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glDepthRange(0, 1);
	
	init_tex();
	init_mp4();

	loadShader();
}

#define STRINGIFY(A) #A
const GLchar* source = STRINGIFY(
	uniform sampler2D tex1;
	uniform sampler2D tex2;
	uniform sampler2D tex3;
	void main() {
		vec2 c0 = gl_TexCoord[0].st;
		vec2 c = vec2(c0.x, 1.0 - c0.y);
		float y = texture2D(tex1, c).r;
		float u = texture2D(tex2, c).r - 0.5;
		float v = texture2D(tex3, c).r - 0.5;
		float r = y + 1.402 * v;  
		float g = y - 0.344 * u - 0.714 * v;  
		float b = y + 1.772 * u;       
		gl_FragColor = vec4(r,g,b,1.0);
	}
);

static void dump(char *name, void *data, int len)
{
	FILE *fp = fopen("/tmp/t", "wb+");
	fwrite(data, 1, len, fp);
	fclose(fp);
	rename("/tmp/t", name);
}

void draw_quads(float x, float y, float z, float w, float h, float cw)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(x, y-h, z);
		glTexCoord2f(0, 1); glVertex3f(x, y, z);
		glTexCoord2f(cw, 1); glVertex3f(x+w, y, z);
		glTexCoord2f(cw, 0); glVertex3f(x+w, y-h, z);
	glEnd();
}

void draw_video(float x, float y, float z, float w, float h) 
{
	float cw = VIDEO_W*1./linesize[0];

	glLoadIdentity();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, linesize[0], VIDEO_H, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, t2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, linesize[1], VIDEO_H/2, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[1]);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, t3);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, linesize[2], VIDEO_H/2, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[2]);

	glUseProgram(program);
	glUniform1i(t1loc, 0);
	glUniform1i(t2loc, 1);
	glUniform1i(t3loc, 2);
	draw_quads(x, y, z, w, h, cw);
	glUseProgram(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void display()
{
	glViewport(0, 0, win_w, win_h);
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int i;
	float pos[4][2] = {
		{-1,1}, {-1,0}, {0,1}, {0,0}
	};
	for (i = 0; i < 4; i++) { 
		memcpy(data, alldata[i], sizeof(alldata[i]));
		memcpy(linesize, alllinesize[i], sizeof(alllinesize[i]));
		//draw_video(pos[i][0], pos[i][1], 0, 1, 1);
	}
	draw_video(-0.5, 0.5, 0.0, 1, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

//	glEnable(GL_ALPHA_TEST);
//	glAlphaFunc(GL_GREATER, 0.0);

	glLoadIdentity();
  glBindTexture(GL_TEXTURE_2D, texstr);

	float tw, th, w, h, x, y, z;
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

	/*
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
	float cur = (float)glutGet(GLUT_ELAPSED_TIME)/1000;
	int i;
	for (i = 0; i < 4; i++) {
		float next = allstart[i] + allpos[i];
		if (cur > next) {
			int r = mp4_read_frame(mp4[i], alldata[i], alllinesize[i]);
			if (r) {
				mp4_seek(mp4[i], 0);
				float pos = mp4_pos(mp4[i]);
				allstart[i] = cur - pos;
				allpos[i] = pos;
			} else {
				allpos[i] = mp4_pos(mp4[i]);
			}
		}
	}
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
