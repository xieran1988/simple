#include <stdio.h>
#include <libavcodec/avcodec.h>
//#include <libavcodec/h264.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <jpeglib.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <x264.h>

typedef struct {
	AVFormatContext *ifc ;
	AVFormatContext *ofc;
	char *name;
	FILE *fp;
} node_t ;

node_t n1, n2, n3, n4;

static void _write_packet(node_t *n, uint8_t *buf, int len)
{
	fwrite(buf, 1, len, n->fp);
}

static int write_packet(node_t *n, uint8_t *buf, int len)
{
//	printf("write_packet: %s %d\n", n->name, len);
	int i;
	fwrite(buf, 1, len, n->fp);
	for (i = 0; i < len; i += 188) {
		//_write_packet(n, buf + i, len);
	}
	return 0;
}

static int write_packet1(void *_d, uint8_t *buf, int len)
{
	return write_packet(&n3, buf, len);
}

static int write_packet2(void *_d, uint8_t *buf, int len)
{
	return write_packet(&n4, buf, len);
}

static AVFormatContext *ifc;
static AVStream *st_h264;
static char opt_preview;
static char *input_filename, *output_filename;
static float preview_pos = 0.4;
static AVFormatContext *ofc;

static void output_init()
{
	int i, err;

	AVCodec *c[2];
	char *guess_fp = "a.flv";

//	AVStream *st = ;
	ofc = avformat_alloc_context();
	AVOutputFormat *ofmt = av_guess_format(NULL, guess_fp, NULL);
	ofc->oformat = ofmt;
	strcpy(ofc->filename, guess_fp);
	printf("ofc ok\n");

	AVStream *ost[2];
	ost[0] = avformat_new_stream(ofc, NULL);
	//ost[0]->codec->codec_id = ist[0]->codec->codec_id;
	AVCodecContext *codec[2];
	codec[0] = ost[0]->codec;
	avcodec_get_context_defaults3(codec[0], NULL);
	codec[0]->codec_id = AV_CODEC_ID_H264;
	codec[0]->codec_type = AVMEDIA_TYPE_VIDEO;
	codec[0]->time_base.num = 1;
	codec[0]->time_base.den = 25;
	codec[0]->width = 720;
	codec[0]->height = 360;
	codec[0]->codec_tag = av_codec_get_tag(ofmt->codec_tag, AV_CODEC_ID_H264);
	printf("ofc->codec codec[0]=%p ofc->st[0]->codec=%p\n", codec[0], ofc->streams[0]->codec);
	printf("codec[0].tag=%d\n", codec[0]->codec_tag);

	err = avio_open2(&ofc->pb, output_filename, AVIO_FLAG_WRITE, NULL, NULL);
	printf("open2=%d\n", err);

	printf("write header\n");
	printf("ofc.nb_streams=%d\n", ofc->nb_streams);
	printf("ofc.st[0]=%p\n", ofc->streams[0]);
	printf("ofc.st[0].codec=%p\n", ofc->streams[0]->codec);
	printf("ofc.oformat=%p\n", ofc->oformat);
	printf("ofc.oformat.write_header=%p\n", ofc->oformat->write_header);
	printf("ofc.oformat.name=%s\n", ofc->oformat->name);
	printf("ofc.pb=%p\n", ofc->pb);
	printf("ofc.st[0].avg_frame_rate={%d,%d}\n", 
			ofc->streams[0]->avg_frame_rate.num,
			ofc->streams[0]->avg_frame_rate.den
			);
	printf("ofc.st[0].codec.timebase={%d,%d}\n", 
			ofc->streams[0]->codec->time_base.num,
			ofc->streams[0]->codec->time_base.den
			);
	printf("ofc.priv=%p\n", ofc->priv_data);

	//err = ofc->oformat->write_header(ofc);
	err = avformat_write_header(ofc, NULL);
	printf("write_header=%d\n", err);
	avio_flush(ofc->pb);
}

void node_read_packet(node_t *n)
{
	int i;
	AVPacket pkt;
	i = av_read_frame(n->ifc, &pkt);
	if (i < 0) 
		return ;
	if (pkt.stream_index == 0) {
		if (!strcmp(n->name, "src1"))
			printf("%s dts=%llu size=%d\n", n->name, pkt.dts, pkt.size);
		av_write_frame(n->ofc, &pkt);
	}
}

static void init() {
	int i;
	printf("opening %s\n", input_filename);
	i = avformat_open_input(&ifc, input_filename, NULL, NULL);
	printf("open %d, nb_streams: %d\n", i, ifc->nb_streams);
	avformat_find_stream_info(ifc, NULL);
	for (i = 0; i < ifc->nb_streams; i++) {
		AVStream *st = ifc->streams[i];
		AVCodec *c = avcodec_find_decoder(st->codec->codec_id);
		printf("st=%p cid=%d c=%p\n", st, st->codec->codec_id, c);
		printf("timebase={%d,%d}\n", 
				st->codec->time_base.num, st->codec->time_base.den);
		printf("%s\n", c->name);
		if (!strcmp(c->name, "h264")) 
			st_h264 = st;
	}
	AVCodec *c = avcodec_find_decoder(st_h264->codec->codec_id);
	i = avcodec_open2(st_h264->codec, c, NULL);
	printf("codec_open %d\n", i);
	st_h264->codec->debug |= FF_DEBUG_PICT_INFO;
}


void poll_frame_and_output_jpg(AVFrame *frm, AVStream *st, char *path) {
	//H264Context *h = st->codec->priv_data;
	int n = 0;
	for (n = 0; n < 1060; n++) {
		AVPacket pkt;
		int i = av_read_frame(ifc, &pkt);
//		printf("read %d, pkt: size=%d index=%d\n", 
//				i, pkt.size, pkt.stream_index);
		if (pkt.stream_index != st->index)
			continue;
		int got_pic; 
		i = avcodec_decode_video2(st->codec, frm, &got_pic, &pkt);
		printf("decode %d, w=%d h=%d\n", i, frm->width, frm->height);
		if (got_pic && frm->key_frame)
			break;
	}

	// YUV420P
	printf("format=%d\n", frm->format);
	printf("key_frame=%d\n", frm->key_frame);
	printf("linesize=%d,%d,%d\n", frm->linesize[0], frm->linesize[1], frm->linesize[2]);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	printf("write to %s\n", path);
	FILE *outfile = fopen(path, "wb+");
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	cinfo.image_width = frm->width; 
	cinfo.image_height = frm->height;
	cinfo.input_components = 3;        
	cinfo.in_color_space = JCS_YCbCr;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 90, TRUE);
	cinfo.raw_data_in = TRUE;
	cinfo.comp_info[0].h_samp_factor = 2; 
	cinfo.comp_info[0].v_samp_factor = 2; 
	cinfo.comp_info[1].h_samp_factor = 1; 
	cinfo.comp_info[1].v_samp_factor = 1; 
	cinfo.comp_info[2].h_samp_factor = 1; 
	cinfo.comp_info[2].v_samp_factor = 1; 
	printf("dct_size=%d\n", DCTSIZE);
	jpeg_start_compress(&cinfo, TRUE);
	int i, j;
	JSAMPROW y[16], cb[16], cr[16];
	JSAMPARRAY data[3];
	data[0] = y;
	data[2] = cb;
	data[1] = cr;
	for (j = 0; j < cinfo.image_height; j += 16) {
		for (i = 0; i < 16; i++) {
			y[i] = frm->data[0] + frm->linesize[0]*(i+j);
			cr[i/2] = frm->data[1] + frm->linesize[1]*((i+j)/2);
			cb[i/2] = frm->data[2] + frm->linesize[2]*((i+j)/2);
		}
		jpeg_write_raw_data(&cinfo, data, 16);
	}
	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);
}

void pick_i_frames_rtmp()
{
	AVFrame *frm = avcodec_alloc_frame();

	int i;
	for (i = 0; i < 8; i++) {
		poll_frame_and_output_jpg(frm, st_h264, "r.jpg");
		printf("GOT IT #%d\n", i);
		sleep(10);
	}
}

void pick_i_frames()
{
	AVStream *st = st_h264;
	int i, r;

	AVFrame *frm = avcodec_alloc_frame();

	printf("nb_index_entries: %d\n", st->nb_index_entries); 
	for (i = 0; i < st->nb_index_entries; i++) {
		AVIndexEntry *ie = &st->index_entries[i];
//		printf("#%d pos=%lld ts=%lld\n", i, ie->pos, ie->timestamp);
	}

	if (opt_preview) {
		i = (int)(st->nb_index_entries * preview_pos);
		printf("preview_pos %d/%d\n", i, st->nb_index_entries);
		r = av_seek_frame(ifc, st->index, st->index_entries[i].timestamp, 0);
		printf("seek %d, index=%d\n", r, st->index);
		poll_frame_and_output_jpg(frm, st, output_filename);
		return ;
	}

	int nr = st->nb_index_entries;
	int step = nr / 16;
	int ti, seq = 0;

	for (ti = 0; ti < nr; ti += step) {
		int64_t ts = st->index_entries[ti].timestamp;

		seq++;

		printf("ts=%lld\n", ts);
		i = av_seek_frame(ifc, st->index, ts, 0);
		printf("seek %d, index=%d\n", i, st->index);

		//	i = avio_seek(ifc->pb, st->index_entries[1033].pos, SEEK_SET);
		//	printf("io_seek %d\n", i);
		char path[128];
		sprintf(path, "%d.jpg", seq);

		poll_frame_and_output_jpg(frm, st, path);
	}
}

static void help() {

	exit(0);
}

static void *decode_frame(AVFrame *frm, AVStream *st, int reach_keyfrm) {
	int n;
	for (n = 0; n < 2000; n++) {
		AVPacket pkt;
		int i = av_read_frame(ifc, &pkt);
		if (pkt.stream_index != st->index)
			continue;
		int got_pic; 
		i = avcodec_decode_video2(st->codec, frm, &got_pic, &pkt);
		printf("decode %d, w=%d h=%d pts=%lld\n", i, frm->width, frm->height, pkt.pts);
		if (got_pic) {
			if (!reach_keyfrm || frm->key_frame)
				return frm;
		}
	}
	return NULL;
}

static void encode() 
{
	AVStream *st = st_h264;
	AVFrame *frm = avcodec_alloc_frame();
	int i, r, n;

	decode_frame(frm, st, 1);
	printf("gotit, startencode\n");

	x264_param_t param;

	av_seek_frame(ifc, st->index, 1000*200, 0);

	x264_param_default_preset(&param, "ultrafast", "");
	param.i_log_level = X264_LOG_DEBUG;
	param.i_width = frm->width;
	param.i_height = frm->height;
	param.i_csp = X264_CSP_I420;
	x264_t *h = x264_encoder_open(&param);
	printf("264 %p\n", h);

	FILE *fp = fopen("1.264", "wb+");
	for (n = 0; n < 400; n++) {
		decode_frame(frm, st, 0);
		printf("pts=%lld", frm->pkt_pts);

		if (n == 100) {
			printf("info: %dx%d\n", frm->width, frm->height);
			printf("info: %d,%d,%d\n", 
					frm->linesize[0], 
					frm->linesize[1], 
					frm->linesize[2]
					);
			FILE *fp;
			fp = fopen("/tmp/Image.Y", "wb+"); 
			fwrite(frm->data[0], frm->linesize[0]*frm->height, 1, fp);
			fclose(fp);
			fp = fopen("/tmp/Image.U", "wb+"); 
			fwrite(frm->data[1], frm->linesize[1]*frm->height, 1, fp);
			fclose(fp);
			fp = fopen("/tmp/Image.V", "wb+"); 
			fwrite(frm->data[2], frm->linesize[2]*frm->height, 1, fp);
			fclose(fp);
			exit(0);
		}

		x264_picture_t picin, picout;
		memset(&picin, 0, sizeof(picin));
		picin.img.i_stride[0] = frm->linesize[0];
		picin.img.i_stride[1] = frm->linesize[1];
		picin.img.i_stride[2] = frm->linesize[2];
		printf("imginfo %d %d %d %dx%d\n", 
				frm->linesize[0],
				frm->linesize[1],
				frm->linesize[2],
				param.i_width, param.i_height
				);
		picin.img.plane[0] = frm->data[0];
		picin.img.plane[1] = frm->data[1];
		picin.img.plane[2] = frm->data[2];
		picin.img.i_csp = X264_CSP_I420;
		picin.i_pts = frm->pkt_pts;

		x264_nal_t *nal;
		int i_nal;
		r = x264_encoder_encode(h, &nal, &i_nal, &picin, &picout);
		printf("##%d encode=%d\n", n, r);
		if (r) {
			printf("write frame\n");
			AVPacket pkt;
			pkt.stream_index = 0;
			pkt.pts = frm->pkt_pts / 20;
			pkt.dts = pkt.pts;
			pkt.data = nal[0].p_payload;
			pkt.size = r;
			av_write_frame(ofc, &pkt);
			avio_flush(ofc->pb);
			fwrite(nal[0].p_payload, r, 1, fp);
		}
	}
	fclose(fp);
}

int main(int argc, char *argv[])
{
//	av_log_set_level(AV_LOG_DEBUG);
	av_register_all();
	int c;
	int i;
	char *filename = NULL;

	output_filename = "a.flv";
	output_init();
	input_filename = "/root/1.mp4";
	init();
	encode();
	return 0;

	if (!strcmp(argv[1], "-p")) {
		opt_preview = 1;
		input_filename = argv[2];
		output_filename = argv[3];
		printf("preview %s %s\n", input_filename, output_filename);
		init();
		encode();
		//pick_i_frames();
	} else 
		help();

	//init("/root/2.mp4");
//	pick_i_frames();
	return 0;

	n1.name = "src1";
	n2.name = "src2";
	n3.name = "sink1";
	n4.name = "sink2";

	node_init(&n1, "/root/1.mp4");
	node_init(&n2, "/root/2.mp4");
	n1.ofc->pb->write_packet = write_packet1;
	n2.ofc->pb->write_packet = write_packet2;

	n3.fp = fopen("3.ts", "wb+");
	n4.fp = fopen("4.ts", "wb+");

	printf("start read frames\n");

	int n = 0;
	while (n < 140) {
		n++;
		node_read_packet(&n1);
		node_read_packet(&n2);
	}

	return 0;
}

