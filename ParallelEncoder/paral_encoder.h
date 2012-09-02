#pragma once

struct sub_encoder_t;

struct encoder_image_t
{
	int i_height[3];
	int i_width[3];
	int i_stride[3]; //Êä³öÍ¼ÏñµÄÁÐ¿í
	unsigned char  *plane[3];
};

struct  encoder_picture_t
{
	int i_type;
	encoder_image_t img;
};

struct encoder_nal_t
{
	int i_ref_idc;	/* nal_priority_e */
	int i_type;		/* nal_unit_type_e */
	int b_long_startcode;

	int temporal_id;
	int output_flag;
	int reserved_one_4bits;

	int i_payload;
	unsigned char *p_payload;
};

struct parallel_encoder_t
{
	int pic_width;
	int pic_height;
	int paral_number;
	int pics_per_idr;
	int dispatch_pic_count;
	int collect_pic_count;
	int end_of_input_pics;
	sub_encoder_t ** sub_encoders;
};

struct parallel_param_t
{
	int pic_width;
	int pic_height;
	int paral_number;
	int pics_per_idr;
};

int encoder_picture_alloc( encoder_picture_t *pic, int i_width, int i_height );
void encoder_picture_free( encoder_picture_t *pic );

int parallel_encoder_open(parallel_encoder_t *encoder, parallel_param_t *param);

//-1 for end of stream, 0 for success
int parallel_encoder_encode(parallel_encoder_t *encoder , encoder_nal_t *p_nal, encoder_picture_t *pic_in);

//>0 for encoding, 0 for stopped
int parallel_encoder_encoding(parallel_encoder_t *encoder );

void parallel_encoder_close(parallel_encoder_t *encoder);