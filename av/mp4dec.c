#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
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
	float pos;
	float h264_time_base;
	float aac_time_base;
	AVFrame *frm_h264;
	AVFrame *frm_aac;

	// frame_size(1024)*sizeof(uint16_t)*nb_channels(2)
	// 	43 frames per second
#define SAMPMAX 100
#define SAMPSIZE 8192
	uint8_t samp[SAMPMAX][SAMPSIZE];
	float sampos[SAMPMAX];
	int sampt, sampcnt;

} mp4dec_t;

#define M(_m) ((mp4dec_t *)_m)

static int debug;

#define dbp(lev, ...) { \
	if (debug) \
		printf("mp4dec: " __VA_ARGS__);	\
	}

void mp4dec_loglevel(int lev)
{
	debug = lev;
	if (!lev)
		av_log_set_level(AV_LOG_ERROR);
}

static int _decode_audio_video_frame(mp4dec_t *m, int check, int exists);

static void _dump_audio(char *fname, void *data, int len)
{
	static int opened;
	static FILE *fp;

	if (!opened) {
		fp = fopen(fname, "wb+");
		opened++;
	}
	fwrite(data, 1, len, fp);
	fflush(fp);
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

void *mp4dec_open(char *fname)
{
	if (!inited) {
		av_register_all();
//		av_log_set_level(AV_LOG_DEBUG);
		inited++;
	}

	mp4dec_t *m = malloc(sizeof(mp4dec_t));
	AVCodec *c;

	memset(m, 0, sizeof(mp4dec_t));

	int i;
	dbp(0, "open %s\n", fname);
	i = avformat_open_input(&m->ifc, fname, NULL, NULL);
	if (i) {
		dbp(0, "  open %s failed err %d\n", fname, i);
		return NULL;
	}

	dbp(1, "  nb_streams: %d\n", m->ifc->nb_streams);
	avformat_find_stream_info(m->ifc, NULL);
	for (i = 0; i < m->ifc->nb_streams; i++) {
		AVStream *st = m->ifc->streams[i];
		c = avcodec_find_decoder(st->codec->codec_id);
		if (!strcmp(c->name, "h264")) 
			m->st_h264 = st;
		if (!strcmp(c->name, "aac"))
			m->st_aac = st;
	}

	//st_h264->codec->debug |= FF_DEBUG_PICT_INFO;

	if (!m->st_h264) {
		dbp(0, "  h264 stream not found\n");
		return NULL;
	}
	if (!m->st_aac) {
		dbp(0, "  aac stream not found\n");
		return NULL;
	}

	m->dur = m->ifc->duration/1e6;
	dbp(0, "  dur: %f\n", m->dur);

	c = avcodec_find_decoder(m->st_h264->codec->codec_id);
	i = avcodec_open2(m->st_h264->codec, c, NULL);
	if (i) {
		dbp(0, "  open h264 decoder failed\n");
		return NULL;
	}

	m->h264_time_base = m->st_h264->time_base.num*1.0/m->st_h264->time_base.den;
	dbp(0, "  width: %d\n", m->st_h264->codec->width);
	dbp(0, "  height: %d\n", m->st_h264->codec->height);
	dbp(0, "  h264.time_base: %d,%d %.3f\n", 
			m->st_h264->time_base.num,
			m->st_h264->time_base.den,
			m->h264_time_base);

	m->frm_h264 = avcodec_alloc_frame();

	c = avcodec_find_decoder(m->st_aac->codec->codec_id);
	i = avcodec_open2(m->st_aac->codec, c, NULL);
	if (i) {
		dbp(0, "  open aac decoder failed\n");
		return NULL;
	}

	m->aac_time_base = m->st_aac->time_base.num*1.0/m->st_aac->time_base.den;
	dbp(0, "  aac.bit_rate: %d\n", m->st_aac->codec->bit_rate);
	dbp(0, "  aac.sample_rate: %d\n", m->st_aac->codec->sample_rate);
	dbp(0, "  aac.channels: %d\n", m->st_aac->codec->channels);
	dbp(0, "  aac.frame_size: %d\n", m->st_aac->codec->frame_size);
	dbp(0, "  aac.sample_fmt: %s\n", 
						av_get_sample_fmt_name(m->st_aac->codec->sample_fmt));
	dbp(0, "  aac.sample_fmt.size: %d\n", 
						av_get_bytes_per_sample(m->st_aac->codec->sample_fmt));
	dbp(0, "  aac.sample.size: %d\n", 
		 				m->st_aac->codec->frame_size * 
						av_get_bytes_per_sample(m->st_aac->codec->sample_fmt) *
		 				m->st_aac->codec->channels 
						);
	dbp(0, "  aac.time_base: %d,%d\n", 
			m->st_aac->time_base.num,
			m->st_aac->time_base.den);

	if (m->st_aac->codec->channels != 2) {
		dbp(0, "  audio channels must be 2\n");
		return NULL;
	}
	if (m->st_aac->codec->sample_fmt != AV_SAMPLE_FMT_FLTP) {
		dbp(0, "  audio sample format is not fltp\n");
		return NULL;
	}
	if (m->st_aac->codec->sample_rate < 44100) {
		dbp(0, "  audio sample rate cannot < 44100\n");
		return NULL;
	}

	m->frm_aac = avcodec_alloc_frame();

	avformat_seek_file(m->ifc, m->st_h264->index, 0, 0, 0, 0);
	int n = 300;
	while (n-- && m->sampcnt < SAMPMAX) 
		_decode_audio_video_frame(m, 2, 2);
	avformat_seek_file(m->ifc, m->st_h264->index, 0, 0, 0, 0);

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
	return m->pos;
}

static int _decode_packet(mp4dec_t *m, AVPacket *pkt, int check, int exits)
{
	int i, got;

	if (pkt->stream_index == m->st_h264->index && (check & 1)) {
		i = avcodec_decode_video2(m->st_h264->codec, m->frm_h264, &got, pkt);
		m->pos = pkt->dts * m->h264_time_base;
		dbp(1, "  h264, decode %d, got=%d key=%d pos=%.3f pts=%lld dts=%lld\n", 
				i, got, m->frm_h264->key_frame, m->pos, pkt->pts, pkt->dts);
		if (!got)
			return 1;
		if (exits & 1)
			return 0;
	}

	if (pkt->stream_index == m->st_aac->index && (check & 2)) {
		i = avcodec_decode_audio4(m->st_aac->codec, m->frm_aac, &got, pkt);
		float pos = pkt->dts * m->aac_time_base;
		dbp(1, "  aac, decode %d, got=%d pos=%.3f [%d,%d]\n", 
				i, got, pos, m->sampt, m->sampcnt
				);
		if (!got)
			return 1;
		if (!m->sampcnt || pos > m->sampos[m->sampt]) {
			m->sampt++;
			m->sampt %= SAMPMAX;
			m->sampos[m->sampt] = pos;
			memcpy(m->samp[m->sampt], m->frm_aac->data[0], 4096);
			memcpy(m->samp[m->sampt]+4096, m->frm_aac->data[1], 4096);
			if (m->sampcnt < SAMPMAX) 
				m->sampcnt++;
			//_dump_audio("/tmp/out.fltp", data, SAMPSIZE);
		}
		if (exits & 2) 
			return 0;
	}

	return 1;
}

static int _decode_audio_video_frame(mp4dec_t *m, int check, int exits) 
{
	int n;

	n = 300;
	while (n--) {
		AVPacket pkt;
		if (av_read_frame(m->ifc, &pkt)) {
			av_free_packet(&pkt);
			return 1;
		} 
		if (!_decode_packet(m, &pkt, check, exits)) {
			av_free_packet(&pkt);
			return 0;
		}
		av_free_packet(&pkt);
	}
	return 1;
}

void mp4dec_seek_precise(void *_m, float pos)
{
	mp4dec_t *m = M(_m);
	int n;
	int64_t ts = (int64_t)(pos / m->h264_time_base);

	dbp(0, "seek %f\n", pos);

	avformat_seek_file(m->ifc, m->st_h264->index, 0, ts, ts, 0);
	m->sampcnt = 0;
	n = 300;
	while (n-- && (!m->sampcnt || m->sampos[m->sampt] < pos+1))
		_decode_audio_video_frame(m, 2, 2);

	avformat_seek_file(m->ifc, m->st_h264->index, 0, ts, ts, 0);
	n = 300;
	while (n--) {
		_decode_audio_video_frame(m, 3, 1);
		if (m->pos >= pos - 1./25) 
			return ;
	}
}

static void _fill_samples(mp4dec_t *m, float start, float end, void **sample, int *cnt)
{
	int i, j;
	int h = (m->sampt + SAMPMAX - m->sampcnt + 1) % SAMPMAX;

	*cnt = 0;
	for (i = h; i < h + m->sampcnt; i++) {
		j = i % SAMPMAX;
		if (m->sampos[j] > start && m->sampos[j] <= end) {
			if (*cnt < 2) {
				dbp(0, "  fill %p %d\n", m->samp[j], j);
				sample[(*cnt)++] = m->samp[j];
			}
		}
	}
}

int mp4dec_read_frame(void *_m, 
		void **data, int *line,
		void **sample, int *cnt
		)
{
	mp4dec_t *m = M(_m);
	int i;
		
	dbp(0, "read frame\n");

	if (sample) {
		float start = mp4dec_pos(m);
		float end = start + 1./25;
		_fill_samples(m, start, end, sample, cnt);
		dbp(0, "  samples [%.2f,%.2f] %d\n", start, end, *cnt);
	}

	i = _decode_audio_video_frame(m, 3, 1);
	dbp(0, "  ret %d\n", i);
	if (i) 
		return i;
	
	if (data) {
		for (i = 0; i < 3; i++) {
			data[i] = m->frm_h264->data[i];
			line[i] = m->frm_h264->linesize[i];
		}
	}

	return 0;
}

