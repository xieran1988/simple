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


typedef struct {
	uint8_t audio_outbuf[10000];
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	AVFrame *audio_frm, *video_frm;
	char *opt;
	float tmstart;
	int vcnt, acnt;

	float dur;
	int hlscnt;
	char *hlspref;
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
	static int a;
	if (a++)
		return ;

	av_register_all();
	avformat_network_init();
	//av_log_set_level(AV_LOG_DEBUG);
}
	
const float afps = 44100./1024;

static inline float video_cnt2tm(int c) 
{
	return c/25.;
}

static inline float audio_cnt2tm(int c)
{
	return c/afps;
}

static inline int video_tm2cnt(float tm)
{
	return floor(tm/25)*25;
}

static inline float audio_tm2cnt(float tm)
{
	return floor(tm/afps)*afps;
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

	c->bit_rate = 100000;
	c->width = w;
	c->height = h;

	dbp(0, "  opt %s\n", m->opt);
	if (strstr(m->opt, "rtmp")) {
		c->time_base.den = 1000;
		c->time_base.num = 1;
	} else {
		c->time_base.den = 25;
		c->time_base.num = 1;
		dbp(0, "  non-rtmp set timebase 25/1\n");
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

	av_dict_set(&opt, "preset", "ultrafast", 0);
//	av_dict_set(&opt, "profile", "high", 0);
//	av_dict_set(&opt, "preset", "medium", 0);
//	av_dict_set(&opt, "tune", "zerolatency", 0);
	av_dict_set(&opt, "tune", "zerolatency", 0);
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
	m->audio_frm->linesize[0] = 4096;

	if (strstr(m->opt, "rtmp")) {
		m->audio_frm->pts = audio_cnt2tm(m->acnt)*1000;
	} else if (strstr(m->opt, "hls")) {
		m->audio_frm->pts = audio_cnt2tm(m->acnt)*100000;
	} else {
		m->audio_frm->pts = m->acnt*1024;
	}

	r = avcodec_encode_audio2(c, &pkt, m->audio_frm, &got);
	dbp(0, "  audio: encode %d got=%d size=%d dts=%lld pts=%lld->%lld key=%d\n", 
			r, got, pkt.size, pkt.dts, m->audio_frm->pts, pkt.pts, pkt.flags & AV_PKT_FLAG_KEY);

	if (pkt.size <= 0) {
		av_free_packet(&pkt);
		return ;
	}

	if (strstr(m->opt, "hls")) {
		pkt.pts = pkt.dts = audio_cnt2tm(m->acnt)*100000;
	}

	if (pkt.pts < 0) 
		pkt.dts = pkt.pts = 0;

	pkt.stream_index = st->index;

	r = av_interleaved_write_frame(oc, &pkt);

	av_free_packet(&pkt);
}

static int write_video_frame(mp4enc_t *m, AVFormatContext *oc, AVStream *st, void **yuv, int *linesize)
{
	AVPacket pkt = {};
	int r, i, got;

	for (i = 0; i < 3; i++) {
		m->video_frm->data[i] = yuv[i];
		m->video_frm->linesize[i] = linesize[i];
	}

	if (strstr(m->opt, "rtmp")) {
		m->video_frm->pts = video_cnt2tm(m->vcnt)*1000;
	} else if (strstr(m->opt, "hls")) {
		m->video_frm->pts = video_cnt2tm(m->vcnt)*100000;
	} else {
		m->video_frm->pts = m->vcnt;
	}

	av_init_packet(&pkt);

	avcodec_encode_video2(m->video_st->codec, &pkt, m->video_frm, &got);
	dbp(0, "  video: encode size=%d got=%d dts=%lld pts=%lld key=%d\n", 
			pkt.size, got, pkt.dts, pkt.pts, pkt.flags & AV_PKT_FLAG_KEY);

	if (pkt.size <= 0) {
		av_free_packet(&pkt);
		return 1;
	}

	if (strstr(m->opt, "hls") && 
			video_cnt2tm(m->vcnt) > (m->hlscnt+1) && 
			(pkt.flags & AV_PKT_FLAG_KEY)) 
	{
		m->hlscnt++;

		av_write_trailer(m->oc);
		avio_close(m->oc->pb);

		char path[256];
		sprintf(path, "%s/h%d.ts", m->hlspref, m->hlscnt);
		r = avio_open(&m->oc->pb, path, AVIO_FLAG_WRITE);
		dbp(0, "  avio_open %s tm %.2f\n", path, video_cnt2tm(m->vcnt));
		avformat_write_header(m->oc, NULL);
	}

	pkt.stream_index = st->index;
	r = av_interleaved_write_frame(oc, &pkt);

	av_free_packet(&pkt);
	return 0;
}

void mp4enc_getdelta(void *_m, float *vpos, float *apos)
{
	mp4enc_t *m = M(_m);

	float tm = tm_elapsed() - m->tmstart;

	if (vpos)
		*vpos = video_cnt2tm(m->vcnt) - tm;
	if (apos)
		*apos = audio_cnt2tm(m->acnt) - tm;
}

void mp4enc_write_frame(void *_m, void **yuv, int *linesize, void **sample, int cnt)
{
	mp4enc_t *m = M(_m);

	float vpos = video_cnt2tm(m->vcnt);
	float apos = audio_cnt2tm(m->acnt);

	dbp(0, "write frame: vpos %.2f apos %.2f\n", vpos, apos);
	//dbp(0, "  apos adjust to %.2f\n", apos);
	
	write_video_frame(m, m->oc, m->video_st, yuv, linesize);
	m->vcnt++;

	int i;
	for (i = 0; i < cnt; i++) {
		write_audio_frame(m, m->oc, m->audio_st, sample[i]);
		m->acnt++;
	}

	// prefix:114.64.255.28 114.64.254.226
}

void mp4enc_write_frame_rtmp(void *_m, void **yuv, int *linesize, void **sample, int cnt)
{
	mp4enc_t *m = M(_m);
	int i;

	float tm = tm_elapsed() - m->tmstart;
	float vpos = video_cnt2tm(m->vcnt);
	float apos = audio_cnt2tm(m->acnt);

	dbp(0, "write frame rtmp: tm %.2f vpos %.2f apos %.2f\n", 
			tm, vpos, apos);

	if (vpos < tm + 3) {
		if (vpos < tm - 3) 
			m->vcnt = video_tm2cnt(tm);
		write_video_frame(m, m->oc, m->video_st, yuv, linesize);
		m->vcnt++;
	}

	if (apos < tm + 3) {
		if (apos < tm - 3) 
			m->acnt = audio_tm2cnt(tm);
		/*
		static uint8_t dummy[2][8192];
		if (!cnt) {
			cnt = 2;
			sample[0] = dummy[0];
			sample[1] = dummy[1];
		}
		*/
		if (apos < vpos) {
			for (i = 0; i < cnt; i++) {
				write_audio_frame(m, m->oc, m->audio_st, sample[i]);
				m->acnt++;
			}
		}
	}
}

static mp4enc_t *_enc_alloc()
{
	mp4enc_t *m = (mp4enc_t *)malloc(sizeof(*m));
	memset(m, 0, sizeof(*m));
	return m;
}

static mp4enc_t *_enc_init(int w, int h, char *filename, char *opt, float dur)
{
	mp4enc_t *m;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	int r;

	_init();

	m = _enc_alloc();
	m->opt = opt;

	char *ext = "mp4";
	if (strstr(opt, "rtmp"))
		ext = "flv";
	if (strstr(opt, "ts"))
		ext = "mpegts";
	if (strstr(opt, "hls"))
		ext = "mpegts";

	dbp(0, "  fmtext: %s\n", ext);
	
	fmt = av_guess_format(ext, NULL, NULL);
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

	if (strstr(opt, "hls")) {

		m->hlspref = filename;

		char path[256];
		sprintf(path, "%s/h0.ts", m->hlspref);
		r = avio_open(&m->oc->pb, path, AVIO_FLAG_WRITE);
		dbp(0, "  avio_open %s\n", path);
		avformat_write_header(m->oc, NULL);

		sprintf(path, "%s/h.m3u8", m->hlspref);
		FILE *fp = fopen(path, "w+");
		fprintf(fp, "#EXTM3U\n");
		fprintf(fp, "#EXT-X-TARGETDURATION:1\n");
		fprintf(fp, "#EXT-X-MEDIA-SEQUENCE:1\n");
		int i;
		for (i = 0; i < ceil(dur); i++) {
			fprintf(fp, "#EXTINF:1,\n");
			fprintf(fp, "h%d.ts\n", i);
		}
			fprintf(fp, "#EXT-X-ENDLIST\n");
		fclose(fp);

	} else {
		r = avio_open(&m->oc->pb, filename, AVIO_FLAG_WRITE);
		dbp(0, "avio_open %d\n", r);
		avformat_write_header(m->oc, NULL);
	}

	if (strstr(opt, "rtmp")) {
		m->tmstart = tm_elapsed();
	}

	return m;
}

void *mp4enc_openrtmp(char *url, int w, int h)
{
	dbp(0, "openrtmp %s\n", url);
	mp4enc_t *m = _enc_init(w, h, url, "rtmp", 0);
	if (m) {
	}
	return m;
}

void *mp4enc_openfile(char *filename, int w, int h)
{
	dbp(0, "openfile %s\n", filename);
	return _enc_init(w, h, filename, "", 0);
}

void *mp4enc_opents(int w, int h)
{
	dbp(0, "opents\n");
	return _enc_init(w, h, "/tmp/a.ts", "ts", 0);
}

void *mp4enc_openhls(char *dir, int w, int h, float dur)
{
	dbp(0, "openhls\n");
	return _enc_init(w, h, dir, "hls", dur);
}

void mp4enc_close(void *_m)
{
	mp4enc_t *m = M(_m);

	av_write_trailer(m->oc);
	avio_close(m->oc->pb);
}

