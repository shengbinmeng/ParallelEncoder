#include <stdio.h>
#include <stdlib.h>
#include "paral_encoder.h"
#include "sub_encoder.h"

struct parallel_encoder_t
{
	int pic_width;
	int pic_height;
	int paral_number;
	int pics_per_idr;
	int dispatch_pic_count;
	int collect_pic_count;
	int end_of_input_pics;
	sub_encoder_t **sub_encoders;
};

int encoder_picture_alloc(encoder_picture_t *pic, int width, int height)
{
	int luma_size = width * height;
	pic->img.width[0] = width;
	pic->img.width[1] = pic->img.width[2] = width / 2;
	pic->img.stride[0] = width;
	pic->img.stride[1] = pic->img.stride[2] = width / 2;
	pic->img.height[0] = height;
	pic->img.height[1] = pic->img.height[2] = height / 2;
	pic->img.plane[0] = (unsigned char*)malloc( luma_size * 3 / 2 );
	if (!pic->img.plane[0]) {
		return -1;
	}
	pic->img.plane[1] = pic->img.plane[0] + luma_size;
	pic->img.plane[2] = pic->img.plane[1] + luma_size / 4;
	return 0;
}

void encoder_picture_free(encoder_picture_t *pic)
{
	if (pic) {
		free(pic->img.plane[0]);
	}
}

parallel_encoder_t* parallel_encoder_open(encoder_param_t *param)
{
	parallel_encoder_t *p = (parallel_encoder_t *)malloc(sizeof(parallel_encoder_t));
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
		sub_encoder_t *sub = (sub_encoder_t*) malloc(sizeof(sub_encoder_t));
		sub->index = i;
		sub->number = num;
		sub->pic_count = p->pics_per_idr;
		sub->pic_width = p->pic_width;
		sub->pic_height = p->pic_height;
		int ret = sub_encoder_start(sub);
		if (ret != 0) {
			printf("sub encoder start failed! \n");
			return NULL;
		}
		p->sub_encoders[i] = sub;
	}
	return p;
}

static int dispatch(parallel_encoder_t *p, encoder_picture_t* pic)
{
	if (pic == NULL) {
		// End of stream.
		if (p->end_of_input_pics == 0) {
			for (int i = 0; i < p->paral_number; i++) {
				sub_encoder_input_pic(p->sub_encoders[i], NULL);
			}
			p->end_of_input_pics = 1;
		}
		return 0;
	}

	int idr_count = p->dispatch_pic_count / p->pics_per_idr;
	int index = idr_count % p->paral_number;
	sub_encoder_input_pic(p->sub_encoders[index], pic);

	p->dispatch_pic_count++;

	return 0;
}

static int collect(parallel_encoder_t *p, encoder_nal_t *nal)
{
	int idr_count = p->collect_pic_count / p->pics_per_idr;
	int index = idr_count % p->paral_number;
	int ret = sub_encoder_output_nal(p->sub_encoders[index], nal);
	if (ret > 0) {
		p->collect_pic_count++;
	}
	return ret;
}

int parallel_encoder_encode(parallel_encoder_t *p , encoder_nal_t *nal_out, encoder_picture_t *pic_in)
{
	dispatch(p, pic_in);
	return collect(p, nal_out);
}

int parallel_encoder_encoding(parallel_encoder_t *encoder)
{
	if (encoder->collect_pic_count < encoder->dispatch_pic_count) {
		return 1;
	} else {
		return 0;
	}
}

void parallel_encoder_close(parallel_encoder_t *p)
{
	for (int i = 0; i < p->paral_number; i++) {
		sub_encoder_t *sub = p->sub_encoders[i];
		sub_encoder_stop(sub);
		free(sub);
    }
	free(p->sub_encoders);
	free(p);
}