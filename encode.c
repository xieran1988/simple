#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x264.h>

typedef struct {
	x264_param_t param;
	x264_t *x;
	int64_t pts;
} h264_t ;

#define H(_m) ((h264_t *)_m)

#if 0
#define dbp(lev, ...) 
#else
#define dbp(lev, ...) printf("h264: " __VA_ARGS__)
#endif

void *h264_new(int width, int height)
{
	h264_t *h = malloc(sizeof(h264_t));

	memset(h, 0, sizeof(h264_t));
	x264_param_default_preset(&h->param, "ultrafast", "");
	h->param.i_log_level = X264_LOG_DEBUG;
	h->param.i_width = width;
	h->param.i_height = height;
	h->param.i_csp = X264_CSP_I420;

	h->x = x264_encoder_open(&h->param);
	dbp(0, "new: %p\n", h->x);

	return h;
}

void h264_encode(void *_h, void **data, int *linesize, void **buf, int *size)
{
	h264_t *h = H(_h);
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

	dbp(0, "h.x %p\n", h->x);
	dbp(0, "data %p,%p,%p\n", data[0], data[1], data[2]);
	dbp(0, "line %d,%d,%d\n", linesize[0], linesize[1], linesize[2]);

	picin.img.i_csp = X264_CSP_I420;
	picin.i_pts = h->pts;
	
	h->pts += 1000000;

	x264_nal_t *nal;
	int i_nal;
	r = x264_encoder_encode(h->x, &nal, &i_nal, &picin, &picout);
	dbp(0, "encode %d\n", r);
	dbp(0, "nal %p\n", nal);

	if (nal)
		*buf = nal[0].p_payload;
	*size = r;
}

