#include "a.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int w, h;
	GLuint tex[2], fb, dph;
	GLuint prog, loc[3];
	uint8_t *data;
} fbotex_t ;

void *fbotex_new(int w, int h) 
{
	fbotex_t *m;

	if (w & 7) {
		printf("fbotex: width must be multiple of 8\n");
		return NULL;
	}

	m = (fbotex_t *)malloc(sizeof(fbotex_t));
	memset(m, 0, sizeof(*m));

	m->w = w;
	m->h = h;

	glGenTextures(2, m->tex);

	glBindTexture(GL_TEXTURE_2D, m->tex[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, m->tex[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenFramebuffersEXT(1, &m->fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m->fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m->tex[0], 0);
	gl_checkerr("fbo2.1");
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, m->tex[1], 0);
	gl_checkerr("fbo2.2");

	glGenRenderbuffersEXT(1, &m->dph);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m->dph);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
	gl_checkerr("fbo3");

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m->dph);
	gl_checkerr("fbo4");

	GLenum status;
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
		printf("fbotex: fbo init failed\n");
		return NULL;
	}

	m->prog = load_frag_shader("/shader/rgb2yuv.frag");
	if (!m->prog)
		return NULL;

 	m->loc[0] = glGetUniformLocation(m->prog, "t");
	m->loc[1] = glGetUniformLocation(m->prog, "w");
	m->loc[2] = glGetUniformLocation(m->prog, "h");

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	return m;
}

void fbotex_render_start(void *_m)
{
	fbotex_t *m = (fbotex_t *)_m;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m->fb);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m->w, m->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, m->w, 0.0, m->h, -1.0, 1.0); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GLenum buffers[] = { 
		GL_COLOR_ATTACHMENT0_EXT, 
		GL_COLOR_ATTACHMENT1_EXT,
	}; 
	glDrawBuffers(2, buffers); 

	glLoadIdentity();
}

void fbotex_render_end(void *_m)
{
	fbotex_t *m = (fbotex_t *)_m;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m->tex[0]);

	glUseProgram(m->prog);
	glUniform1i(m->loc[0], 0);
	glUniform1i(m->loc[1], m->w);
	glUniform1i(m->loc[2], m->h);
	gl_draw_quads(0, 0, 0, m->w, m->h);
	glUseProgram(0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

int fbotex_w(void *_m)
{
	fbotex_t *m = (fbotex_t *)_m;
	return m->w;
}

int fbotex_h(void *_m)
{
	fbotex_t *m = (fbotex_t *)_m;
	return m->h;
}

void fbotex_getyuv(void *_m, void **data, int *line)
{
	fbotex_t *m = (fbotex_t *)_m;

	if (!m->data) 
		m->data = malloc(m->w*m->h*4);

	glBindTexture(GL_TEXTURE_2D, m->tex[1]);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m->data);

	line[0] = m->w;
	line[1] = m->w/2;
	line[2] = m->w/2;
	data[0] = m->data;
	data[1] = m->data + line[0]*m->h;
	data[2] = m->data + line[0]*m->h + line[1]*m->h/2;
}

GLuint fbotex_tex(void *_m)
{
	fbotex_t *m = (fbotex_t *)_m;
	return m->tex[0];
}

GLuint fbotex_tex2(void *_m)
{
	fbotex_t *m = (fbotex_t *)_m;
	return m->tex[1];
}

