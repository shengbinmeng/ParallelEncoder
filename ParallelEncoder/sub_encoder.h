#pragma once 
#include "mem_share.h"

struct parallel_encoder_t;
struct encoder_picture_t;
struct encoder_nal_t;

struct sub_encoder_t {
	share_mem_info_t pic_buffer;
	share_mem_info_t nal_buffer;
	PROCESS_INFORMATION process_info;
};

int sub_encoder_start(parallel_encoder_t *p, sub_encoder_t* sub, int index);
int sub_encoder_cleanup(sub_encoder_t* sub);
int sub_encoder_input_pic(parallel_encoder_t *p, sub_encoder_t* sub, encoder_picture_t *pic);
int sub_encoder_output_nal(parallel_encoder_t *p, sub_encoder_t* sub, encoder_nal_t *p_nal);