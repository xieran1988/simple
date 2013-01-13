
#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/dict.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include <av/util/a.h>

#include "a.h"

static int inited;

typedef struct {
	uint8_t audio_outbuf[10000];
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	AVFrame *audio_frm, *video_frm;
	float pos;
	int64_t pts;
	int frmcnt;
	char *opt;
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
	if (!lev) 
		av_log_set_level(AV_LOG_ERROR);
}

static void _init() 
{
	if (!inited) {
		av_register_all();
		avformat_network_init();
	//	av_log_set_level(AV_LOG_DEBUG);
		inited++;
	}
}

static AVStream *add_video_stream(mp4enc_t *m, AVFormatContext *oc, enum CodecID codec_id, int w, int h)
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
	if (strstr(m->opt, "rtmp")) {
		c->time_base.den = 1000;
		c->time_base.num = 1;
	} else {
		c->time_base.den = 25;
		c->time_base.num = 1;
	}
	c->gop_size = 12; 
	c->pix_fmt = PIX_FMT_YUV420P;

	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

static AVStream *add_audio_stream(mp4enc_t *m, AVFormatContext *oc, enum CodecID codec_id)
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

	c->sample_fmt = AV_SAMPLE_FMT_FLTP;
	c->bit_rate = 64000;
	c->sample_rate = 44100;
	c->time_base.den = 1000;
	c->time_base.num = 1;
	c->channels = 2;

	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

static int open_audio(AVFormatContext *oc, AVStream *st)
{
	AVCodecContext *c;
	AVCodec *codec;

	c = st->codec;
  c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	if (avcodec_open2(c, NULL, NULL) < 0) {
		dbp(0, "  could not open audio codec\n");
		return 1;
	}
	return 0;
}

static int open_video(AVFormatContext *oc, AVStream *st)
{
	AVCodec *codec;
	AVCodecContext *c;
	AVDictionary *opt = NULL;

	c = st->codec;

	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) {
		dbp(0, "  video codec not found\n");
		return 1;
	}

//	av_dict_set(&opt, "preset", "ultrafast", 0);
	av_dict_set(&opt, "profile", "high", 0);
//	av_dict_set(&opt, "preset", "medium", 0);
//	av_dict_set(&opt, "tune", "zerolatency", 0);
	if (avcodec_open2(c, NULL, &opt) < 0) {
		dbp(0, "  could not open video codec\n");
		return 1;
	}

	return 0;
}

static void write_audio_frame(mp4enc_t *m, AVFormatContext *oc, AVStream *st, uint8_t *samples)
{
	AVCodecContext *c;
	AVPacket pkt = {};
	int r;
	int got;

	av_init_packet(&pkt);
	c = st->codec;

	pkt.data = m->audio_outbuf;
	pkt.size = sizeof(m->audio_outbuf);

	m->audio_frm->nb_samples = 1024;
	m->audio_frm->extended_data = m->audio_frm->data;
	m->audio_frm->data[0] = samples;
	m->audio_frm->data[1] = samples + 4096;
	m->audio_frm->linesize[0] = 8192;
	dbp(0, "  audio: write %p %p %d,%d\n", samples, 
			m->audio_frm,
			m->audio_frm->linesize[0],
			m->audio_frm->linesize[1]
			);

	r = avcodec_encode_audio2(c, &pkt, m->audio_frm, &got);
	dbp(0, "  audio: encode %d got=%d size=%d\n", r, got, pkt.size);

//	if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
//		pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
	if (strstr(m->opt, "rtmp")) {
		pkt.pts = m->pts;
		m->pts += 1000/43;
	}

	dbp(0, "  audio: pts=%lld\n", pkt.pts);
	//pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = st->index;

	r = av_interleaved_write_frame(oc, &pkt);
	dbp(0, "  audio: frame %d size=%d\n", r, pkt.size);
}

void mp4enc_setpts(void *_m, float pos)
{
	mp4enc_t *m = M(_m);
	m->pts = (int64_t)(pos*1000);
}

static void write_video_frame(mp4enc_t *m, AVFormatContext *oc, AVStream *st, void **yuv, int *linesize)
{
	AVPacket pkt = {};
	int r, i, got;

	for (i = 0; i < 3; i++) {
		m->video_frm->data[i] = yuv[i];
		m->video_frm->linesize[i] = linesize[i];
	}

	if (strstr(m->opt, "rtmp")) {
		m->video_frm->pts = m->pts;
	}
//	m->pts = (int)(m->frmcnt*1000./24);
//	m->frmcnt++;

	av_init_packet(&pkt);

	avcodec_encode_video2(m->video_st->codec, &pkt, m->video_frm, &got);
	dbp(0, "  video: encode size=%d got=%d dts=%lld pts=%lld\n", 
			pkt.size, got, pkt.dts, pkt.pts);

	if (pkt.size > 0) {
		pkt.stream_index = st->index;
		r = av_interleaved_write_frame(oc, &pkt);
		dbp(0, "  video: write=%d\n", r);
		av_free_packet(&pkt);
	}
}

void mp4enc_write_frame(void *_m, void **yuv, int *linesize, void **sample, int cnt)
{
	mp4enc_t *m = M(_m);
	int i;

	dbp(0, "write frame: sample=%d\n", cnt);

	write_video_frame(m, m->oc, m->video_st, yuv, linesize);

	static uint8_t dummy[2][8192];
	if (!cnt) {
		write_audio_frame(m, m->oc, m->audio_st, dummy[0]);
		write_audio_frame(m, m->oc, m->audio_st, dummy[1]);
	} else {
		for (i = 0; i < cnt; i++) {
			write_audio_frame(m, m->oc, m->audio_st, sample[i]);
		}
	}
}

static mp4enc_t *_enc_init(int w, int h, char *filename, char *opt)
{
	mp4enc_t *m;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	int r;

	_init();

	m = (mp4enc_t *)malloc(sizeof(mp4enc_t));
	memset(m, 0, sizeof(mp4enc_t));
	
	m->opt = opt;
	
	fmt = av_guess_format(
			strncmp(filename, "rtmp", 4) ? "mp4" : "flv", 
			NULL, NULL);
	dbp(0, "  guessed fmt: %p\n", fmt);
	if (!fmt) {
		dbp(0, "  cannot get fmt\n");
		return NULL;
	}

	oc = avformat_alloc_context();
	oc->oformat = fmt;
	strcpy(oc->filename, filename);

	fmt->video_codec = CODEC_ID_H264;
	video_st = add_video_stream(m, oc, fmt->video_codec, w, h);
	if (!video_st)
		return NULL;

	fmt->audio_codec = CODEC_ID_AAC;
	audio_st = add_audio_stream(m, oc, fmt->audio_codec);
	if (!audio_st)
		return NULL;

	if (open_video(oc, video_st))
		return NULL;

	if (open_audio(oc, audio_st))
		return NULL;

	av_dump_format(oc, 0, filename, 1);

	m->oc = oc;
	m->audio_st = audio_st;
	m->video_st = video_st;
	m->audio_frm = avcodec_alloc_frame();
	m->video_frm = avcodec_alloc_frame();

	r = avio_open(&m->oc->pb, filename, AVIO_FLAG_WRITE);
	dbp(0, "avio_open %d\n", r);
	avformat_write_header(m->oc, NULL);

	return m;
}

void *mp4enc_openrtmp(char *url, int w, int h)
{
	dbp(0, "openrtmp %s\n", url);
	return _enc_init(w, h, url, "rtmp");
}

void *mp4enc_openfile(char *filename, int w, int h)
{
	dbp(0, "openfile %s\n", filename);
	return _enc_init(w, h, filename, "");
}

void mp4enc_close(void *_m)
{
	mp4enc_t *m = M(_m);

	av_write_trailer(m->oc);
	avio_close(m->oc->pb);
}

