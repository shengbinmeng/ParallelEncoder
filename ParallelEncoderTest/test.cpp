//****************************************************************
//包含一个简单的测试外壳
//****************************************************************

#include "./include/stdint.h"
#include "./include/paral_encoder.h"
#include <stdio.h>
#include <stdlib.h>

int read_frame( encoder_picture_t *pic, FILE *f )
{
	size_t luma_size = pic->img.i_width[0] * pic->img.i_height[0];
	size_t chroma_size = luma_size >> 2;
	size_t size;
	fread( pic->img.plane[0], sizeof(char), luma_size  , f );
	fread( pic->img.plane[1], sizeof(char), chroma_size, f );
	size = fread( pic->img.plane[2], sizeof(char), chroma_size, f );
	return size == chroma_size;
}

void skip_frame( encoder_picture_t *pic, FILE *f, int n )
{
	int64_t size = pic->img.i_width[0] * pic->img.i_height[0] * 3 / 2;
	_fseeki64( f, size * n, SEEK_SET );
}


int Encode( parallel_param_t *param )
{
	parallel_encoder_t e, *h = &e;
	encoder_picture_t pic;
	FILE *fin, *fout;
	int i_nal_size;
	encoder_nal_t nal;
	int i = 0, i_frame_count = 240, i_frame_output = 0, i_frame_skip = 0;
	int i_width = param->pic_width;
	int i_height = param->pic_height;

	fin = fopen(param->input_file, "rb" );
	fout = fopen(param->output_file, "wb" );

	if( !fin || !fout )
		return -1;

	if (parallel_encoder_open(h, param) != 0) 
		return -1;

	if (encoder_picture_alloc( &pic, i_width, i_height) != 0)
		return -1;

	skip_frame( &pic, fin, i_frame_skip );

	while( read_frame( &pic, fin ) )
	{
		i_nal_size = parallel_encoder_encode( h, &nal, &pic);
		if( i_nal_size < 0 )
			break;
		else if( i_nal_size ) {
			fwrite( nal.p_payload, sizeof(uint8_t), i_nal_size, fout );
			printf("output nal of frame : %d \n", i_frame_output);
			i_frame_output ++;
		}

		if( ++ i == i_frame_count )
			break;
	}

	while( parallel_encoder_encoding( h ) )
	{
		i_nal_size = parallel_encoder_encode( h, &nal, NULL);
		if( i_nal_size < 0 )
			break;
		else if( i_nal_size ) {
			fwrite( nal.p_payload, sizeof(uint8_t), i_nal_size, fout );
			printf("output nal of frame : %d \n", i_frame_output);
			i_frame_output ++;
		}
	}

	fclose( fin );
	fclose( fout );
	encoder_picture_free( &pic );
	parallel_encoder_close( h );

	return 0;
}

int main( int argc, char **argv )
{
	parallel_param_t param;

	if(argc != 3 ) {
		printf("usage: *.exe input.yuv output.bin \n");
		return -1;
	}

	param.paral_number = 3;
	param.pic_width = 640;
	param.pic_height = 480;
	param.pics_per_idr = 100;
	param.input_file = argv[1];
	param.output_file = argv[2];

	return Encode( &param );
}