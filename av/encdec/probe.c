#include "av.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s %s\n", argv[0], argv[1]);
		return 0;
	}

	av_register_all();

	char *filename = argv[1];
	AVFormatContext *ifc = NULL;
	int i;

	printf("open %s\n", filename);
	i = avformat_open_input(&ifc, filename, NULL, NULL);
	if (i) {
		printf("open %s failed: %d\n", filename, i);
		return 1;
	}

	avformat_find_stream_info(ifc, NULL);
	av_dump_format(ifc, 0, filename, 0);

//	printf("time_base %lld\n", pkt.stream_index, pkt.pts);

	while (1) {
		AVPacket pkt;
		i = av_read_frame(ifc, &pkt);
		if (i) 
			break;
		printf("pkt: idx %d pts %lld\n", pkt.stream_index, pkt.pts);
	}

	return 0;
}

