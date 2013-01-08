#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "mp4enc.h"
#include "x264enc.h"

static int inited;

typedef struct {
	uint8_t audio_outbuf[10000];
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	AVFrame *audio_frm, *video_frm;
	float pos;
	void *x264;
	int64_t pts;
} mp4enc_t;

#define M(_m) ((mp4enc_t *)_m)

static int debug;

#define dbp(lev, ...) { \
	if (debug) \
		printf("mp4enc: " __VA_ARGS__);	\
	}

static int inited;

void mp4enc_loglevel(int lev)
{
	debug = lev;
}

static void _init() 
{
	if (!inited) {
		av_register_all();
		av_log_set_level(AV_LOG_DEBUG);
		inited++;
	}
}

static AVStream *add_video_stream(AVFormatContext *oc, enum CodecID codec_id, int w, int h)
{
	AVCodecContext *c;
	AVStream *st;
	AVCodec *codec;

	codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		dbp(0, "  cannot find video encoder\n");
		return NULL;
	}

	st = avformat_new_stream(oc, codec);
	if (!st) {
		dbp(0, "  Could not alloc video stream\n");
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_VIDEO;

	c->bit_rate = 400000;
	c->width = w;
	c->height = h;
	c->time_base.den = 25;
	c->time_base.num = 1;
	c->gop_size = 12; 
	c->pix_fmt = PIX_FMT_YUV420P;

	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

static AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id)
{
	AVCodecContext *c;
	AVStream *st;
	AVCodec *codec;

	codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		dbp(0, "  cannot find audio encoder\n");
		return NULL;
	}

	st = avformat_new_stream(oc, codec);
	if (!st) {
		dbp(0, "  Could not alloc audio stream\n");
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_AUDIO;

	c->sample_fmt = AV_SAMPLE_FMT_S16;
	c->bit_rate = 64000;
	c->sample_rate = 44100;
	c->channels = 2;

	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

static void open_audio(AVFormatContext *oc, AVStream *st)
{
	AVCodecContext *c;
	AVCodec *codec;

	c = st->codec;

	if (avcodec_open2(c, NULL, NULL) < 0) {
		dbp(0, "  could not open audio codec\n");
		return ;
	}
}

static void open_video(AVFormatContext *oc, AVStream *st)
{
	AVCodec *codec;
	AVCodecContext *c;

	c = st->codec;

	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) {
		dbp(0, "  video codec not found\n");
		return ;
	}

	if (avcodec_open2(c, NULL, NULL) < 0) {
		dbp(0, "  could not open video codec\n");
		return ;
	}
}

static void write_audio_frame(mp4enc_t *m, AVFormatContext *oc, AVStream *st, uint8_t *samples)
{
	AVCodecContext *c;
	AVPacket pkt;
	int r;
	int got;

	av_init_packet(&pkt);
	c = st->codec;

	pkt.data = m->audio_outbuf;
	pkt.size = sizeof(m->audio_outbuf);
	m->audio_frm->data[0] = samples;
	m->audio_frm->nb_samples = c->frame_size;
	r = avcodec_encode_audio2(c, &pkt, m->audio_frm, &got);
	dbp(0, "  audio_encode %d got=%d\n", r, got);

	if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
		pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
	pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = st->index;

	r = av_interleaved_write_frame(oc, &pkt);
	dbp(0, "  audio_frame %d size=%d\n", r, pkt.size);
}

static void write_video_frame(mp4enc_t *m, AVFormatContext *oc, AVStream *st, void **yuv, int *linesize)
{
	AVPacket pkt;
	int r, i, got;

	av_init_packet(&pkt);

	for (i = 0; i < 3; i++) {
		m->video_frm->data[i] = yuv[i];
		m->video_frm->linesize[i] = linesize[i];
	}
	m->video_frm->pts = m->pts;
	m->pts += 1;

	r = avcodec_encode_video2(m->video_st->codec, &pkt, m->video_frm, &got);
	dbp(0, "  video: encode r=%d got=%d\n", r, got);

	pkt.stream_index = st->index;

	r = av_interleaved_write_frame(oc, &pkt);
	dbp(0, "  video: write=%d\n", r);

	av_free_packet(&pkt);
}

void mp4enc_write_frame(void *_m, void **yuv, int *linesize, void *sample, int cnt)
{
	mp4enc_t *m = M(_m);
	int i;

	dbp(0, "write frame\n");

	write_video_frame(m, m->oc, m->video_st, yuv, linesize);

	for (i = 0; i < cnt; i++)
		write_audio_frame(m, m->oc, m->audio_st, (uint8_t *)sample + i*4096);
}

void *mp4enc_openfile(int w, int h, char *filename)
{
	mp4enc_t *m;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	int r;

	_init();

	dbp(0, "open %s\n", filename);

	m = (mp4enc_t *)malloc(sizeof(mp4enc_t));
	memset(m, 0, sizeof(mp4enc_t));
	
	fmt = av_guess_format("mp4", NULL, NULL);
	oc = avformat_alloc_context();
	oc->oformat = fmt;
	strcpy(oc->filename, filename);

	fmt->video_codec = CODEC_ID_H264;
	fmt->audio_codec = CODEC_ID_AAC;
	video_st = add_video_stream(oc, fmt->video_codec, w, h);
	audio_st = add_audio_stream(oc, fmt->audio_codec);

	open_video(oc, video_st);
	open_audio(oc, audio_st);

	av_dump_format(oc, 0, filename, 1);

	r = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
	dbp(0, "avio_open %d\n", r);

	avformat_write_header(oc, NULL);

	m->oc = oc;
	m->audio_st = audio_st;
	m->video_st = video_st;
	m->x264 = x264enc_new(w, h);
	m->audio_frm = avcodec_alloc_frame();
	m->video_frm = avcodec_alloc_frame();

	return m;
}


