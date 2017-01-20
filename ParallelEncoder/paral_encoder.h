#pragma once

typedef struct {
	int height[3];
	int width[3];
	int stride[3];
	unsigned char *plane[3];
} encoder_image_t;

typedef struct {
	int type;
	encoder_image_t img;
} encoder_picture_t;

typedef struct {
	int payload_size;
	unsigned char *payload;
} encoder_nal_t;

typedef struct {
	int pic_width;
	int pic_height;
	int paral_number;
	int pics_per_idr;
} encoder_param_t;

typedef struct parallel_encoder_t parallel_encoder_t;

int encoder_picture_alloc(encoder_picture_t *pic, int width, int height);
void encoder_picture_free(encoder_picture_t *pic);

parallel_encoder_t* parallel_encoder_open(encoder_param_t *param);

int parallel_encoder_encode(parallel_encoder_t *encoder, encoder_nal_t *nal_out, encoder_picture_t *pic_in);

int parallel_encoder_encoding(parallel_encoder_t *encoder);

void parallel_encoder_close(parallel_encoder_t *encoder);
