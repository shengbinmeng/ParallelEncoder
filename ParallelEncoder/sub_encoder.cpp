#include <stdio.h>
#include "sub_encoder.h"
#include "mem_share.h"
#include "paral_encoder.h"

int sub_encoder_start(parallel_encoder_t *p, sub_encoder_t* sub, int index)
{
	char pic_name[256], nal_name[256];

	sprintf(pic_name, "MEM_SHARE_PIC_%d", index);
	sprintf(nal_name, "MEM_SHARE_NAL_%d", index);

	int unit_size = 0, unit_count = 0;
	unit_size = PIC_UNIT_SIZE;
	unit_count = p->pics_per_idr * UNITS_PER_PIC;
	share_mem_init(&sub->pic_buffer, pic_name, unit_size, unit_count, unit_count, 1);

	unit_size = NAL_UNIT_SIZE;
	unit_count = p->pics_per_idr;
	share_mem_init(&sub->nal_buffer, nal_name, unit_size, unit_count, unit_count, 1);

	//open encoder
	char args[1024];
	sprintf(args, "Encoder.exe %s %s %d %d %d %d %d", pic_name, nal_name, p->pic_width, p->pic_height, 32, index, p->paral_number);
	wchar_t args_w[1024];
	MultiByteToWideChar( CP_ACP, 0, args, -1, args_w, 400 );


	ZeroMemory( &sub->process_info, sizeof(sub->process_info) );
	STARTUPINFO si;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);

	// Start the child process
	BOOL ret = 0;
	ret = CreateProcess(NULL, args_w, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &sub->process_info);
	if(ret == NULL) {
		printf("CreateProcess failed! error: %d", GetLastError());
		return -1;
	}

	return 0;
}

int sub_encoder_cleanup(sub_encoder_t* sub)
{
	share_mem_uninit(&sub->pic_buffer);
	share_mem_uninit(&sub->nal_buffer);

	TerminateProcess(sub->process_info.hProcess , 0);

	return 0;
}


int sub_encoder_input_pic(parallel_encoder_t *p, sub_encoder_t* sub, encoder_picture_t *pic)
{
	if (pic == NULL) {
		share_mem_write(&sub->pic_buffer, NULL, 0, 1);
		return 0;
	}

	int y_size = pic->img.i_height[0] * pic->img.i_width[0];
	share_mem_write(&sub->pic_buffer, pic->img.plane[0], y_size, 0);
	share_mem_write(&sub->pic_buffer, pic->img.plane[1], y_size * 1/4, 0);
	share_mem_write(&sub->pic_buffer, pic->img.plane[2], y_size * 1/4, 0);

	return 0;
}

static int max_size = 2<<16 ;
static uint8_t *static_buffer = (uint8_t*) malloc(max_size);  // large enough buffer
int sub_encoder_output_nal(parallel_encoder_t *p, sub_encoder_t* sub, encoder_nal_t *p_nal)
{
	int eos = 0;
	int read_size = share_mem_read_more (&sub->nal_buffer, static_buffer, max_size, &eos, 0);
	if (eos == 1) {
		// end of stream
		return -1;
	}

	p_nal->p_payload = static_buffer;

	return read_size;
}