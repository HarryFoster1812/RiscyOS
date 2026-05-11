#include "io/io_sheduler.h"
#include "io/sd_functions.h"
#include <types.h>
#include <mm.h>
#include <io/fs/fs_seek.h>
#include <io/fs/fs_read.h>


void fs_read_step(void*, int);

void fs_read_post_io(fs_read_ctx_t* context,unsigned int lba, fs_read_state_t next_state){
	context->state=next_state;

	io_sched_req_t* req = kmalloc(sizeof(*req));
	req->lba      = lba;
	req->callback = fs_read_step;   
	req->ctx      = context;
	io_sched_submit(req);
}

void fs_read_submit(FILE* file, void* buffer, int bytes, op_complete_cb callback, void* ctx){
	fs_read_ctx_t* context = kmalloc(sizeof(fs_read_ctx_t));
	context->callback = callback;
	context->ctx = ctx;
	context->file = file;
	context->buffer = buffer;
	context->bytes_remaining = bytes;

	int lba = cluster_to_lba(file->current_cluster);
	lba += file->current_sector;

	fs_read_post_io(context, lba, FSREAD_COPY_CONTENTS);

}

void fs_read_finish(fs_read_ctx_t* ctx){
	int status = ctx->state==FSREAD_DONE ?  0: 1;
	op_complete_cb cb = ctx->callback;
	void* cb_ctx = ctx->ctx;
	kfree(ctx);
	cb(cb_ctx, status);
}

void fs_read_step(void* raw_ctx, int status){
	fs_read_ctx_t* ctx = (fs_read_ctx_t*)raw_ctx;

	if (status != 0) {
		ctx->state = FSREAD_ERROR;
		fs_read_finish(ctx);
		return;
	}

	switch(ctx->state){
		case FSREAD_LOAD_CONTENTS: {
																 int lba = cluster_to_lba(ctx->file->current_cluster);
																 lba += ctx->file->current_sector;
																 fs_read_post_io(ctx, lba, FSREAD_COPY_CONTENTS);
																 break;
															 }

		case FSREAD_COPY_CONTENTS: {

																		uint8_t* sector_contents = (uint8_t*)SD_RX_RAM;
																		int bytes_to_copy_in_sector = ctx->bytes_remaining;
																		if(ctx->file->current_byte+ctx->bytes_remaining >= 512){
																			bytes_to_copy_in_sector=512-ctx->file->current_byte;
																		}

                                    uint8_t* dst = (uint8_t*)ctx->buffer;
					
																		for(int i=0;i<bytes_to_copy_in_sector;i++){
																			dst[i] = sector_contents[ctx->file->current_byte+i];
																		}
                                    ctx->buffer = dst+bytes_to_copy_in_sector;
                                    ctx->bytes_remaining-=bytes_to_copy_in_sector;

                                    if(ctx->bytes_remaining==0){
                                      ctx->file->current_byte += bytes_to_copy_in_sector;
                                      ctx->state = FSREAD_DONE;
                                      fs_read_finish(ctx);
                                      return;
                                    }

                                    ctx->state = FSREAD_LOAD_CONTENTS;

																		fs_seek_whence(ctx->file, bytes_to_copy_in_sector, SEEK_CUR, fs_read_step, ctx);
																		break;
															 }
                         // increment sector
	}
}
