#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

static int debug = 1;

#define dbp(lev, ...) { \
	if (debug) \
		printf("shader: " __VA_ARGS__);	\
	}

GLuint shader_new_frag(char *filename)
{
	GLuint program = glCreateProgram();
	GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

	static char buf[64*1024];
	const char *src = buf;
	FILE *fp = fopen(filename, "r");
	int len;
	if (!fp) {
		dbp(0, "open %s failed\n", filename);
		return 0;
	}
	len = fread(buf, 1, sizeof(buf)-1, fp);
	buf[len] = 0;
	fclose(fp);

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint logLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		GLchar* log = (GLchar*)malloc(logLength);
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		dbp(0, "log: %s\n", log);
		free(log);
	}

	glAttachShader(program, shader);  
	glLinkProgram(program);

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		GLchar* log = (GLchar*)malloc(logLength);
		glGetProgramInfoLog(program, logLength, &logLength, log);
		dbp(0, "log2: %s\n", log);
		free(log);
	}

	return program;
}

