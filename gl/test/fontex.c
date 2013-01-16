#include "a.h"

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

#include <wchar.h>

#define MAX_GLYPHS 4096

static FT_Library library;

typedef struct {
	FT_Face face;
	FT_Glyph glyphs[MAX_GLYPHS];
	FT_Vector pos[MAX_GLYPHS]; 
	FT_BBox box;
	int num_glyphs;

	wchar_t *text;
	int font_size;
	int w, h;

	GLuint tex, prog;
	GLfloat toff[25][2];
	GLuint loc[3];
} fontex_t ;

static int debug;

#define dbp(lev, ...) { \
	if (debug) \
		printf("fontex: " __VA_ARGS__);	\
	}

void fontex_loglevel(int l)
{
	debug = l;
}

static void _init()
{
	static int a;
	int error;

	if (a++)
		return ;

	error = FT_Init_FreeType( &library );
	if (error) {
		dbp(0, "freetype init failed\n");
	}

}

static void compute_string_bbox(fontex_t *m)
{
	FT_BBox  bbox;
	int n;

	bbox.xMin = bbox.yMin =  32000;
	bbox.xMax = bbox.yMax = -32000;

	for ( n = 0; n < m->num_glyphs; n++ ) {
		FT_BBox  glyph_bbox;
		int error;

		FT_Glyph_Get_CBox(m->glyphs[n], ft_glyph_bbox_pixels, &glyph_bbox );

		dbp(0, "glyph: %ld,%ld,%ld,%ld\n", 
				glyph_bbox.xMin,
				glyph_bbox.xMax,
				glyph_bbox.yMin,
				glyph_bbox.yMax
				);

		glyph_bbox.xMin += m->pos[n].x;
		glyph_bbox.xMax += m->pos[n].x;
		glyph_bbox.yMin += m->pos[n].y;
		glyph_bbox.yMax += m->pos[n].y;

		if (glyph_bbox.xMin < bbox.xMin)
			bbox.xMin = glyph_bbox.xMin;

		if (glyph_bbox.yMin < bbox.yMin)
			bbox.yMin = glyph_bbox.yMin;

		if (glyph_bbox.xMax > bbox.xMax)
			bbox.xMax = glyph_bbox.xMax;

		if (glyph_bbox.yMax > bbox.yMax)
			bbox.yMax = glyph_bbox.yMax;
	}

	/* check that we really grew the string bbox */
	if (bbox.xMin > bbox.xMax) {
		bbox.xMin = 0;
		bbox.yMin = 0;
		bbox.xMax = 0;
		bbox.yMax = 0;
	}
	
	dbp(0, "box: pos %ld,%ld,%ld,%ld\n", 
			bbox.xMin,
			bbox.xMax,
			bbox.yMin,
			bbox.yMax
		 );
	int w = bbox.xMax - bbox.xMin;
	int h = bbox.yMax - bbox.yMin;
	dbp(0, "box: size %dx%d\n", w, h);

	m->box = bbox;
	m->w = w;
	m->h = h;
}

static int debug_dump;

static void _dump(int i, void *data, int w, int h)
{
	char path[256];
	sprintf(path, "/tmp/ft%d.gray", i);
	FILE *fp = fopen(path, "wb+");
	fwrite(data, 1, w*h, fp);
	fclose(fp);
	char cmd[256];
	sprintf(cmd, "convert -depth 8 -size %dx%d /tmp/ft%d.gray /tmp/ft%d.png", 
			w, h, i, i);
	system(cmd);
}

static int load_glyphs(fontex_t *m)
{
	int pen_x, pen_y;
	int n;
	int previous;
	int error;

	pen_x = 0;
	pen_y = 0;
	previous = 0;
	m->num_glyphs = 0;

	/*
	FT_Stroker stroker;
	error = FT_Stroker_New(library, &stroker);
	if (error) {
		dbp(0, "stroke failed\n");
		return 1;
	}
	FT_Stroker_Set(stroker, 3*64, 
			FT_STROKER_LINECAP_ROUND,
			FT_STROKER_LINEJOIN_ROUND,
			0);
	*/

	for (n = 0; n < wcslen(m->text); n++) {
		FT_BBox glyph_bbox;
		FT_GlyphSlot slot = m->face->glyph;

		int glyph_index = FT_Get_Char_Index(m->face, m->text[n]);
		dbp(0, "idx: %d\n", glyph_index);

		if (previous && glyph_index) {
			FT_Vector delta;
			FT_Get_Kerning(m->face, previous, glyph_index,
					FT_KERNING_DEFAULT, &delta);
			pen_x += delta.x >> 6;
		}
	
		error = FT_Load_Glyph(m->face, glyph_index, FT_LOAD_DEFAULT);
		if (error) 
			continue;

		error = FT_Get_Glyph(slot, &m->glyphs[m->num_glyphs]);
		if (error)
			continue;

		m->pos[m->num_glyphs].x = pen_x;
		m->pos[m->num_glyphs].y = pen_y;

		dbp(0, "pos %d,%d\n", pen_x, pen_y);

		pen_x += slot->advance.x >> 6;
		previous = glyph_index;
		m->num_glyphs++;
	}

	return 0;
}

static int load_font(fontex_t *m)
{
	int error;
		
	dbp(0, "load_font: size %d\n", m->font_size);

	error = FT_New_Face(library, "/font/Hei.ttf", 0, &m->face);
	if (error) {
		dbp(0, "freetype newface failed\n");
		return 1;
	}
	dbp(0, "num_glyphs: %ld\n", m->face->num_glyphs);

	error = FT_Set_Char_Size(
			m->face,    /* handle to face object           */
			0,       /* char_width in 1/64th of points  */
			m->font_size*64,   /* char_height in 1/64th of points */
			300,     /* horizontal device resolution    */
			300);   /* vertical device resolution      */
	if (error) {
		dbp(0, "freetype setcharsize failed\n");
		return 1;
	}

	error = FT_Set_Pixel_Sizes(
			m->face,   /* handle to face object */
			0,      /* pixel_width           */
			m->font_size);   /* pixel_height          */
	if (error) {
		dbp(0, "freetype setpixelsize failed\n");
		return 1;
	}

	return 0;
}

static int gen_tex(fontex_t *m)
{
	glGenTextures(1, &m->tex);
	glBindTexture(GL_TEXTURE_2D, m->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	void *data = malloc(m->w*m->h);
	memset(data, 0, m->w*m->h);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m->w, m->h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
	gl_checkerr("fontex: gentex");
	free(data);

	int n, error;
	for (n = 0; n < m->num_glyphs; n++) {
		FT_Glyph g = m->glyphs[n];

		/*
		error = FT_Glyph_StrokeBorder(&g, stroker, 0, 0);
		if (error) {
			dbp(0, "bmp: stroke error %d\n", error);
			return 1;
		}
		*/

		error = FT_Glyph_To_Bitmap(&g, FT_RENDER_MODE_NORMAL, 0, 0);
		if (error) {
			dbp(0, "bmp: freetype glyphtobmp error %d\n", error);
			return 1;
		}

		FT_BitmapGlyph gb = (FT_BitmapGlyph)g;
		FT_Bitmap bmp = gb->bitmap;

		int offx = m->pos[n].x + gb->left - m->box.xMin;
		int offy = m->box.yMax - gb->top;

		dbp(0, "bmp: pos %ld left %d top %d\n", m->pos[n].x, gb->left, gb->top);
		dbp(0, "bmp: 	off %dx%d\n", offx, offy);
		dbp(0, "bmp: 	size %dx%d\n", bmp.pitch, bmp.rows);
		dbp(0, "bmp: 	mode %d\n", bmp.pixel_mode);
		dbp(0, "bmp: 	8bit mode %d\n", FT_PIXEL_MODE_GRAY);
		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 
				offx, offy,
				bmp.pitch, bmp.rows, 
				GL_ALPHA, GL_UNSIGNED_BYTE, bmp.buffer
				);

		//_dump(n, bmp.buffer, bmp.pitch, bmp.rows);

		FT_Done_Glyph(g);
	}

	//FT_Stroker_Done(stroker);

	for (n = 0; n < m->num_glyphs; n++)
		FT_Done_Glyph(m->glyphs[n]);

	return 0;
}

static void load_prog(fontex_t *m)
{
	m->prog = load_frag_shader("/shader/blur.frag");
 	m->loc[0] = glGetUniformLocation(m->prog, "t");
 	m->loc[1] = glGetUniformLocation(m->prog, "toff");
}

void *fontex_new(wchar_t *text, int font_size)
{
	_init();

	fontex_t *m = (fontex_t *)malloc(sizeof(*m));

	memset(m, 0, sizeof(*m));
	m->font_size = font_size;
	m->text = text;

	load_prog(m);
	load_font(m);
	load_glyphs(m);
	compute_string_bbox(m);
	gen_tex(m);

	return m;
}

GLuint fontex_tex(void *_m)
{
	fontex_t *m = (fontex_t *)_m;
	return m->tex;
}

void fontex_draw(void *_m, float x, float y)
{
	fontex_t *m = (fontex_t *)_m;

	glLoadIdentity();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m->tex);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_SMOOTH);

	glUseProgram(m->prog);
	glUniform1i(m->loc[0], 0);
	gl_draw_quads_revtex(x, y, 0, m->w, m->h);
	glUseProgram(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
}

float fontex_w(void *_m)
{
	fontex_t *m = (fontex_t *)_m;
	return m->w;
}

float fontex_h(void *_m)
{
	fontex_t *m = (fontex_t *)_m;
	return m->h;
}

