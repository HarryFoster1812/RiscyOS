; 256 KiB
; User ram:
; start:    4_0000
; end:      8_0000

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
init_alloc:
la t0, alloc_bitmap
li t1, ALLOC_WORD_SIZE
add t0, t0, t1
1:
beqz t1, 2f
sw zero, [t0]
addi t1, t1, -1
j 1b
2:
ret

; for (int i=0;i<128;++i) bitmap[i]=0

; User allocate (this will allocate user memory)
; a0 bytes to allocate
ualloc:
    beqz a0, 2f
    call round_next_power_of_two
    li t0, MIN_BLOCK
    bge a0, t0, 1f
    mv a0, t0 ; max min block 
    1:
    ; div by min block size to get the number of contiguous bits needed
    div t1, a0, t0


2: ; alloc fail
    li a0, 1 ; 1 error no
    ret

; given a power of 2
; size allocate 
; a0 - number of contiguous bits to allocate
; a0 - memory address allocated (NULL if fail)
try_alloc_block:
    la t0, alloc_bitmap



; kernel memory allocate
; this will user the internal kernel heap to allocate memory
kmalloc:

kfree:

