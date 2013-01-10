#include <gl/vid/vid.h>
#include <gl/util/util.h>
#include <stdio.h>

static void _disp() 
{
	glLoadIdentity();
	glColor4f(0.3, 0.3, 0.3, 1);
	gl_draw_quads(0.3, 0.3, 0.1, 0.2, 0.2);
	gl_checkerr();
}

static void *v;

static void _init2()
{
	v = video_new(32, 32);
}

static void _disp2()
{
	static char y[32*32];
	static char u[16*16];
	static char v[16*16];
	void *data[3] = {y, u, v};
	int line[3] = {32*32, 16*16, 16*16};
	int i, j;

	for (i = 0; i < 32; i++)
		for (j = 0; j < 32; j++)
			y[i*32+j] = j*8;

	glLoadIdentity();
//	video_render_start(v, data, line);
	gl_draw_quads(0.5, 1, 0, 0.2, 0.2);
//	video_render_end(v);
}

int main(int argc, char *argv[]) 
{
	if (argc != 2)
		return 0;
	int sel = -1;
	sscanf(argv[1], "%d", &sel);

	if (sel == 1)
		test(300, 300, NULL, _disp);
	if (sel == 2)
		test(300, 300, _init2, _disp2);

	return 0;
}

