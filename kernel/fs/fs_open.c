#include <types.h>
#include <process.h>
#include <io/sd_io_request.h>
#include <fat_directory.h>
#include <fat_entry.h>
#include <io/io_sheduler.h>
#include <io/sd_functions.h>
#include <mm.h>
#include <util.h>
#include <io/fs/fs_open.h>

// Open-file FSM context.
// One of these is allocated per open() call and freed on completion.

void fs_open_step(void* raw_ctx, int status);

static inline int toupper(int chr)
{
	return ((chr >= 'a' && chr <= 'z') ? (chr - 32) : (chr));
}

static int fat_valid_char(char c){

	if(c < 0x20)
		return 0;

	switch(c){
		case '"':
		case '*':
		case '+':
		case ',':
		case '/':
		case ':':
		case ';':
		case '<':
		case '=':
		case '>':
		case '?':
		case '[':
		case '\\':
		case ']':
		case '|':
			return 0;
	}

	return 1;
}

static const char* get_next_path_component(const char* path, char out[13]) {
	const char* pathcpy = path;
	if(*pathcpy == '/')
		(pathcpy)++;

	if(*pathcpy == 0)
		return 0;

	int i = 0;

	while(*pathcpy && *pathcpy != '/') {
		out[i++] = *pathcpy;
		(pathcpy)++;
	}

	out[i] = 0;

	return pathcpy;
}

static int make_fat_83_name(const char* input, char out[11]){

	memset(out, ' ', 11);

	char name[9];
	char ext[4];

	memset(name, 0, sizeof(name));
	memset(ext, 0, sizeof(ext));

	char* cpy = input;
	char* dot=NULL;
	while((*cpy++)){
		if(*cpy == '.'){
			dot = cpy;
			break;
		}
	}

	int name_len = 0;
	int ext_len = 0;

	// split name/ext
	if(dot){

		while(*input && input != dot){

			if(!fat_valid_char(*input))
				return 0;

			if(name_len < 8)
				name[name_len++] = toupper(*input);

			input++;
		}

		input = dot + 1;

		while(*input){

			if(!fat_valid_char(*input))
				return 0;

			if(ext_len < 3)
				ext[ext_len++] = toupper(*input);

			input++;
		}

	} else {

		while(*input){

			if(!fat_valid_char(*input))
				return 0;

			if(name_len < 8)
				name[name_len++] = toupper(*input);

			input++;
		}
	}

	memcpy(name, out, name_len);
	memcpy(ext, out + 8, ext_len);

	return 1;
}

// Advances walker to the next component.
// Returns 1 if there is a next component, 0 if path is exhausted.
static int path_walker_advance(path_walker_t* w) {
	const char* next = get_next_path_component(
			w->full_path + w->offset,
			w->component);
	if (!next || *next == '\0') {
		w->offset = -1; 
		return 0;
	}
	w->offset = (int)(next - w->full_path);
	make_fat_83_name(w->component, w->component_83);
	return 1;
}

static int path_walker_has_next(const path_walker_t* w) {
	return w->offset >= 0;
}


static void fs_open_post_io(fs_open_ctx_t* ctx, uint32_t lba,
		fs_open_state_t next_state) {
	ctx->state       = next_state;
	ctx->current_lba = lba;

	io_sched_req_t* req = kmalloc(sizeof(*req));
	req->lba      = lba;
	req->callback = fs_open_step;   
	req->ctx      = ctx;
	io_sched_submit(req);
}

void fs_open_submit(const char* path, FILE* out_file, uint8_t proc_id, op_complete_cb callback, void* caller_context) {
	fs_open_ctx_t* ctx  = kmalloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));
	ctx->out_file        = out_file;
	ctx->proc_id         = proc_id;
	ctx->on_complete		 = callback;
	ctx->caller_ctx			 = caller_context;
	ctx->walker.full_path = path;
	ctx->walker.offset    = 0;
	set_initial_dir(ctx);

	// Parse the first component before posting any IO
	path_walker_advance(&ctx->walker);
	make_fat_83_name(ctx->walker.component, ctx->walker.component_83);

	uint32_t start_lba         = cluster_to_lba(ctx->current_cluster);
	ctx->sector_in_cluster    = 0;

	fs_open_post_io(ctx, start_lba, FSOPEN_PARSE_DIR_SECTOR);
}


// Completion handler wakes the requesting process
static void fs_open_finish(fs_open_ctx_t* ctx) {
    int ok = (ctx->state == FSOPEN_DONE) ? 0 : -1;
		kfree(ctx);
	if (ctx->proc_id) {
		pcb_t* pcb = get_pcb_from_id(ctx->proc_id);
		pcb->tf.TF_A0 = ok ? (uintptr_t)ctx->out_file : 0;
		unblock_process(pcb);
		return;
	} else if(ctx->on_complete){
        op_complete_cb cb  = ctx->on_complete;
        void*    caller    = ctx->caller_ctx;
        kfree(ctx);
        cb(caller, ok);
	}
}

void fs_open_step(void* raw_ctx, int status) {
	fs_open_ctx_t* ctx = (fs_open_ctx_t*)raw_ctx;

	if (status != 0) {
		ctx->state = FSOPEN_ERROR;
		fs_open_finish(ctx);
		return;
	}

	switch (ctx->state) {

		case FSOPEN_PARSE_DIR_SECTOR: {
																		// (SD_TX_RAM) holds one sector
																		FAT_DIRECTORY* dir = (FAT_DIRECTORY*)SD_TX_RAM;
																		int n = 512 / sizeof(FAT_DIRECTORY);

																		for (int i = 0; i < n; i++) {
																			FAT_DIRECTORY* e = &dir[i];

																			if (e->DIR_Name[0] == 0x00) {       // end of directory
																				ctx->state = FSOPEN_NOT_FOUND;
																				fs_open_finish(ctx);
																				return;
																			}
																			if (e->DIR_Name[0] == 0xE5) continue; // deleted
																			if ((e->DIR_Attr & 0x0F) == 0x0F) continue; // LFN

																			// Fixup the 0x05 → 0xE5 escape
																			char name0 = e->DIR_Name[0];
																			if (name0 == 0x05) name0 = 0xE5;

																			if (memcmp(e->DIR_Name, ctx->walker.component_83, 11) != 0)
																				continue;

																			// Match found 
																			uint32_t cluster = ((uint32_t)e->DIR_FstClusHI << 16)
																				| (uint32_t)e->DIR_FstClusLO;

																			if (!path_walker_has_next(&ctx->walker)) {
																				// This is the final component we have the file
																				ctx->out_file->first_cluster = cluster;
																				ctx->out_file->current_cluster = cluster;
																				ctx->out_file->current_sector  = 0;
																				ctx->out_file->current_byte    = 0;
																				ctx->out_file->fd              = 0;
																				ctx->out_file->file_offset     = 0;
																				ctx->out_file->file_size     = e->DIR_FileSize;
																				ctx->state = FSOPEN_DONE;
																				fs_open_finish(ctx);
																				return;
																			}

																			// It's a directory component descend into it
																			path_walker_advance(&ctx->walker);
																			ctx->current_cluster   = cluster;
																			ctx->sector_in_cluster = 0;
																			fs_open_post_io(ctx, cluster_to_lba(cluster),
																					FSOPEN_PARSE_DIR_SECTOR);
																			return;
																		}

																		// No match in this sector advance
																		ctx->sector_in_cluster++;
																		ctx->current_lba++;

																		if (ctx->sector_in_cluster >= fs_running_info->SECTORS_PER_CLUSTER) {
																			// Need to follow FAT chain to get next cluster
																			uint32_t fat_lba = fat_calcualte_lba(ctx->current_cluster);
																			fs_open_post_io(ctx, fat_lba, FSOPEN_PROCESS_FAT);
																		} else {
																			fs_open_post_io(ctx, ctx->current_lba, FSOPEN_PARSE_DIR_SECTOR);
																		}
																		return;
																	}

		case FSOPEN_PROCESS_FAT: {
															 // DMA buffer holds the FAT sector containing our cluster entry
															 uint32_t next = get_next_cluster(ctx->current_cluster); // reads from buffer

															 if (next >= 0x0FFFFFF8 || next == 0) {  // EOC or free
																 ctx->state = FSOPEN_NOT_FOUND;
																 fs_open_finish(ctx);
																 return;
															 }

															 ctx->current_cluster   = next;
															 ctx->sector_in_cluster = 0;
															 fs_open_post_io(ctx, cluster_to_lba(next), FSOPEN_PARSE_DIR_SECTOR);
															 return;
														 }

		default:
														 // DONE / NOT_FOUND / ERROR should not be stepped
														 return;
	}
}

