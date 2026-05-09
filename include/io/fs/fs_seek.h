#pragma once
#include <types.h>
#include <file.h>
#include "fs.h"

typedef enum {
    FS_SEEK_WALK_FAT,  
    FS_SEEK_DONE,
    FS_SEEK_ERROR,
} fs_seek_state_t;

typedef struct {
    uint32_t cluster_index;
    uint8_t  sector_in_cluster;
    uint16_t byte_in_sector;
} seek_position_t;

typedef struct {
    fs_seek_state_t    state;
    FILE* file;
    uint32_t           current_cluster;     // cluster we are currently walking from
    uint32_t           clusters_remaining;  // how many more hops needed
    seek_position_t    target;              // pre-computed final position
    uint32_t           target_offset;       // saved for updating file->file_offset
    op_complete_cb     on_complete;
    void*              caller_ctx;
} fs_seek_ctx_t;


typedef enum{
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
} seek_whence;

void fs_seek_whence(FILE* file, int32_t offset, int whence, op_complete_cb on_complete, void* ctx);
