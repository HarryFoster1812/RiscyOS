#include "io/fs/fs_seek.h"
#include <io/sd_functions.h>
#include <types.h>
#include <mm.h>
#include <util.h>
#include <io/fs/fs_open.h>
#include <io/fs/fs_read.h>
#include <io/fs/fs_close.h>
#include <elf/elf_header.h>
#include <process.h>
#include <elf/elf_section_header.h>
#include <elf/elf_program_header.h>

typedef enum {
	ELF_OPENING,
	ELF_READING_HEADER,
	ELF_LOAD_PHEADER,     /* waiting for seek-to-phdr-table to complete  */
	ELF_READ_PHEADER,     /* waiting for a Phdr read to complete          */
	ELF_SEEK_PCONTENT,    /* waiting for seek-to-segment-data to complete */
	ELF_LOAD_PCONTENT,    /* waiting for segment payload read to complete  */
	ELF_CLOSING,
	ELF_DONE,
	ELF_ERROR,
} elf_state_t;

typedef struct{
    int parent_dir_cluster;// this is the cluster no of the parent directory 
    memory_region_t* ptext_memory_region; // this is text section base offset
    memory_region_t* pdata_memory_region; // this is text section base offset
		void* pdata_start; // this is the .rodata section and is used for data MMU virt start
		void* heap_start;  // pdata_start -> heap_start = rodata + data + bss 
    int mepc;
} process_image_t;

typedef struct {
	elf_state_t state;
	FILE        file;
	Elf32_Ehdr  header;
	Elf32_Phdr  pheader;
	int         current_seg; /* index of the Phdr currently being processed */
	process_image_t tmp_image;
	uint8_t     proc_id;
	pcb_t*      pcb;
	void*       seg_buf;     /* physical buffer for the segment being loaded */
} elf_ctx_t;

void elf_step(void* raw_ctx, int status);

void elf_load_submit(const char* path, uint8_t proc_id, pcb_t* pcb_to_fill) {
	elf_ctx_t* ctx = kmalloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));
	ctx->state   = ELF_OPENING;
	ctx->proc_id = proc_id;
	ctx->pcb     = pcb_to_fill;
	ctx->tmp_image.pdata_memory_region=NULL;
	ctx->tmp_image.ptext_memory_region=NULL;
	fs_open_submit_with_dir_parent(path, &ctx->file, 0, elf_step, ctx, &ctx->tmp_image.parent_dir_cluster);
}

/*
 * Advance to the next program header.
 * current_seg is incremented first so it always equals the index
 * of the Phdr we are about to read / have just finished with.
 */
static void elf_read_next_program_header(elf_ctx_t* ctx) {
	ctx->current_seg++;
	if (ctx->current_seg < ctx->header.e_phnum) {
		ctx->state = ELF_READ_PHEADER;
		fs_read_submit(&ctx->file, &ctx->pheader, sizeof(Elf32_Phdr),
				elf_step, ctx);
	} else {
		ctx->state = ELF_CLOSING;
		fs_close_submit(&ctx->file, elf_step, ctx);
	}
}

extern void free_pcb(pcb_t*);

static void elf_finish(elf_ctx_t* ctx) {
	if (ctx->state == ELF_ERROR) {
		if(ctx->tmp_image.ptext_memory_region)
			kfree(ctx->tmp_image.ptext_memory_region);

		if(ctx->tmp_image.pdata_memory_region)
			kfree(ctx->tmp_image.pdata_memory_region);
	}
	else {
		// edit all pcb content
		ctx->pcb->brk = ctx->tmp_image.heap_start;
		ctx->pcb->heap_start = ctx->tmp_image.heap_start;
		ctx->pcb->mepc = ctx->tmp_image.mepc;
		ctx->pcb->pdata_memory_region = ctx->tmp_image.pdata_memory_region;
		ctx->pcb->ptext_memory_region = ctx->tmp_image.ptext_memory_region;
		ctx->pcb->pdata_start = ctx->tmp_image.pdata_start;
		ctx->pcb->parent_dir_cluster = ctx->tmp_image.parent_dir_cluster;

		unblock_process(ctx->pcb);
	}
	kfree(ctx);
}

int elf_validate_header(Elf32_Ehdr* hdr) {
	if (!hdr)
		return false;
	if (hdr->e_ident[EI_MAG0] != ELFMAG0)
		return false;
	if (hdr->e_ident[EI_MAG1] != ELFMAG1)
		return false;
	if (hdr->e_ident[EI_MAG2] != ELFMAG2)
		return false;
	if (hdr->e_ident[EI_MAG3] != ELFMAG3)
		return false;
	if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
		return false;
	if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
		return false;
	if (hdr->e_machine != ELF_MACHINE_RISCV)
		return false;
	if (hdr->e_type != ET_EXEC)
		return false;

	return true;
}

void elf_step(void* raw_ctx, int status) {
	elf_ctx_t* ctx = (elf_ctx_t*)raw_ctx;

	if (status != 0) {
		ctx->state = ELF_ERROR;
		elf_finish(ctx);
		return;
	}

	if (ctx->state == ELF_OPENING) {
		ctx->state = ELF_READING_HEADER;
		fs_read_submit(&ctx->file, &ctx->header, sizeof(Elf32_Ehdr),
				elf_step, ctx);

	} else if (ctx->state == ELF_READING_HEADER) {
		if (!elf_validate_header(&ctx->header)) {
			ctx->state = ELF_ERROR;
			elf_finish(ctx);
			return;
		}
		ctx->current_seg = 0;
		ctx->state       = ELF_LOAD_PHEADER;
		/* e_phoff is an absolute file offset, so use SEEK_SET */
		fs_seek_whence(&ctx->file, ctx->header.e_phoff, SEEK_SET,
				elf_step, ctx);

	} else if (ctx->state == ELF_LOAD_PHEADER) {
		/* Seek to the program-header table is done; read Phdr[0] */
		ctx->state = ELF_READ_PHEADER;
		fs_read_submit(&ctx->file, &ctx->pheader, sizeof(Elf32_Phdr),
				elf_step, ctx);

	} else if (ctx->state == ELF_READ_PHEADER) {
		if (ctx->pheader.p_type != PT_LOAD) {
			elf_read_next_program_header(ctx);
			return;
		}

		/*
		 * Allocate a physical memory region for this segment.
		 * We zero the entire memsz region up front, which covers:
		 *   - the file bytes  [0 .. p_filesz)      loaded below
		 *   - the BSS bytes   [p_filesz .. p_memsz) left as zero
		 */
		memory_region_t* region = kmalloc(sizeof(memory_region_t));
		if (!region) {
			ctx->state = ELF_ERROR;
			elf_finish(ctx);
			return;
		}
		void* mem = ualloc(ctx->pheader.p_memsz);
		if (!mem) {
			kfree(region);
			ctx->state = ELF_ERROR;
			elf_finish(ctx);
			return;
		}
		memset(mem, 0, ctx->pheader.p_memsz);

		region->physical_base   = mem;
		region->region_size     = ctx->pheader.p_memsz;
		region->reference_count = 1;
		ctx->seg_buf            = mem;

		/*
		 * My linker script produces exactly two PT_LOAD segments:
		 *   text  – PF_R | PF_X (flags == 5)
		 *   data  – PF_R | PF_W (flags == 6)  contains rodata + data + bss
		 */
		if (ctx->pheader.p_flags & PF_X) {
			// text segment 
			ctx->tmp_image.ptext_memory_region = region;
			ctx->tmp_image.mepc = (int)ctx->header.e_entry;
		} else {
			// data segment (rodata + data + bss)
			ctx->tmp_image.pdata_memory_region = region;
			ctx->tmp_image.pdata_start         = (void*)ctx->pheader.p_vaddr;

			/*
			 * From the linker script:
			 *  _end = . after .bss == p_vaddr + p_memsz
			 *   __heap_stack_start = ALIGN(_end, 16)
			 *
			 * brk starts at heap_start; sbrk() grows it upward.
			 */
			uint32_t heap = (uint32_t)ctx->pheader.p_vaddr
				+ ctx->pheader.p_memsz;
			heap = (heap + 15u) & ~15u; // align to 16
			ctx->tmp_image.heap_start = (void*)heap;
		}

		ctx->state = ELF_SEEK_PCONTENT;
		fs_seek_whence(&ctx->file, ctx->pheader.p_offset, SEEK_SET,
				elf_step, ctx);

	} else if (ctx->state == ELF_SEEK_PCONTENT) {
		// Seek to segment data complete
		ctx->state = ELF_LOAD_PCONTENT;
		fs_read_submit(&ctx->file, ctx->seg_buf, ctx->pheader.p_filesz,
				elf_step, ctx);

	} else if (ctx->state == ELF_LOAD_PCONTENT) {
		/*
		 * Segment payload is in place.
		 * BSS (p_memsz - p_filesz bytes at the tail) is already zero
		 * from the memset done in ELF_READ_PHEADER.
		 */
		ctx->seg_buf = NULL;
		elf_read_next_program_header(ctx);

	} else if (ctx->state == ELF_CLOSING) {
		ctx->state = ELF_DONE;
		elf_finish(ctx);
	}
}
