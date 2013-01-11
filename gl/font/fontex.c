#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#define MAX_GLYPHS 4096

static FT_Library library;
static FT_Face face;
static FT_Glyph glyphs[MAX_GLYPHS];
static FT_Vector pos[MAX_GLYPHS]; 
static int num_glyphs;

static int debug;
static int inited;

#define dbp(lev, ...) { \
	if (debug) \
		printf("font: " __VA_ARGS__);	\
	}

typedef struct {
	
} font_t ;

static void _init()
{
	int error;

	if (inited++)
		return ;

	debug = 1;

	error = FT_Init_FreeType( &library );
	if (error) {
		dbp(0, "open ft failed\n");
	}

	error = FT_New_Face( library, "/font/Hei.ttf", 0, &face );
	if (error) {
		dbp(0, "newface failed\n");
	}
	dbp(0, "num_glyphs: %ld\n", face->num_glyphs);
}

static void compute_string_bbox(FT_BBox *ret)
{
	FT_BBox  bbox;
	int n;

	bbox.xMin = bbox.yMin =  32000;
	bbox.xMax = bbox.yMax = -32000;

	for ( n = 0; n < num_glyphs; n++ ) {
		FT_BBox  glyph_bbox;
		int error;

		FT_Glyph_Get_CBox(glyphs[n], ft_glyph_bbox_pixels, &glyph_bbox );

		dbp(0, "glyph: %ld,%ld,%ld,%ld\n", 
				glyph_bbox.xMin,
				glyph_bbox.xMax,
				glyph_bbox.yMin,
				glyph_bbox.yMax
				);

		glyph_bbox.xMin += pos[n].x;
		glyph_bbox.xMax += pos[n].x;
		glyph_bbox.yMin += pos[n].y;
		glyph_bbox.yMax += pos[n].y;

		if ( glyph_bbox.xMin < bbox.xMin )
			bbox.xMin = glyph_bbox.xMin;

		if ( glyph_bbox.yMin < bbox.yMin )
			bbox.yMin = glyph_bbox.yMin;

		if ( glyph_bbox.xMax > bbox.xMax )
			bbox.xMax = glyph_bbox.xMax;

		if ( glyph_bbox.yMax > bbox.yMax )
			bbox.yMax = glyph_bbox.yMax;
	}

	/* check that we really grew the string bbox */
	if ( bbox.xMin > bbox.xMax )
	{
		bbox.xMin = 0;
		bbox.yMin = 0;
		bbox.xMax = 0;
		bbox.yMax = 0;
	}
	
	dbp(0, "box: %ld,%ld,%ld,%ld\n", 
			bbox.xMin,
			bbox.xMax,
			bbox.yMin,
			bbox.yMax
		 );
	*ret = bbox;
}

static debug_dump;

static void _dump(int i, void *data, int w, int h)
{
	char path[256];
	sprintf(path, "/tmp/ft/%d.gray", i);
	FILE *fp = fopen(path, "wb+");
	fwrite(data, 1, w*h, fp);
	fclose(fp);
	char cmd[256];
	sprintf(cmd, "convert -depth 8 -size %dx%d /tmp/ft/%d.gray /tmp/ft/%d.png", 
			w, h, i, i);
	system(cmd);
}

int font_fill_texture(wchar_t *text, int font_size)
{
	_init();

	int error;
	FT_BBox  box;

	error = FT_Set_Char_Size(
			face,    /* handle to face object           */
			0,       /* char_width in 1/64th of points  */
			font_size*64,   /* char_height in 1/64th of points */
			300,     /* horizontal device resolution    */
			300 );   /* vertical device resolution      */
	if (error) {
		dbp(0, "setcharsize failed\n");
		return 1;
	}

	error = FT_Set_Pixel_Sizes(
			face,   /* handle to face object */
			0,      /* pixel_width           */
			font_size);   /* pixel_height          */
	if (error) {
		dbp(0, "setpixelsize failed\n");
		return 1;
	}

	int pen_x, pen_y;
	int n;
	int previous;

	pen_x = 0;
	pen_y = 0;
	num_glyphs = 0;
	previous = 0;

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

	for ( n = 0; n < wcslen(text); n++ )
	{
		FT_BBox glyph_bbox;
		FT_GlyphSlot slot = face->glyph;

		int glyph_index = FT_Get_Char_Index( face, text[n] );
		dbp(0, "idx: %d\n", glyph_index );

		if (previous && glyph_index) {
			FT_Vector  delta;
			FT_Get_Kerning( face, previous, glyph_index,
					FT_KERNING_DEFAULT, &delta );
			pen_x += delta.x >> 6;
		}
	
		error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
		if (error) 
			continue;

		error = FT_Get_Glyph( slot, &glyphs[num_glyphs] );
		if (error)
			continue;

		pos[num_glyphs].x = pen_x;
		pos[num_glyphs].y = pen_y;

		dbp(0, "pos %d,%d\n", pen_x, pen_y);

		pen_x += slot->advance.x >> 6;
		previous = glyph_index;
		num_glyphs++;
	}

	compute_string_bbox(&box);

	int w = box.xMax - box.xMin;
	int h = box.yMax - box.yMin;

	dbp(0, "tex: %dx%d\n", w, h);

	void *buf = malloc(w*h);
	memset(buf, 0, w*h);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, buf);
	free(buf);

	for ( n = 0; n < num_glyphs; n++ )
	{
		FT_Glyph g = glyphs[n];

		error = FT_Glyph_StrokeBorder(&g, stroker, 0, 0);
		dbp(0, "bmp: stroke %d\n", error);

		error = FT_Glyph_To_Bitmap(&g, FT_RENDER_MODE_NORMAL, 0, 0);
		dbp(0, "bmp: glyph2bmp %d\n", error);

		FT_BitmapGlyph gb = (FT_BitmapGlyph)g;
		FT_Bitmap bmp = gb->bitmap;

		dbp(0, "bmp: pos %ld left %d top %d\n", pos[n].x, gb->left, gb->top);
		dbp(0, "bmp: 	%dx%d\n", bmp.pitch, bmp.rows);
		dbp(0, "bmp: 	mode %d\n", bmp.pixel_mode);
		dbp(0, "bmp: 	8bit mode %d\n", FT_PIXEL_MODE_GRAY);
		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 
				pos[n].x + gb->left - box.xMin, box.yMax - gb->top, bmp.pitch, bmp.rows, 
				GL_ALPHA, GL_UNSIGNED_BYTE, bmp.buffer);

		_dump(n, bmp.buffer, bmp.pitch, bmp.rows);

		FT_Done_Glyph(g);
	}

	FT_Stroker_Done(stroker);

	for (n = 0; n < num_glyphs; n++)
		FT_Done_Glyph(glyphs[n]);

	return 0;
}

