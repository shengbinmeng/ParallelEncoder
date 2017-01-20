#include <stdio.h>
#include <stdlib.h>
#include "paral_encoder.h"

static int read_frame(encoder_picture_t *pic, FILE *f)
{
	size_t luma_size = pic->img.width[0] * pic->img.height[0];
	size_t chroma_size = luma_size >> 2;
	size_t size;
	fread(pic->img.plane[0], sizeof(char), luma_size, f);
	fread(pic->img.plane[1], sizeof(char), chroma_size, f);
	size = fread(pic->img.plane[2], sizeof(char), chroma_size, f);
	return size == chroma_size;
}

static void skip_frame(encoder_picture_t *pic, FILE *f, int n)
{
	long long size = pic->img.width[0] * pic->img.height[0] * 3 / 2;
	_fseeki64(f, size * n, SEEK_SET);
}

static int encode(char *input_file, char *output_file, encoder_param_t *param, int frame_skip, int frame_number)
{
	FILE *fin = fopen(input_file, "rb");
	if (fin == NULL) {
		fprintf(stderr, "Error when open input file: %s\n", input_file);
		return -1;
	}

	FILE *fout = fopen(output_file, "wb");
	if (fout == NULL) {
		fprintf(stderr, "Error when open output file: %s\n", input_file);
		return -1;
	}

	parallel_encoder_t *h = parallel_encoder_open(param);
	if (h == NULL) {
		fprintf(stderr, "Error when open parallel encoder\n");
		return -1;
	}

	encoder_picture_t pic;
	if (encoder_picture_alloc(&pic, param->pic_width, param->pic_height) != 0) {
		fprintf(stderr, "Error when allocate encoder picture\n");
		return -1;
	}

	skip_frame(&pic, fin, frame_skip);

	encoder_nal_t nal;
	int nal_size = 0, frame_output = 0, frame_input = 0;
	while (read_frame(&pic, fin)) {
		nal_size = parallel_encoder_encode(h, &nal, &pic);
		if (nal_size < 0) {
			break;
		} else if (nal_size > 0) {
			fwrite(nal.payload, sizeof(unsigned char), nal_size, fout);
			printf("Output nal of frame: %d\n", frame_output);
			frame_output++;
		}

		frame_input++;
		if (frame_input == frame_number) {
			break;
		}
	}

	while (parallel_encoder_encoding(h)) {
		nal_size = parallel_encoder_encode(h, &nal, NULL);
		if (nal_size < 0) {
			break;
		} else if (nal_size > 0) {
			fwrite(nal.payload, sizeof(unsigned char), nal_size, fout);
			printf("Output nal of frame: %d\n", frame_output);
			frame_output++;
		}
	}

	fclose(fin);
	fclose(fout);
	encoder_picture_free(&pic);
	parallel_encoder_close(h);

	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 5) {
		printf("Usage: %s <width> <height> <input_file> <output_file> [frame_skip] [frame_number]\n", argv[0]);
		return -1;
	}

	encoder_param_t param;
	param.pic_width = atoi(argv[1]);
	param.pic_height = atoi(argv[2]);
	param.paral_number = 3;
	param.pics_per_idr = 100;
	int frame_skip = 0, frame_number = -1;
	if (argc > 5) {
		frame_skip = atoi(argv[5]);
	}
	if (argc > 6) {
		frame_number = atoi(argv[6]);
	}

	return encode(argv[3], argv[4], &param, frame_skip, frame_number);
}