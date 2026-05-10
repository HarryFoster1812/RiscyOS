#pragma once
#include <types.h>
#include <file.h>
#include "fs.h"

typedef enum {
	FSOPEN_PARSE_DIR_SECTOR,  // a dir sector is in the DMA buffer; parse it
	FSOPEN_PROCESS_FAT,       // a FAT sector is in the DMA buffer; get next cluster
	FSOPEN_DONE,
	FSOPEN_NOT_FOUND,
	FSOPEN_ERROR,
} fs_open_state_t;


typedef struct {
	const char* full_path;       // e.g. "/dir_a/dir_b/file.txt"
	int         offset;          // byte offset of the component we are currently resolving
	char        component[13];   // current 8.3 component string (null-terminated)
	char        component_83[11]; // FAT 8.3 packed format (space-padded, no dot)
} path_walker_t;

typedef struct {
	fs_open_state_t  state;
	path_walker_t    walker;
	uint32_t         current_cluster;
	uint32_t         current_lba;        // lba of the sector we last requested
	uint8_t          sector_in_cluster;  // 0-based index within current cluster
	FILE* out_file;         // filled in on FSOPEN_DONE
	uint32_t*					parent_cluster_storage;
	uint8_t          proc_id;            // process to wake when done
	op_complete_cb   on_complete;
    void*            caller_ctx;
} fs_open_ctx_t;



void fs_open_submit(const char* path, FILE* out_file, uint8_t proc_id, op_complete_cb callback, void* caller_context);
