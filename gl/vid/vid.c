#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <av/av.h>
#include <gl/shader/shader.h>

typedef struct {
	GLuint loc[3], tex[3], prog;
	int w, h;
} video_t ;

#define V(_v) ((video_t *)(_v))

static int debug = 1;

#define dbp(lev, ...) { \
	if (debug) \
		printf("video: " __VA_ARGS__);	\
	}

static GLuint _tex_new(int w, int h) 
{
	GLuint t;

	glGenTextures(1, &t);    
	glBindTexture(GL_TEXTURE_2D, t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	return t;
}

void *video_new(int w, int h)
{
	video_t *v = (video_t *)malloc(sizeof(video_t));

	v->w = w;
	v->h = h;

	v->tex[0] = _tex_new(w, h); 
	v->tex[1] = _tex_new(w/2, h/2); 
	v->tex[2] = _tex_new(w/2, h/2); 

	v->prog = shader_new_frag("/shader/yuv2rgb.frag");
	if (!v->prog)
		return NULL;

	v->loc[0] = glGetUniformLocation(v->prog, "tex1");
	v->loc[1] = glGetUniformLocation(v->prog, "tex2");
	v->loc[2] = glGetUniformLocation(v->prog, "tex3");

	dbp(0, "video_new %d,%d,%d\n", v->loc[0], v->loc[1], v->loc[2]);

	return v;
}

void video_render_start(void *_v, void **data, int *line)
{
	video_t *v = V(_v);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, v->tex[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, line[0], v->h, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, v->tex[1]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, line[1], v->h/2, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[1]);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, v->tex[2]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, line[2], v->h/2, 
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data[2]);

	glUseProgram(v->prog);
	glUniform1i(v->loc[0], 0);
	glUniform1i(v->loc[1], 1);
	glUniform1i(v->loc[2], 2);
}

void video_render_end(void *_v)
{
	glUseProgram(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


