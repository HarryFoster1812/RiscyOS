#include "io/io_sheduler.h"
#include "io/sd_functions.h"
#include <types.h>
#include <mm.h>
#include <io/fs/fs_seek.h>
#include <io/fs/fs_read.h>


void fs_close_submit(FILE* file, op_complete_cb callback,void* ctx){
	kfree(file);
	// tell the file management that the descriptor is free
	callback(ctx, 0); // 0 is status ok
	return;
}

