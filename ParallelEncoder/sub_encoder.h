#pragma once

#include "share_mem.h"
#include "paral_encoder.h"

typedef struct
{
	int index;
	int number;
	int pic_count;
	int pic_width;
	int pic_height;
	share_mem_info_t pic_buffer;
	share_mem_info_t nal_buffer;
	PROCESS_INFORMATION process_info;
} sub_encoder_t;

int sub_encoder_start(sub_encoder_t* sub);
int sub_encoder_stop(sub_encoder_t* sub);
int sub_encoder_input_pic(sub_encoder_t* sub, encoder_picture_t *pic);
int sub_encoder_output_nal(sub_encoder_t* sub, encoder_nal_t *nal);