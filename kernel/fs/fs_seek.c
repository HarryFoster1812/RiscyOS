#include <types.h>
#include <process.h>
#include <fat_directory.h>
#include <fat_entry.h>
#include <io/io_sheduler.h>
#include <io/sd_functions.h>
#include <mm.h>
#include <util.h>
#include <io/fs/fs_seek.h>

void fs_seek_step(void* raw_ctx, int status);

static seek_position_t seek_decompose(uint32_t offset) {
    uint32_t bytes_per_sector  = 512;
    uint32_t sectors_per_clus  = fs_running_info->SECTORS_PER_CLUSTER;
    uint32_t bytes_per_cluster = bytes_per_sector * sectors_per_clus;

    seek_position_t pos;
    pos.cluster_index     = offset / bytes_per_cluster;
    uint32_t off_in_clus  = offset % bytes_per_cluster;
    pos.sector_in_cluster = off_in_clus / bytes_per_sector;
    pos.byte_in_sector    = off_in_clus % bytes_per_sector;
    return pos;
}


static void fs_seek_post_io(fs_seek_ctx_t* ctx, uint32_t lba) {
    io_sched_req_t* req = kmalloc(sizeof(*req));
    req->lba      = lba;
    req->callback = fs_seek_step;
    req->ctx      = ctx;
    io_sched_submit(req);
}

static void fs_seek_submit(FILE* file,
                    uint32_t           target_offset,
                    op_complete_cb     on_complete,
                    void*              caller_ctx) {

    if (target_offset > file->file_size) {
        on_complete(caller_ctx, -1);  // out of bounds
        return;
    }

    seek_position_t target = seek_decompose(target_offset);

    uint32_t start_cluster;
    uint32_t start_cluster_index;

    seek_position_t current = seek_decompose(file->file_offset);

    if (target.cluster_index >= current.cluster_index) {
        // Forward seek: start from where we already are
        start_cluster       = file->current_cluster;
        start_cluster_index = current.cluster_index;
    } else {
        // Backward seek
        start_cluster       = file->first_cluster;
        start_cluster_index = 0;
    }

    uint32_t clusters_to_skip = target.cluster_index - start_cluster_index;

    if (clusters_to_skip == 0) {
        // Already on the right cluster just update the sub-cluster fields
        file->current_sector = target.sector_in_cluster;
        file->current_byte    = target.byte_in_sector;
        file->file_offset       = target_offset;
        // No IO needed at all
        on_complete(caller_ctx, 0);
        return;
    }

    // Need to walk the FAT chain
    fs_seek_ctx_t* ctx   = kmalloc(sizeof(*ctx));
    ctx->state           = FS_SEEK_WALK_FAT;
    ctx->file            = file;
    ctx->current_cluster = start_cluster;
    ctx->clusters_remaining = clusters_to_skip;
    ctx->target          = target;
    ctx->target_offset   = target_offset;
    ctx->on_complete     = on_complete;
    ctx->caller_ctx      = caller_ctx;

    // Issue first FAT read
    uint32_t fat_lba = fat_calcualte_lba(start_cluster);
    fs_seek_post_io(ctx, fat_lba);
}

void fs_seek_whence(FILE* file, int32_t offset, int whence, op_complete_cb on_complete, void* ctx) {
    uint32_t target;
    switch (whence) {
        case SEEK_SET: target = (uint32_t)offset; break;
        case SEEK_CUR: target = file->file_offset + offset; break;
        case SEEK_END: target = file->file_size   + offset; break;  // offset usually negative
        default: on_complete(ctx, -1); return;
    }
    fs_seek_submit(file, target, on_complete, ctx);
}

static void fs_seek_finish(fs_seek_ctx_t* ctx) {
    int      ok  = (ctx->state == FS_SEEK_DONE) ? 0 : -1;
    op_complete_cb cb  = ctx->on_complete;
    void*    caller    = ctx->caller_ctx;
    kfree(ctx);
    cb(caller, ok);
}


void fs_seek_step(void* raw_ctx, int status) {
    fs_seek_ctx_t* ctx = (fs_seek_ctx_t*)raw_ctx;

    if (status != 0) { ctx->state = FS_SEEK_ERROR; fs_seek_finish(ctx); return; }

    uint32_t next = get_next_cluster(ctx->current_cluster);  // reads from buffer

    if (next >= 0x0FFFFFF8 || next == 0) {
        ctx->state = FS_SEEK_ERROR;  // hit EOC before reaching target
        fs_seek_finish(ctx);
        return;
    }

    ctx->current_cluster = next;
    ctx->clusters_remaining--;

    if (ctx->clusters_remaining == 0) {
        FILE* f  = ctx->file;
				f->current_cluster = ctx->current_cluster;
				f->current_sector = ctx->target.sector_in_cluster;
				f->current_byte = ctx->target.byte_in_sector;
				f->file_offset = ctx->target_offset;
        ctx->state = FS_SEEK_DONE;
        fs_seek_finish(ctx);
        return;
    }

    uint32_t fat_lba = fat_calcualte_lba(ctx->current_cluster);
    fs_seek_post_io(ctx, fat_lba);
}

