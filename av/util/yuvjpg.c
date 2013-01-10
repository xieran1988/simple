#include <stdio.h>
#include <stdint.h>
#include <jpeglib.h>
#include <stdlib.h>

int yuv2jpg(char *path, int w, int h, void **_data, int *line)
{
	FILE *outfile = fopen(path, "wb+");
	uint8_t **data = (uint8_t **)_data;

	if (!outfile)
		return 1;
	h &= ~15;

	printf("yuv2jpg: %d,%d\n", w, h);
	printf("yuv2jpg: %p,%p,%p\n", data[0], data[1], data[2]);
	printf("yuv2jpg: %d,%d,%d\n", line[0], line[1], line[2]);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = w; 
	cinfo.image_height = h;
	cinfo.input_components = 3;        
	cinfo.in_color_space = JCS_YCbCr;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 90, TRUE);
	cinfo.dct_method = JDCT_FASTEST; 

	cinfo.raw_data_in = TRUE;
	cinfo.comp_info[0].h_samp_factor = 2; 
	cinfo.comp_info[0].v_samp_factor = 2; 
	cinfo.comp_info[1].h_samp_factor = 1; 
	cinfo.comp_info[1].v_samp_factor = 1; 
	cinfo.comp_info[2].h_samp_factor = 1; 
	cinfo.comp_info[2].v_samp_factor = 1; 

	int i, j;
	JSAMPROW y[16], cb[16], cr[16];
	JSAMPARRAY p[3];

	jpeg_start_compress(&cinfo, TRUE);
	p[0] = y;
	p[2] = cb;
	p[1] = cr;

	printf("start comp: %d\n", cinfo.image_height);
	for (j = 0; j < cinfo.image_height; j += 16) {
		printf("at: %d\n", j);
		for (i = 0; i < 16; i++) {
			y[i] = data[0] + line[0]*(i+j);
			cr[i/2] = data[1] + line[1]*((i+j)/2);
			cb[i/2] = data[2] + line[2]*((i+j)/2);
		}
		printf("at2: %d\n", j);
		jpeg_write_raw_data(&cinfo, p, 16);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);
}

