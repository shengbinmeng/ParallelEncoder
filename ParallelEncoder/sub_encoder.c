#include <stdio.h>
#include "sub_encoder.h"
#include "share_mem.h"

int sub_encoder_start(sub_encoder_t* sub)
{
	char pic_name[256], nal_name[256];

	sprintf(pic_name, "MEM_SHARE_PIC_%d", sub->index);
	sprintf(nal_name, "MEM_SHARE_NAL_%d", sub->index);

	int unit_size = PIC_UNIT_SIZE;
	int unit_count = sub->pic_count * UNITS_PER_PIC;
	share_mem_init(&sub->pic_buffer, pic_name, unit_size, unit_count, unit_count, 1);

	unit_size = NAL_UNIT_SIZE;
	unit_count = sub->pic_count;
	share_mem_init(&sub->nal_buffer, nal_name, unit_size, unit_count, unit_count, 1);

	char args[1024];
	sprintf(args, "Encoder.exe %s %s %d %d %d %d %d", pic_name, nal_name, sub->pic_width, sub->pic_height, 32, sub->index, sub->number);
	wchar_t args_w[1024];
	MultiByteToWideChar(CP_ACP, 0, args, -1, args_w, 400);

	ZeroMemory(&sub->process_info, sizeof(sub->process_info));
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	// Start the child process.
	BOOL ret = CreateProcess(NULL, args_w, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &sub->process_info);
	if (ret == FALSE) {
		printf("CreateProcess failed! error: %d", GetLastError());
		return -1;
	}

	return 0;
}

int sub_encoder_stop(sub_encoder_t* sub)
{
	TerminateProcess(sub->process_info.hProcess , 0);
	share_mem_uninit(&sub->pic_buffer);
	share_mem_uninit(&sub->nal_buffer);
	return 0;
}

int sub_encoder_input_pic(sub_encoder_t *sub, encoder_picture_t *pic)
{
	if (pic == NULL) {
		share_mem_write(&sub->pic_buffer, NULL, 0, 1, 1);
		return 0;
	}

	int y_size = pic->img.height[0] * pic->img.width[0];
	share_mem_write(&sub->pic_buffer, pic->img.plane[0], y_size, 0, 1);
	share_mem_write(&sub->pic_buffer, pic->img.plane[1], y_size * 1/4, 0, 1);
	share_mem_write(&sub->pic_buffer, pic->img.plane[2], y_size * 1/4, 0, 1);

	return 0;
}

#define MAX_SIZE 1024*1024
static uint8_t static_buffer[MAX_SIZE];  // large enough buffer

int sub_encoder_output_nal(sub_encoder_t *sub, encoder_nal_t *nal)
{
	int eos = 0;
	int read_size = share_mem_read(&sub->nal_buffer, static_buffer, MAX_SIZE, &eos, 0);
	if (eos == 1) {
		// End of stream.
		return -1;
	}

	nal->payload = static_buffer;

	return read_size;
}
