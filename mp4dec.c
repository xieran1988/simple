#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "mp4dec.h"

static int inited;

typedef struct {
	AVFormatContext *ifc;
	AVStream *st_h264;
	AVStream *st_aac;
	float dur;
	float h264_time_base;
	float aac_time_base;
	AVFrame *frm_h264;
	AVFrame *frm_aac;
} mp4dec_t;

#define M(_m) ((mp4dec_t *)_m)

static int debug;

#define dbp(lev, ...) { \
	if (debug) \
		printf("mp4dec: " __VA_ARGS__);	\
	}

void *mp4dec_open(char *fname)
{
	if (!inited) {
		av_register_all();
//	av_log_set_level(AV_LOG_DEBUG);
		inited++;
	}

	mp4dec_t *m = malloc(sizeof(mp4dec_t));
	memset(m, 0, sizeof(mp4dec_t));
	int i;
	dbp(0, "opening %s\n", fname);
	i = avformat_open_input(&m->ifc, fname, NULL, NULL);
	if (i) {
		dbp(0, "open %s failed err %d\n", fname, i);
		return NULL;
	}

	dbp(1, "nb_streams: %d\n", m->ifc->nb_streams);
	avformat_find_stream_info(m->ifc, NULL);
	for (i = 0; i < m->ifc->nb_streams; i++) {
		AVStream *st = m->ifc->streams[i];
		AVCodec *c = avcodec_find_decoder(st->codec->codec_id);
		if (!strcmp(c->name, "h264")) 
			m->st_h264 = st;
		if (!strcmp(c->name, "aac"))
			m->st_aac = st;
	}

	//st_h264->codec->debug |= FF_DEBUG_PICT_INFO;

	if (!m->st_h264) {
		dbp(0, "h264 stream not found\n");
		return NULL;
	}
	if (!m->st_aac) {
		dbp(0, "aac stream not found\n");
	}

	m->dur = m->ifc->duration/1e6;
	dbp(0, "dur: %f\n", m->dur);

	AVCodec *c = avcodec_find_decoder(m->st_h264->codec->codec_id);
	i = avcodec_open2(m->st_h264->codec, c, NULL);
	if (i) {
		dbp(0, "open h264 decoder failed\n");
		return NULL;
	}

	m->h264_time_base = m->st_h264->time_base.num*1.0/m->st_h264->time_base.den;
	dbp(0, "width: %d\n", m->st_h264->codec->width);
	dbp(0, "height: %d\n", m->st_h264->codec->height);
	dbp(0, "h264.time_base: %d,%d\n", 
			m->st_h264->time_base.num,
			m->st_h264->time_base.den);

	m->frm_h264 = avcodec_alloc_frame();

	AVCodec *c2 = avcodec_find_decoder(m->st_aac->codec->codec_id);
	i = avcodec_open2(m->st_aac->codec, c2, NULL);
	if (i) {
		dbp(0, "open aac decoder failed\n");
		return NULL;
	}

	m->aac_time_base = m->st_aac->time_base.num*1.0/m->st_aac->time_base.den;
	dbp(0, "aac.bit_rate: %d\n", m->st_aac->codec->bit_rate);
	dbp(0, "aac.sample_rate: %d\n", m->st_aac->codec->sample_rate);
	dbp(0, "aac.channels: %d\n", m->st_aac->codec->channels);
	dbp(0, "aac.frame_size: %d\n", m->st_aac->codec->frame_size);
	dbp(0, "aac.time_base: %d,%d\n", 
			m->st_aac->time_base.num,
			m->st_aac->time_base.den);

	m->frm_aac = avcodec_alloc_frame();

	return m;
}

int mp4dec_height(void *_m) 
{
	mp4dec_t *m = M(_m);
	return m->st_h264->codec->height;
}

int mp4dec_width(void *_m)
{
	mp4dec_t *m = M(_m);
	return m->st_h264->codec->width;
}

float mp4dec_dur(void *_m)
{
	mp4dec_t *m = M(_m);
	return m->dur;
}

float mp4dec_pos(void *_m)
{
	mp4dec_t *m = M(_m);
	return m->st_h264->cur_dts * m->h264_time_base;
}

static int _decode_h264_frame(mp4dec_t *m) 
{
	int i, n, got;

	n = 300;
	while (n--) {
		AVPacket pkt;
		i = av_read_frame(m->ifc, &pkt);
		if (i) {
			av_free_packet(&pkt);
			return 1;
		}
		if (pkt.stream_index == m->st_h264->index) {
			i = avcodec_decode_video2(m->st_h264->codec, m->frm_h264, &got, &pkt);
			dbp(1, "	h264, decode %d, got=%d key=%d dts=%lld pos=%.3f\n", 
					i, got, m->frm_h264->key_frame, pkt.dts, pkt.dts*m->h264_time_base);
			if (got) {
				av_free_packet(&pkt);
				return 0;
			}
		}
		if (pkt.stream_index == m->st_aac->index) {
			static int idx;
			i = avcodec_decode_audio4(m->st_aac->codec, m->frm_aac, &got, &pkt);
			int size = av_samples_get_buffer_size(NULL, 
						m->st_aac->codec->channels,
						m->frm_aac->nb_samples,
						m->st_aac->codec->sample_fmt, 1);
			dbp(1, "  aac, decode %d, got=%d dts=%lld pos=%.3f size=%d #%d\n", 
					i, got, pkt.dts, pkt.dts*m->aac_time_base, size, idx++);
		}
		av_free_packet(&pkt);
	}

	return 1;
}

void mp4dec_seek(void *_m, float pos)
{
	mp4dec_t *m = M(_m);
	int i;
	int64_t ts = (int64_t)(pos/m->h264_time_base);

	i = avformat_seek_file(m->ifc, m->st_h264->index, 0, ts, ts, 0);
	dbp(0, "seek %f\n", pos);

	while (1) {
		i = _decode_h264_frame(m);
		if (i || m->frm_h264->key_frame)
			break;
	}
	dbp(0, "	final pos %f\n", mp4dec_pos(m));
}

void mp4dec_seek_precise(void *_m, float pos)
{
	mp4dec_t *m = M(_m);

	mp4dec_seek(m, pos);
	int n = 300;
	while (n--) {
		if (mp4dec_pos(m) >= pos) 
			return ;
		_decode_h264_frame(m);
	}
}

int mp4dec_read_frame(void *_m, 
		void **data, int *linesize
		// void **audio, int audiosize
		)
{
	mp4dec_t *m = M(_m);
	int i;
		
	dbp(0, "read frame\n");

	i = _decode_h264_frame(m);
	if (i) 
		return i;

	if (data) {
		for (i = 0; i < 3; i++) {
			data[i] = m->frm_h264->data[i];
			linesize[i] = m->frm_h264->linesize[i];
		}
	}
	return 0;
}

static _dump_yuv(mp4dec_t *m, char *prefix)
{
	int *linesize = m->frm_h264->linesize;
	char path[256];
	FILE *fp;
	int i;
	int h = mp4dec_height(m);

	for (i = 0; i < 3; i++) {
		sprintf(path, "%s.%c", prefix, "YUV"[i]);
		fp = fopen(path, "wb+");
		fwrite(m->frm_h264->data[i], 1, linesize[i]*h, fp);
		fclose(fp);
	}
}

int main(int argc, char *argv[])
{
	void *m[4];
	void *data[3];
	int linesize[3];
	int i;

	debug = 1;

	for (i = 0; i < 1; i++) {
		char path[256];
		sprintf(path, "/vid/%d.mp4", i+1);
		m[i] = mp4dec_open(path);
		if (!m[i])
			return 0;
	}
	
	int n = 600;
	while (n--) {
		for (i = 0; i < 1; i++) {
			int r = mp4dec_read_frame(m[i], data, linesize);
			if (r)
				mp4dec_seek(m[i], 0);
		}
	}

	return 0;
}

