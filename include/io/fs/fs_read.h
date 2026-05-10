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

void fs_open_submit_with_dir_parent(const char *path, FILE *out_file, uint8_t proc_id, op_complete_cb callback, void *caller_context, uint32_t* parent_cluster_storage);
