; TODO: MEMORY SEGMENT NEEDS TO EXPOSE:
; AQUIRE SEGMENT - Will increment a share counter
; RELEASE SEGMENT - once the share counter is 0 then we can safely call ufree


make_memory_segment:
	li a0, MEMORY_REGION_STRUCT_SIZE
	tail kmalloc


; a0 - memory_segment_t*
aquire_memory_segment:
beqz a0, aquire_memory_segment_exit
	lw t0, MEMORY_REGION_REF_COUNT[a0]
	addi t0, t0, 1
	sw t0, MEMORY_REGION_REF_COUNT[a0]
aquire_memory_segment_exit:
	ret

release_memory_segment:
beqz a0, aquire_memory_segment_exit
	lw t0, MEMORY_REGION_REF_COUNT[a0]
	addi t0, t0, -1
	beqz t0, memory_segment_free
	sw t0, MEMORY_REGION_REF_COUNT[a0]
	ret
	memory_segment_free:
	addi sp, sp, -8
	sw a0, [sp]
	sw ra, 4[sp]
; void ufree(void* addr, int size)
	lw a1, MEMORY_REGION_SIZE[a0]
	lw a0, MEMORY_REGION_PHYSICAL_BASE[a0]
	call ufree
	lw a0, [sp]
	lw ra, 4[sp]
	addi sp, sp, 8
	tail kfree
