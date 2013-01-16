#include "av.h"

int main()
{
	AVFilterGraph *graph = avfilter_graph_alloc();
	AVFilterInOut *inputs, *outputs;
  AVFilterContext *fc_in, *fc_out, *format;
  char *name, *args;
	int i;

	avfilter_register_all();
	av_register_all();

	i = avfilter_graph_parse2(graph, "anull", &inputs, &outputs);
	printf("parse2 %d\n", i);

  name = "graph 0 input from stream 0:0";
  args = "time_base=1/22050:sample_rate=22050:sample_fmt=s16p:channel_layout=0x3";
	i = avfilter_graph_create_filter(&fc_in,
		avfilter_get_by_name("abuffer"),
		name, args, NULL, graph);
	printf("creat in %d\n", i);
	i = avfilter_link(fc_in, 0, inputs->filter_ctx, 0);
	printf("link1 %d\n", i);

	name = "output stream 0:0";
	i = avfilter_graph_create_filter(&fc_out, 
		avfilter_get_by_name("abuffersink"), 
		name, NULL, NULL, graph
	);
	printf("create sink %d\n", i);

	name = "audio format for output stream 0:0";
	args = "sample_fmts=fltp:sample_rates=44100";
	i = avfilter_graph_create_filter(&format, 
		avfilter_get_by_name("aformat"), 
		name, args, NULL, graph
	);
	printf("create aformat %d\n", i);

	i = avfilter_link(outputs->filter_ctx, 0, format, 0);
	printf("link2 %d\n", i);

	i = avfilter_link(format, 0, fc_out, 0);
	printf("link3 %d\n", i);

	AVFrame *frm_in = avcodec_alloc_frame();
	i = av_buffersrc_write_frame(fc_in, frm_in);
	printf("writefrm %d\n", i);

	AVFrame *frm_out = avcodec_alloc_frame();
	avcodec_get_frame_defaults(frm_out); // repeat

	AVFilterBufferRef *picref;
	av_buffersink_read_samples(fc_out, &picref, 1024);
	avfilter_copy_buf_props(frm_out, picref);
//	frm->pts = av_rescale_q(picref->pts, intpus->time_base, codec->time_base);

	return 0;
}

