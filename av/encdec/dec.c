
#include "a.h"
#include "av.h"

typedef struct {
	AVFormatContext *ifc;
	AVStream *st_video;
	AVStream *st_audio;

	AVFrame *frm_video;
	AVFrame *frm_audio;

	float dur;
	float pos;

	float video_time_base;
	float audio_time_base;

#define SAMPMAX 120
#define SAMPSIZE 8192
	uint8_t samp[SAMPMAX][SAMPSIZE];
	float sampos[SAMPMAX];
	float samptpos, samphpos;
	int samph, sampt, sampcnt;

	char *opt;

	int vcnt, acnt;
} mp4dec_t;

#define M(_m) ((mp4dec_t *)_m)

static int debug;

#define dbp(lev, ...) { \
	if (debug) \
		printf("mp4dec: " __VA_ARGS__);	\
	}

static void _init() 
{
	static int a;
	if (a++)
		return ;

	av_register_all();
	av_log_set_level(AV_LOG_ERROR);
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
	static FILE *fp;
	if (!fp) 
		fp = fopen(fname, "wb+");
	fwrite(data, 1, len, fp);
	fflush(fp);
}

static void _dump_yuv(mp4dec_t *m, char *prefix)
{
	int *linesize = m->frm_video->linesize;
	char path[256];
	FILE *fp;
	int i;
	int h = mp4dec_height(m);

	for (i = 0; i < 3; i++) {
		sprintf(path, "%s.%c", prefix, "YUV"[i]);
		fp = fopen(path, "wb+");
		fwrite(m->frm_video->data[i], 1, linesize[i]*h, fp);
		fclose(fp);
	}
}

static int open_video(mp4dec_t *m)
{
	AVCodec *c;
	int i;

	c = avcodec_find_decoder(m->st_video->codec->codec_id);
	i = avcodec_open2(m->st_video->codec, c, NULL);
	if (i) {
		dbp(0, "  open video decoder failed\n");
		return 1;
	}

	m->video_time_base = m->st_video->time_base.num*1.0/m->st_video->time_base.den;
	dbp(0, "  video.width: %d\n", m->st_video->codec->width);
	dbp(0, "  video.height: %d\n", m->st_video->codec->height);
	dbp(0, "  video.time_base: %d,%d %.3f\n", 
			m->st_video->time_base.num,
			m->st_video->time_base.den,
			m->video_time_base);

	m->frm_video = avcodec_alloc_frame();

	return 0;
}

static int open_audio(mp4dec_t *m)
{
	AVCodec *c;
	int i;

	c = avcodec_find_decoder(m->st_audio->codec->codec_id);
	i = avcodec_open2(m->st_audio->codec, c, NULL);
	if (i) {
		dbp(0, "  open audio decoder failed\n");
		return 1;
	}

	m->audio_time_base = m->st_audio->time_base.num*1.0/m->st_audio->time_base.den;
	dbp(0, "  audio.bit_rate: %d\n", m->st_audio->codec->bit_rate);
	dbp(0, "  audio.sample_rate: %d\n", m->st_audio->codec->sample_rate);
	dbp(0, "  audio.channels: %d\n", m->st_audio->codec->channels);
	dbp(0, "  audio.frame_size: %d\n", m->st_audio->codec->frame_size);
	dbp(0, "  audio.sample_fmt: %s\n", 
						av_get_sample_fmt_name(m->st_audio->codec->sample_fmt));
	dbp(0, "  audio.sample_fmt.size: %d\n", 
						av_get_bytes_per_sample(m->st_audio->codec->sample_fmt));
	dbp(0, "  audio.sample.size: %d\n", 
		 				m->st_audio->codec->frame_size * 
						av_get_bytes_per_sample(m->st_audio->codec->sample_fmt) *
		 				m->st_audio->codec->channels 
						);
	dbp(0, "  audio.time_base: %d,%d\n", 
			m->st_audio->time_base.num,
			m->st_audio->time_base.den);

	if (m->st_audio->codec->channels != 2) {
		dbp(0, "  audio channels must be 2\n");
		return 1;
	}
	if (m->st_audio->codec->sample_fmt != AV_SAMPLE_FMT_FLTP) {
		dbp(0, "  audio sample format is not fltp\n");
		return 1;
	}
	if (m->st_audio->codec->sample_rate < 44100) {
		dbp(0, "  audio sample rate cannot < 44100\n");
		return 1;
	}

	m->frm_audio = avcodec_alloc_frame();

	return 0;
}

static void _prefetch_audio(mp4dec_t *m, float pos)
{
	dbp(0, "  prefetch audio to %.2f\n", pos);
	m->sampcnt = 0;
	m->samph = m->sampt = -1;
	int n = 300;
	while (n-- && m->sampcnt < SAMPMAX) {
		if (_decode_audio_video_frame(m, 2, 2))
			break;
		if (m->sampt >= 0 && m->sampos[m->sampt%SAMPMAX] >= pos)
			break;
	}
	dbp(0, "  prefetch done\n");
}

void mp4dec_set(void *_m, char *opt)
{
	mp4dec_t *m = (mp4dec_t *)_m;

	if (strstr(opt, "novideo"))
		m->st_video = NULL;
}

void *mp4dec_open(char *fname)
{

	_init();

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
		if (!c)
			continue;
		if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			dbp(1, "  audio: %s\n", c->name);
			m->st_audio = st;
		}
		if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			dbp(1, "  video: %s\n", c->name);
			m->st_video = st;
		}
	}

	//st_video->codec->debug |= FF_DEBUG_PICT_INFO;

	if (!m->st_video && !m->st_audio) {
		dbp(0, "  both video and audio stream not found\n");
		return NULL;
	}

	m->dur = m->ifc->duration/1e6;
	dbp(0, "  duration: %f s\n", m->dur);

	if (m->st_video && open_video(m))
		return NULL;

	if (m->st_audio && open_audio(m))
		return NULL;

	avformat_seek_file(m->ifc, 0, 0, 0, 0, 0);
	_prefetch_audio(m, 1);
	avformat_seek_file(m->ifc, 0, 0, 0, 0, 0);

	return m;
}

int mp4dec_height(void *_m) 
{
	mp4dec_t *m = M(_m);
	return m->st_video->codec->height;
}

int mp4dec_width(void *_m)
{
	mp4dec_t *m = M(_m);
	return m->st_video->codec->width;
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

	if (m->st_video && 
		  pkt->stream_index == m->st_video->index && 
			(check & 1)) 
	{
		i = avcodec_decode_video2(m->st_video->codec, m->frm_video, &got, pkt);
		m->pos = pkt->dts * m->video_time_base;
		dbp(1, "  video: decode %d, got=%d key=%d pos=%.3f pts=%lld dts=%lld\n", 
				i, got, m->frm_video->key_frame, m->pos, pkt->pts, pkt->dts);
		if (!got)
			return 1;
		if (exits & 1)
			return 0;
	}

	if (m->st_audio && 
		  pkt->stream_index == m->st_audio->index && 
			(check & 2)) 
	{
		i = avcodec_decode_audio4(m->st_audio->codec, m->frm_audio, &got, pkt);
		float pos = pkt->dts * m->audio_time_base;
		dbp(1, "  audio: decode %d got=%d pos=%.3f pts=%lld "
					 "[%d,%d] [%.2f,%.2f] %d\n", 
				i, got, pos, pkt->pts,
				m->samph, m->sampt, 
				m->sampos[m->samph%SAMPMAX], 
				m->sampos[m->sampt%SAMPMAX],
				m->sampcnt
				);
		if (!got)
			return 1;
		dbp(0, "  audio: size %d,%d\n", 
				m->frm_audio->linesize[0],
				m->frm_audio->nb_samples
				);
		if (!m->sampcnt || pos > m->sampos[m->sampt%SAMPMAX]) {
			m->sampt++;
			m->sampos[m->sampt%SAMPMAX] = pos;
			uint8_t *p = (uint8_t *)m->samp[m->sampt%SAMPMAX];
			memcpy(p, m->frm_audio->data[0], 4096);
			memcpy(p + 4096, m->frm_audio->data[1], 4096);
			if (m->sampcnt < SAMPMAX)
				m->sampcnt++;
			m->samph = m->sampt-m->sampcnt+1;
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
	int64_t ts;
	int index;

	dbp(0, "seek %f\n", pos);

	if (m->st_video) {
		ts = (int64_t)(pos / m->video_time_base);
		index = m->st_video->index;

		avformat_seek_file(m->ifc, index, 0, ts, ts, 0);
		_prefetch_audio(m, pos+1);
		avformat_seek_file(m->ifc, index, 0, ts, ts, 0);

		n = 300;
		while (n--) {
			_decode_audio_video_frame(m, 3, 1);
			if (m->pos >= pos - 1./25) 
				return ;
		}
	} else {
		ts = (int64_t)(pos / m->audio_time_base);
		index = m->st_audio->index;
		avformat_seek_file(m->ifc, index, 0, ts, ts, 0);
		_prefetch_audio(m, pos+1);
		avformat_seek_file(m->ifc, index, 0, ts, ts, 0);
		m->pos = pos;
	}
}

static float _fill_samples(mp4dec_t *m, float start, float end, void **sample, int *cnt)
{
	int i, j;
	float ret = start;

	*cnt = 0;
	for (i = m->samph; i < m->sampt; i++) {
		j = i % SAMPMAX;
		if (m->sampos[j] > start && m->sampos[j] <= end) {
			if (*cnt < 2) {
				sample[(*cnt)++] = m->samp[j];
				ret = m->sampos[j];
			}
		}
	}
	return ret;
}

int mp4dec_read_frame(void *_m, 
		void **data, int *line,
		void **_sample, int *_cnt
		)
{
	mp4dec_t *m = M(_m);
	int i, r;
		
	dbp(0, "read frame\n");

	if (m->st_video && data) {
		i = _decode_audio_video_frame(m, 3, 1);
		dbp(0, "  ret %d\n", i);
		if (i) 
			return i;
		for (i = 0; i < 3; i++) {
			data[i] = m->frm_video->data[i];
			line[i] = m->frm_video->linesize[i];
		}
		m->vcnt++;
		r = 0;
	}
	
	if (m->st_audio && !m->st_video && _sample) {
		float start = m->pos;
		float end = m->pos + 1./25;
		while (!m->sampcnt || m->sampos[m->sampt%SAMPMAX] < end)
			if (_decode_audio_video_frame(m, 2, 2)) {
				r = 1;
				goto ret;
			}
		_fill_samples(m, start, end, _sample, _cnt);
		m->pos = end;
		dbp(0, "  samples [%.2f,%.2f] %d\n", start, end, *_cnt);
		r = 0;
	}

	if (m->st_audio && m->st_video && _sample) {
		void *sample[2];
		int cnt;
		float start = mp4dec_pos(m);
		float end = start + 1/25.;
		_fill_samples(m, start, end, sample, &cnt);
		dbp(0, "  samples [%.2f,%.2f] %d\n", start, end, cnt);
		memcpy(_sample, sample, sizeof(sample));
		*_cnt = cnt;
		m->acnt += cnt;
		r = 0;
	}

	dbp(0, "  apos %.2lf vpos %.2lf\n",
		 m->vcnt/25., m->acnt*1024/44100.);

ret:
	return r;
}

