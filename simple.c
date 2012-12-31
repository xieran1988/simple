#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include "decode.h"

GLuint t1, t2, t3;
GLuint t1loc, t2loc, t3loc;
GLuint program;
extern const GLchar* source;

GLuint makeDataTex(void *data, int w, int h) {
	GLuint texName;

	glGenTextures(1, &texName);    
	glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);

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


void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_FLAT);

	void *data[3];
	int linesize[3];
	void *m = mp4_open("/Users/xieran/1.mp4");
	mp4_seek_precise(m, 5.5);
	mp4_read_frame(m, data, linesize);
	printf("pos %f\n", mp4_pos(m));
	int h = mp4_height(m);
	printf("h %d\n", h);
	printf("linesize %d,%d,%d\n", linesize[0], linesize[1], linesize[2]);

	FILE *fp = fopen("/tmp/dy", "wb+");
	fwrite(data[0], 1, linesize[0]*h, fp);
	fclose(fp);

	t1 = makeDataTex(data[0], linesize[0], h);
	t2 = makeDataTex(data[1], linesize[1], h/2);
	t3 = makeDataTex(data[2], linesize[2], h/2);

	/*
	t1 = makeFileTex("Image.Y", 672, 360);
	t2 = makeFileTex("Image.U", 336, 180);
	t3 = makeFileTex("Image.V", 336, 180);
	*/

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

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, t2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, t3);

	glUseProgram(program);
	glUniform1i(t1loc, 0);
	glUniform1i(t2loc, 1);
	glUniform1i(t3loc, 2);

	glBegin(GL_QUADS);
	//lower left
	glTexCoord2f(0, 0);
	glVertex2f(-1.0, -1.0);
	//upper left
	glTexCoord2f(0, 1.0);
	glVertex2f(-1.0, 1.0);
	//upper right
	glTexCoord2f(1.0, 1.0);
	glVertex2f(1.0, 1.0);
	//lower right
	glTexCoord2f(1.0, 0);
	glVertex2f(1.0, -1.0);
	glEnd();

	glUseProgram(0);

	glutSwapBuffers();
}


void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

	glutInitWindowSize(640, 360);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("GLSL Texture Blending");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);

	init();

	glutMainLoop();

	return 0;
}
