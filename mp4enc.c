#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "mp4enc.h"

static int inited;

typedef struct {
} mp4enc_t;

#define M(_m) ((mp4enc_t *)_m)

static int debug;

#define dbp(lev, ...) { \
	if (debug) \
		printf("mp4enc: " __VA_ARGS__);	\
	}

static int inited;

int main(int argc, char *argv[])
{
	AVOutputFormat *fmt;
	AVFormatContext *oc;

	debug = 1;

	if (!inited) {
		av_register_all();
		inited++;
	}

	fmt = av_guess_format("mp4", NULL, NULL);
	dbp(0, "fmt %p\n", fmt);
	oc = avformat_alloc_context();
	oc->oformat = fmt;
	strcpy(oc->filename, "/tmp/out.mp4");

//	codec = avcodec_find_encoder();

	return 0;
}

