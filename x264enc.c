#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <x264.h>

typedef struct {
	x264_param_t param;
	x264_t *x;
	int64_t pts;
} x264enc_t ;

#define H(_m) ((x264enc_t *)_m)

static int debug = 1;

#define dbp(lev, ...) { \
	if (debug) \
		printf("x264enc: " __VA_ARGS__);	\
}

void x264enc_loglevel(int lev)
{
	debug = lev;
}

void *x264enc_new(int width, int height)
{
	x264enc_t *h = (x264enc_t *)malloc(sizeof(x264enc_t));

	memset(h, 0, sizeof(x264enc_t));
	x264_param_default_preset(&h->param, "ultrafast", "");
	h->param.i_log_level = X264_LOG_DEBUG;
	h->param.i_width = width;
	h->param.i_height = height;
	h->param.i_csp = X264_CSP_I420;

	h->x = x264_encoder_open(&h->param);
	dbp(0, "new: %p\n", h->x);

	return h;
}

void x264enc_encode(void *_h, void **data, int *linesize, void **buf, int *size)
{
	x264enc_t *h = H(_h);
	x264_picture_t picin, picout;
	int r;

	memset(&picin, 0, sizeof(picin));
	memset(&picout, 0, sizeof(picout));

	picin.img.i_stride[0] = linesize[0];
	picin.img.i_stride[1] = linesize[1];
	picin.img.i_stride[2] = linesize[2];
	picin.img.plane[0] = data[0];
	picin.img.plane[1] = data[1];
	picin.img.plane[2] = data[2];

	picin.img.i_csp = X264_CSP_I420;
	picin.i_pts = h->pts;

	x264_nal_t *nal;
	int i_nal;
	r = x264_encoder_encode(h->x, &nal, &i_nal, &picin, &picout);
	dbp(0, "encode %d nal=%p\n", r, nal);

	if (nal) {
		*buf = nal[0].p_payload;
		*size = r;
	} else {
		*buf = NULL;
		*size = 0;
	}
}

