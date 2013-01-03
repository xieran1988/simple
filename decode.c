#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "decode.h"

static int inited;

typedef struct {
	AVFormatContext *ifc;
	AVStream *st_h264;
	float dur;
	float tm_base;
	AVFrame *frm;
} mp4_t;

#define M(_m) ((mp4_t *)_m)

#if 0
#define dbp(lev, ...) 
#else
#define dbp(lev, ...) printf("mp4: " __VA_ARGS__)
#endif

void *mp4_open(char *fname)
{
	if (!inited) {
		av_register_all();
//	av_log_set_level(AV_LOG_DEBUG);
		inited++;
	}

	mp4_t *m = malloc(sizeof(mp4_t));
	memset(m, 0, sizeof(mp4_t));
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
	}
	if (!m->st_h264) {
		dbp(0, "h264 stream not found\n");
		return NULL;
	}

	m->dur = m->ifc->duration/1e6;
	dbp(0, "dur: %f\n", m->dur);

	AVCodec *c = avcodec_find_decoder(m->st_h264->codec->codec_id);
	i = avcodec_open2(m->st_h264->codec, c, NULL);
	if (i) {
		dbp(0, "open decoder failed\n");
		return NULL;
	}
	dbp(0, "width: %d\n", m->st_h264->codec->width);
	dbp(0, "height: %d\n", m->st_h264->codec->height);
	//st_h264->codec->debug |= FF_DEBUG_PICT_INFO;
	
	m->tm_base = m->st_h264->time_base.num*1.0/m->st_h264->time_base.den;
	dbp(0, "base: %d,%d\n", 
			m->st_h264->time_base.num,
			m->st_h264->time_base.den);
	
	m->frm = avcodec_alloc_frame();

	return m;
}

int mp4_height(void *_m) 
{
	mp4_t *m = M(_m);
	return m->st_h264->codec->height;
}

int mp4_width(void *_m)
{
	mp4_t *m = M(_m);
	return m->st_h264->codec->width;
}

float mp4_dur(void *_m)
{
	mp4_t *m = M(_m);
	return m->dur;
}

float mp4_pos(void *_m)
{
	mp4_t *m = M(_m);
	return m->st_h264->cur_dts * m->tm_base;
}

static int _decode_h264_frame(mp4_t *m) 
{
	int i, n, gotpic;

	n = 300;
	while (n--) {
		AVPacket pkt;
		i = av_read_frame(m->ifc, &pkt);
		if (i) {
			av_free_packet(&pkt);
			return 1;
		}
		if (pkt.stream_index != m->st_h264->index) {
			av_free_packet(&pkt);
			continue;
		}
		i = avcodec_decode_video2(m->st_h264->codec, m->frm, &gotpic, &pkt);
		dbp(1, "	idx=%d, decode %d, gotpic=%d key=%d\n", 
				pkt.stream_index, i, gotpic, m->frm->key_frame);
		if (gotpic) {
			av_free_packet(&pkt);
			return 0;
		}
		av_free_packet(&pkt);
	}
	return 1;
}

void mp4_seek(void *_m, float pos)
{
	mp4_t *m = M(_m);
	int i;
	int64_t ts = (int64_t)(pos/m->tm_base);

	i = avformat_seek_file(m->ifc, m->st_h264->index, 0, ts, ts, 0);
	dbp(0, "seek %f\n", pos);

	while (1) {
		i = _decode_h264_frame(m);
		if (i || m->frm->key_frame)
			break;
	}
	dbp(0, "	final pos %f\n", mp4_pos(m));
}

void mp4_seek_precise(void *_m, float pos)
{
	mp4_t *m = M(_m);

	mp4_seek(m, pos);
	int n = 300;
	while (n--) {
		if (mp4_pos(m) >= pos) 
			return ;
		_decode_h264_frame(m);
	}
}

int mp4_read_frame(void *_m, void **data, int *linesize)
{
	mp4_t *m = M(_m);
	int i;
		
	dbp(0, "read frame\n");

	i = _decode_h264_frame(m);
	if (i) 
		return i;

	if (data) {
		for (i = 0; i < 3; i++) {
			data[i] = m->frm->data[i];
			linesize[i] = m->frm->linesize[i];
		}
	}
	return 0;
}

static _dump_yuv(mp4_t *m, char *prefix)
{
	int *linesize = m->frm->linesize;
	char path[256];
	FILE *fp;
	int i;
	int h = mp4_height(m);

	for (i = 0; i < 3; i++) {
		sprintf(path, "%s.%c", prefix, "YUV"[i]);
		fp = fopen(path, "wb+");
		fwrite(m->frm->data[i], 1, linesize[i]*h, fp);
		fclose(fp);
	}
}

int main(int argc, char *argv[])
{
	void *m[4];
	void *data[3];
	int linesize[3];
	int i;

	for (i = 0; i < 4; i++) {
		char path[256];
		sprintf(path, "/vid/%d.mp4", i+1);
		m[i] = mp4_open(path);
	}
	
	while (1) {
		for (i = 0; i < 4; i++) {
			int r = mp4_read_frame(m[i], data, linesize);
			if (r)
				mp4_seek(m[i], 0);
		}
	}

	return 0;
}

