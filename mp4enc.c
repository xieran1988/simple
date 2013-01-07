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

	char *filename = "/tmp/out.mp4";

	fmt = av_guess_format("mp4", NULL, NULL);
	oc = avformat_alloc_context();
	oc->oformat = fmt;
	strcpy(oc->filename, filename);

	fmt->audio_codec = CODEC_ID_AAC;
//	codec = avcodec_find_encoder();

	AVCodecContext *c;
	AVStream *st;
	AVCodec *codec;

	st = avformat_new_stream(oc, 0);
	c = st->codec;
	c->codec_id = CODEC_ID_AAC;
	c->codec_type = AVMEDIA_TYPE_AUDIO;

	/* put sample parameters */
	c->sample_fmt = AV_SAMPLE_FMT_S16;
	c->bit_rate = 64000;
	c->sample_rate = 44100;
	c->channels = 2;

	av_dump_format(oc, 0, filename, 1);

	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) {
		dbp(0, "unable find encoder\n");
	}

	int r = avcodec_open2(c, codec, NULL);
	if (r) {
		dbp(0, "codec open failed\n");
	}

	return 0;
}

