; 256 KiB
; User ram:
; start:    4_0000
; end:      8_0000

.equ USER_RAM_START 0x40000

; 1 = USED 
; 0 = FREE

round_next_power_of_two:
;v--
;v |= v >> 1
;v |= v >> 2
;v |= v >> 4
;v |= v >> 8
;v |= v >> 16
;v++;

; unrolled loop is faster

addi a0, a0, -1

srli t0, a0, 1
or a0, a0, t0

srli t0, a0, 2
or a0, a0, t0

srli t0, a0, 4
or a0, a0, t0

srli t0, a0, 8
or a0, a0, t0

srli t0, a0, 16
or a0, a0, t0

addi a0, a0, 1
ret

; this will zero all of the bytes
ualloc_init:
	la t0, alloc_bitmap
	li t1, ALLOC_WORD_SIZE
	add t0, t0, t1
	1
	beqz t1, %F2
	sw zero, (t0)
	addi t1, t1, -1
	j %B1
	2
	ret


; User allocate (this will allocate user memory)
; a0 bytes to allocate
ualloc:
		addi sp, sp, -4
		sw ra, (sp)

    beqz a0, %F2
    call round_next_power_of_two
    li t0, MIN_BLOCK
    bge a0, t0, %F1
    mv a0, t0 ; max min block 
    1
    ; div by min block size to get the number of contiguous bits needed
    divu t1, a0, t0
    remu t2, a0, t0
		add a0, t1, t2
		call ualloc_try_block ; umem_alloc.c
		j %F3

		2 ; alloc fail
    li a0, 1 ; 1 - called ualloc with null
		3
		lw ra, (sp)
		addi sp, sp, 4
    ret

; void ufree(void* addr, int size)
ufree:
		addi sp, sp, -4
		sw ra, (sp)

    beqz a1, %B2 ; jump to alloc fail above (save space)
    call round_next_power_of_two
    li t0, MIN_BLOCK
    bge a1, t0, %F1
    mv a1, t0 ; max min block 
    1
    ; div by min block size to get the number of contiguous bits needed
    divu t1, a1, t0
    remu t2, a1, t0
		add a1, t1, t2
		
		li t1, USER_RAM_START
		sub a0, a0, t1
		div a0, a0, t0
		call mark_bits_free ; defined in umem_alloc.c

		j %B3
