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
    ELF_READING_SEGMENT,
    ELF_CLOSING,
    ELF_DONE,
    ELF_ERROR,
} elf_state_t;

typedef struct {
    elf_state_t state;
    FILE file;
    Elf32_Ehdr header;
    Elf32_Phdr pheader;
    int current_seg;  // which program header we are loading
    uint8_t proc_id;      // process to launch when done
} elf_ctx_t;

void elf_step(void* raw_ctx, int status);

void elf_load_submit(const char* path, uint8_t proc_id) {
    elf_ctx_t* ctx  = kmalloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    ctx->state      = ELF_OPENING;
    ctx->proc_id    = proc_id;

    // Sub-operation: open the file.
    // on_complete = elf_step, so when open finishes it drives us forward.
    fs_open_submit(path, &ctx->file, 0, elf_step, ctx);
}

static void elf_submit_next_segment_read(elf_ctx_t* ctx) {
    // Advance past any non-loadable segments 
    while (ctx->current_seg < ctx->header.e_phnum) {
        Elf32_Phdr* phdr = &ctx->header.phdrs[ctx->current_seg];
        if (phdr->p_type == PT_LOAD) break;
        ctx->current_seg++;
    }

    if (ctx->current_seg >= ctx->header.e_phnum) {
        // No loadable segments left go straight to close
        ctx->state = ELF_CLOSING;
        fs_close_submit(&ctx->file, elf_step, ctx);
        return;
    }

    Elf32_Phdr* phdr = &ctx->header.phdrs[ctx->current_seg];
    fs_read_submit(&ctx->file, (void*)phdr->p_paddr, phdr->p_filesz, elf_step, ctx);
}

static void elf_finish(elf_ctx_t* ctx) {
    if (ctx->proc_id) {
        pcb_t* pcb    = get_pcb_from_id(ctx->proc_id);
        pcb->tf.TF_A0 = (ctx->state == ELF_DONE)
                            ? ctx->header.e_entry
                            : 0;
        unblock_process(pcb);
    }
    kfree(ctx);
}

int elf_validate_header(Elf32_Ehdr* hdr){
	if(!hdr) return false;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
		return false;
	}
	if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
		return false;
	}
	if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
		return false;
	}
	if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
		return false;
	}
	if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		return false;
	}
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		return false;
	}

	if(hdr->e_machine != ELF_MACHINE_RISCV) return false;
	if(hdr->e_type != ET_EXEC) return false;

	return true;

}

void elf_step(void* raw_ctx, int status) {
    elf_ctx_t* ctx = (elf_ctx_t*)raw_ctx;

    if (status != 0) {
        ctx->state = ELF_ERROR;
        elf_finish(ctx);
        return;
    }

    switch (ctx->state) {

    case ELF_OPENING:
        // open succeeded; now read the ELF header
        ctx->state = ELF_READING_HEADER;
        fs_read_submit(&ctx->file,
                       &ctx->header, sizeof(Elf32_Ehdr),
                       elf_step, ctx);
        return;

    case ELF_READING_HEADER:
        // header is in hdr_buf; validate and parse
        if (!elf_validate_header(&ctx->header)) {
            ctx->state = ELF_ERROR;
            elf_finish(ctx);
            return;
        }
        ctx->current_seg = 0;
        // fall through: load first segment
        ctx->state = ELF_READING_SEGMENT;
        elf_submit_next_segment_read(ctx);
        return;

    case ELF_READING_SEGMENT:
        // segment loaded into its target address (set up in submit)
        ctx->current_seg++;
        if (ctx->current_seg < ctx->header.e_phnum) {
            elf_submit_next_segment_read(ctx);  // more segments
        } else {
            ctx->state = ELF_CLOSING;
            fs_close_submit(&ctx->file, elf_step, ctx);
        }
        return;

    case ELF_CLOSING:
        ctx->state = ELF_DONE;
        elf_finish(ctx);
        return;

    default:
        return;
    }
}
