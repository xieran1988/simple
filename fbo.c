#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include "decode.h"

GLuint fb, color_tex, color_tex2, rb, depth_rb;
GLuint program;
extern const GLchar* source;

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
}

void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
//	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_FLAT);

	glGenFramebuffersEXT(1, &fb);
	glGenTextures(1, &color_tex);

	int i;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 800, 600, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
			GL_TEXTURE_2D, color_tex, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glGenRenderbuffersEXT(1, &depth_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 800, 600);

	GLuint maxbuffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);
	printf("max draw buffers = %d\n", maxbuffers);

	GLenum status;
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status)
	{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			printf("good\n");
			break;
		default:
			printf("error\n");
	}


	loadShader();
}

#define STRINGIFY(A) #A
const GLchar* source = STRINGIFY(
	uniform sampler2D tex1;
	uniform sampler2D tex2;
	uniform sampler2D tex3;
	void main() {
		float r = 0.6;
		float g = 0.5;
		float b = 0.3;
		gl_FragColor = vec4(r,g,b,1.0);
	}
);

void dump_gray(int quit)
{
#define W 800
#define H 600
	static GLubyte pixels[W*H];

	glReadBuffer( GL_BACK );
	glReadPixels( 0, 0, W, H, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels); 

	FILE *fp = fopen("/tmp/c.gray", "wb+");
	fwrite(pixels, 1, sizeof(pixels), fp);
	fclose(fp);

	if (quit) {
		char cmd[1024];
		sprintf(cmd, "display -depth 8 -size %dx%d /tmp/c.gray", W, H);
		system(cmd);
		exit(0);
	}
}

void dump_bmp(int quit)
{
#define W 800
#define H 600
	static GLubyte pixels[W*H*3];

	glReadBuffer( GL_BACK );
	glReadPixels( 0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, pixels); 

	FILE *fp = fopen("/tmp/c.rgb", "wb+");
	fwrite(pixels, 1, sizeof(pixels), fp);
	fclose(fp);

	if (quit) {
		char cmd[1024];
		sprintf(cmd, "display -depth 8 -size %dx%d /tmp/c.rgb", W, H);
		system(cmd);
		exit(0);
	}
}

void display()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	float r;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glViewport(0, 0, 800, 600);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 1, 10);
	glOrtho(0.0, 256.0, 0.0, 256.0, -1.0, 1.0); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor4f(0.0, 1.0, 0.0, 1.0);
	glBegin(GL_QUADS);
		glVertex2f(100, 100);
		glVertex2f(200, 100);
		glVertex2f(200, 200);
		glVertex2f(100, 200);
	glEnd();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glClearColor(1.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glViewport(0, 0, 800, 600);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 256.0, 0.0, 256.0, -1.0, 1.0); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex2f(0, 128);
		glTexCoord2f(1, 1); glVertex2f(128, 128);
		glTexCoord2f(1, 0); glVertex2f(128, 0);
		glTexCoord2f(0, 0); glVertex2f(0, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glFlush();
	
	glutSwapBuffers();
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

	glutInitWindowSize(800, 600);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("GLSL Texture Blending");

	glutDisplayFunc(display);
	glutIdleFunc(display);

	init();

	glutMainLoop();

	return 0;
}
