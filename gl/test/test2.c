
typedef struct {
	void *m;
	void *data[3];
	int line[3];
	float pos[4];
	float start[4];
	void *v;
} vidplay_t ;

static vidplay_t vid[4];

static void vidplay_init() 
{
	int i;

	for (i = 0; i < 4; i++) {
		char path[256];
		sprintf(path, "/vid/%d.mp4", i+1);
		vid[i].m = mp4dec_open(path);
		if (!vid[i].m)
			return ;
		vid[i].v = video_new(
				mp4dec_width(vid[i].m), 
				mp4dec_height(vid[i].m)
				);
		if (!vid[i].v)
			return ;
		mp4dec_seek_precise(vid[i].m, 10);
		int r = mp4dec_read_frame(vid[i].m, vid[i].data, vid[i].line, NULL, NULL);
		if (r)
			return ;
		printf("create vid %d %d,%p\n", i, vid[i].line[0], vid[i].data[0]);
	}
}
