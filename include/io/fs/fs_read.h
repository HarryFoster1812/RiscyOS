#pragma once
#include <file.h>
#include <types.h>

typedef enum{
	FSREAD_LOAD_CONTENTS,
	FSREAD_COPY_CONTENTS,
	FSREAD_DONE,
	FSREAD_ERROR,
}fs_read_state_t;

typedef struct{
	FILE* file;
	uint8_t* buffer;
	int state;
	int bytes_remaining; 
	op_complete_cb callback; 
	void* ctx;
}fs_read_ctx_t;


void fs_read_submit(FILE* file, void* buffer, int bytes, op_complete_cb callback, void* ctx);
