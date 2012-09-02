#include <stdio.h>
#include "paral_encoder.h"
#include "mem_share.h"
#include "sub_encoder.h"

int encoder_picture_alloc( encoder_picture_t *pic, int i_width, int i_height )
{
	int luma_size = i_width * i_height;
	pic->img.i_width[0] = i_width;
	pic->img.i_width[1] = pic->img.i_width[2] = i_width / 2;
	pic->img.i_stride[0] = i_width;
	pic->img.i_stride[1] = pic->img.i_stride[2] = i_width / 2;
	pic->img.i_height[0] = i_height;
	pic->img.i_height[1] = pic->img.i_height[2] = i_height / 2;
	pic->img.plane[0] = (unsigned char*) malloc( luma_size * 3 / 2 );
	if( !pic->img.plane[0] ) return -1;
	pic->img.plane[1] = pic->img.plane[0] + luma_size;
	pic->img.plane[2] = pic->img.plane[1] + luma_size / 4;
	return 0;
}

void encoder_picture_free( encoder_picture_t *pic )
{
	if( pic )
		free( pic->img.plane[0] );
}

int parallel_encoder_open(parallel_encoder_t *p, parallel_param_t *param)
{
	p->pic_height = param->pic_height;
	p->pic_width = param->pic_width;
	p->pics_per_idr = param->pics_per_idr;
	p->paral_number = param->paral_number;
	p->dispatch_pic_count = 0;
	p->collect_pic_count = 0;
	p->end_of_input_pics = 0;

	int num = p->paral_number;
	p->sub_encoders = (sub_encoder_t **) malloc (sizeof(sub_encoder_t*) * num);
	for (int i = 0; i < num; i++) {
		p->sub_encoders[i] = (sub_encoder_t*) malloc(sizeof(sub_encoder_t));
		int ret = sub_encoder_start(p, p->sub_encoders[i], i);
		if (ret != 0) {
			printf("sub encoder start failed! \n");
			return -1;
		}
	}

	return 0;
}

static int dispatch (parallel_encoder_t *p, encoder_picture_t* pic)
{
	if (pic == NULL) {
		//end of stream
		if (p->end_of_input_pics == 0) {
			for (int i = 0; i < p->paral_number; i++) {
				sub_encoder_input_pic(p, p->sub_encoders[i], NULL);
			}
			p->end_of_input_pics = 1;
		}
		return 0;
	}

	int idr_count = p->dispatch_pic_count / p->pics_per_idr;
	int index = idr_count % p->paral_number;
	sub_encoder_input_pic(p, p->sub_encoders[index], pic);

	p->dispatch_pic_count ++;

	return 0;
}

static int collect (parallel_encoder_t *p, encoder_nal_t *p_nal)
{
	int idr_count = p->collect_pic_count / p->pics_per_idr;
	int index = idr_count % p->paral_number;
	int ret = sub_encoder_output_nal(p, p->sub_encoders[index], p_nal);
	if (ret > 0) {
		p->collect_pic_count ++;
	}
	return ret;
}

//-1 for failure, 0 for success
int parallel_encoder_encode(parallel_encoder_t *p , encoder_nal_t *p_nal, encoder_picture_t *pic_in)
{
	dispatch(p, pic_in);
	return collect(p, p_nal);
}

int parallel_encoder_encoding(parallel_encoder_t *encoder )
{
	if (encoder->collect_pic_count < encoder->dispatch_pic_count)
		return 1;
	else return 0;
}

void parallel_encoder_close(parallel_encoder_t *p)
{
	for (int i = 0; i < p->paral_number; i++) {
		sub_encoder_t *sub = p->sub_encoders[i];
		sub_encoder_cleanup(sub);
		delete sub;
    }
	delete p->sub_encoders;
}