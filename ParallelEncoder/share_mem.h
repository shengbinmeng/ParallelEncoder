#pragma once

#include <stdint.h>
#include <windows.h>

#define UNIT_HEADER_LENGTH 8 // 8 byte
#define CONTENT(unit) ((uint8_t *)unit + UNIT_HEADER_LENGTH)

#define PIC_UNIT_SIZE  ((1<<19)-UNIT_HEADER_LENGTH) // a little larger than size of 1920x1080 chroma
#define UNITS_PER_PIC 6 // a pic's yuv data size is 6 times of chroma data size
#define NAL_UNIT_SIZE  ((1<<14)-UNIT_HEADER_LENGTH) // 4K

typedef struct {
	uint32_t content_size;
	uint8_t has_content;
	uint8_t eos; //end of stream
	uint8_t eob; //end of b
	uint8_t reserved; //reserved

	//contents
} share_mem_unit_t;

typedef struct {
	HANDLE map_handle;
	LPVOID buffer_handle;
	share_mem_unit_t **share_unit;
	int32_t current_read;
	int32_t current_write;
	int32_t current_read_map;
	int32_t current_write_map;
	int32_t unit_size;  // size of usefull data, not including unit header
	int32_t unit_count;
	int32_t mapping_count;
} share_mem_info_t;

int share_mem_init(share_mem_info_t *shm, char *file_mapping_name, int unit_size, int unit_count, int mapping_count, int create);
int share_mem_read(share_mem_info_t *shm, void *buffer, int max_size, int *eos, int wait);
int share_mem_write(share_mem_info_t *shm, void *buffer, int size, int eos, int wait);
void share_mem_uninit(share_mem_info_t *shm);
