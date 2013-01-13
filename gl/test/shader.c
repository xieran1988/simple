#include <gl/util/a.h>

#include <stdlib.h>
#include <stdio.h>

GLuint load_frag_shader(char *filename) 
{
	GLuint program = glCreateProgram();
	GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

	static char _src[65536];
	const char *src = _src;
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		printf("shader: open %s failed\n", filename);
		return 0;
	}
	int len = fread(_src, 1, sizeof(_src), fp);
	_src[len] = 0;

	glShaderSource(shader, 1, &src, NULL);
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

	return program;
}

