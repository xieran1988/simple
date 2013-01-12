
#include "a.h"

#include <gl/util/util.h>
#include <stdlib.h>

typedef struct {
	GLuint tex[3];
	int w, h;
} yuvtex_t ;

static GLuint prog;
static GLuint loc[3];

static GLuint _tex_new(int w, int h) 
{
	GLuint texName;

	glGenTextures(1, &texName);    
	glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	return texName;
}

void *yuvtex_new(int w, int h)
{
	yuvtex_t *m = (yuvtex_t *)malloc(sizeof(yuvtex_t));

	if (!prog) {
		prog = load_frag_shader("/shader/yuv2rgb.frag");
		if (!prog)
			return NULL;
		loc[0] = glGetUniformLocation(prog, "tex1");
		loc[1] = glGetUniformLocation(prog, "tex2");
		loc[2] = glGetUniformLocation(prog, "tex3");
	}

	m->w = w;
	m->h = h;

	m->tex[0] = _tex_new(w, h);
	m->tex[1] = _tex_new(w/2, h/2);
	m->tex[2] = _tex_new(w/2, h/2);

	return m;
}

void yuvtex_bind(void *_m, void **data, int *line)
{
	yuvtex_t *m = (yuvtex_t *)_m;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m->tex[0]);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, line[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m->w, m->h, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m->tex[1]);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, line[1]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m->w/2, m->h/2, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[1]);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m->tex[2]);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, line[2]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m->w/2, m->h/2, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[2]);

	glUseProgram(prog);
	glUniform1i(loc[0], 0);
	glUniform1i(loc[1], 1);
	glUniform1i(loc[2], 2);
}

void yuvtex_unbind()
{
	glUseProgram(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

