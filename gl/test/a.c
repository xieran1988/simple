#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <av/encdec/a.h>
#include <av/util/a.h>

#include "a.h"

#define VIDEO_W 640
#define VIDEO_H 360

#define FBO_W 640
#define FBO_H 360

static GLuint rgbtex, pictex;
void *yuvtex, *fbotex;
void *dec[4], *enc;
int win_w, win_h;

void all_init(int w, int h)
{
	win_w = w;
	win_h = h;

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glDepthRange(0, 1);
	glDepthFunc(GL_LEQUAL);
	
	yuvtex = yuvtex_new(win_w, win_h);
	fbotex = fbotex_new(FBO_W, FBO_H);
	rgbtex = sample_rgb_tex(64, 64);
	pictex = load_rgb_tex("128x128.rgb", 128, 128);

	dec[0] = mp4dec_open("/vid/1.mp4");
	dec[1] = mp4dec_open("/vid/2.mp4");
	dec[2] = mp4dec_open("/vid/3.mp4");
	dec[3] = mp4dec_open("/vid/4.mp4");
}

static void dump(char *name, void *data, int len)
{
	FILE *fp = fopen("/tmp/t", "wb+");
	fwrite(data, 1, len, fp);
	fclose(fp);
	rename("/tmp/t", name);
}

struct {
	char stat;
	char s1;
	float pos;
	int idx;
	int li, ri;
	float j;
} ani;

struct {
	void *data[3];
	int line[3];
} mov[4];

struct {
	char stat;
} rec;

struct {
	char nodisp;
	char noreadpix;
} mon;

void all_render()
{
	printf("render\n");

	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-0.5, 0.5, -0.5, 0.5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

	int i;
	for (i = 0; i < 4; i++) {
		if (mp4dec_read_frame(dec[i], mov[i].data, mov[i].line, NULL, NULL)) {
			mp4dec_seek_precise(dec[i], 0);
		}
	}

	int fw = fbotex_w(fbotex);
	int fh = fbotex_h(fbotex);

	if (ani.stat == 'l' || ani.stat == 'r') {
		if (ani.j < 1) {
			if (ani.stat == 'r') {
				ani.pos = sinf(ani.j*3.14/2)*fw;
				ani.li = (ani.idx+3)%4;
				ani.ri = ani.idx;
			}
			if (ani.stat == 'l') {
				ani.pos = (1-sinf(ani.j*3.14/2))*fw;
				ani.li = ani.idx;
				ani.ri = (ani.idx+5)%4;
			}
			ani.j += 1./12;
		} else {
			ani.j = 0;
			ani.pos = 0;
			if (ani.stat == 'r') 
				ani.idx = ani.li;
			if (ani.stat == 'l') 
				ani.idx = ani.ri;
			ani.stat = 0;
		}
	}

	fbotex_render_start(fbotex);
	if (ani.stat == 'l' || ani.stat == 'r') {
		yuvtex_bind(yuvtex, mov[ani.li].data, mov[ani.li].line);
		gl_draw_quads(ani.pos-fw, 0, 0, fw, fh);
		yuvtex_unbind();
		yuvtex_bind(yuvtex, mov[ani.ri].data, mov[ani.ri].line);
		gl_draw_quads(ani.pos, 0, 0, fw, fh);
		yuvtex_unbind();
	} 
	if (ani.stat == '4') {
		yuvtex_bind(yuvtex, mov[0].data, mov[0].line);
			gl_draw_quads(0, 0, 0, fw/2, fh/2);
		yuvtex_unbind();
		yuvtex_bind(yuvtex, mov[1].data, mov[1].line);
			gl_draw_quads(fw/2, 0, 0, fw/2, fh/2);
		yuvtex_unbind();
		yuvtex_bind(yuvtex, mov[2].data, mov[2].line);
			gl_draw_quads(0, fh/2, 0, fw/2, fh/2);
		yuvtex_unbind();
		yuvtex_bind(yuvtex, mov[3].data, mov[3].line);
			gl_draw_quads(fw/2, fh/2, 0, fw/2, fh/2);
		yuvtex_unbind();
	}
	if (!ani.stat) {
		yuvtex_bind(yuvtex, mov[ani.idx].data, mov[ani.idx].line);
		gl_draw_quads(0, 0, 0, fw, fh);
		yuvtex_unbind();
	}
	fbotex_render_end(fbotex);

	glViewport(0, 0, win_w, win_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, win_w, 0, win_h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (!mon.nodisp) {
		glLoadIdentity();
		glBindTexture(GL_TEXTURE_2D, fbotex_tex(fbotex));
		gl_draw_quads(win_w/8, win_h/8, 0, win_w/2, win_h/2);
	}

	if (rec.stat == 'r') {
		void *data[3];
		int line[3];
		fbotex_getyuv(fbotex, data, line);
		mp4enc_write_frame(enc, data, line, NULL, 0);
	}
	if (rec.stat == 't') {
		void *data[3];
		int line[3];
		fbotex_getyuv(fbotex, data, line);
		mp4enc_setpts(enc, tm_elapsed());
		mp4enc_write_frame(enc, data, line, NULL, 0);
	}
//	dump("/tmp/Y.gray", data2[0], fh*line2[0]);
//	yuv2jpg("/tmp/c.jpg", fw, fh, data2, line2);

}

void all_ctrl(char *fmt, ...)
{
	char key = *fmt;

	switch (key) {
		case 'a':
			printf("key: a\n");
			if (!ani.stat) 
				ani.stat = 'l';
			break;
		case 's':
			printf("key: s\n");
			if (!ani.stat)
				ani.stat = 'r';
			break;
		case 'r':
			printf("key: r\n");
			if (!rec.stat) {
				rec.stat = 'r';
				printf("rec: start\n");
				enc = mp4enc_openfile("/tmp/out.mp4", FBO_W, FBO_H);
			} else {
				rec.stat = 0;
				printf("rec: stop\n");
				mp4enc_close(enc);
			}
			break;
		case 't':
			if (!rec.stat) {
				enc = mp4enc_openrtmp("rtmp://localhost/myapp/1", FBO_W, FBO_H);
				//mp4enc_loglevel(1);
				rec.stat = 't';
				printf("rtmp: start\n");
			} else {
				rec.stat = 0;
				printf("rtmp: end\n");
				mp4enc_close(enc);
			}
			break;
		case 'n':
			printf("key: n\n");
			mon.nodisp ^= 1;
			break;
		case 'm':
			printf("key: m\n");
			mon.noreadpix ^= 1;
			fbotex_set(fbotex, mon.noreadpix ? "noreadpix" : "readpix");
			break;
		case '4':
			printf("key: 4\n");
			if (!ani.stat)
				ani.stat = '4';
			else
				ani.stat = 0;
			break;
	}
}
