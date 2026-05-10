#include <memory_region.inc>
#include <process.inc>
; int ecall_getpid(trap_frame_t* tf)
; returns -1 on failure
ecall_getpid:
	lw t0, current_pcb 
	beqz t0, %F1

	lw t0, PCB_PID[t0]
	j %F2

	1
	li t0, -1
	2
	sw t0, TF_A0[a0]
	ret


get_pcb_from_id:
	lw t0, current_pcb
	; check if current_pcb is null (this should not happen)
	beqz t0, %F1
	mv t3, t0 ; store the current pcb so we can detect if we go round in a circle
	2
  lbu t1, PCB_PID[t0] ; read id
	beq t1, a0, %F1 ; if the process is the target return pcb
	lw t0, PCB_NEXT[t0] ; walk pcb pointer

	bne t0, t3, %B2 ; if the process is not the one we started at then read the next one
  mv t0, zero ; this is a null failure
  1
  mv a0, t0
  ret

pcb_next_pid DEFW 0x0

; just a simple monotonic counter (not the best idea but its ok for now)
pid_alloc:
    la t0, pcb_next_pid
    lw a1, [t0]
    addi a1, a1, 1
    sw a1, [t0]
    ret

alloc_pcb:
	addi sp, sp, -4
	sw ra, [sp]
	la a0, pcb_slab_head
	call slab_get      ; a0 = pcb*

	call pid_alloc     ; a1 = pid

	sb a1, PCB_PID[a0]

	lw ra, [sp]
	addi sp, sp, 4
	ret

free_pcb:
		mv a1, a0
		la a0, pcb_slab_head
		tail slab_free

; fork
; int ecall_fork();
; failure = -1
ecall_fork:
	addi sp, sp, -16
	sw s0, 8[sp]
	sw ra, 12[sp]

	mv s0, a0

	lw a0, PCB_PTEXT_MEMORY_REGION[s0]
	call aquire_memory_segment

	lw a0, PCB_PDATA_MEMORY_REGION[s0]
	lw a0, MEMORY_REGION_SIZE[a0]
	call ualloc
	sw a0, 4[sp]
	beqz a0, fork_ualloc_fail
	call alloc_pcb
	beqz a0, fork_pcb_fail 
	sw a0, [sp] ; save new pcb
	call make_memory_segment
	beqz a0, fork_dmem_seg_fail
	call aquire_memory_segment ; increment count

	; everything is good so now we can fill these out
	lw t0, 4[sp] ; t0 = pointer to umem data region
	sw t0, MEMORY_REGION_PHYSICAL_BASE[a0]
	; copy the size from
	lw t1, PCB_PDATA_MEMORY_REGION[s0]
	lw t1, MEMORY_REGION_SIZE[t1]
	sw t1, MEMORY_REGION_SIZE[a0]
	sw a0, 4[sp]

	; STACK CONTENTS
	; CHILD PCB
	; NEW DMEM_REG (filled out)
	; s0
	; ra
	; void* memcpy(src, dest, size)
	mv a0, s0 ; parent is src
	lw a1, [sp]
	li a3, PCB_FORK_MEM_CPY_SIZE ; this copies everythingbut pid, ppid ect which is good
	call memcpy
	; memcpy will return the child pcb in a0
	; here all that is left is to:
	; - modify parent tf and set a0 to child pid
	lbu t0, PCB_PID[a0]
	sw t0, TF_A0[s0]

	; - set child tf a0 to 0
	sw zero, TF_A0[a0]
	; - set child ppid
	lbu t0, PCB_PID[s0]
	sb t0, PCB_PPID[a0]
	; store new data region for child
	lw t0, 4[sp]
	sw t0, PCB_PDATA_MEMORY_REGION[a0]
	; add onto the pcb linked list
	; (just link it to the parent)
	sw a0, PCB_NEXT[s0]
	; set status to ready
	li t0, STATE_READY
	sb t0, PCB_STATUS[a0]
	; copy current csr (mscratch and mepc)
	csrr t0, MEPC
	csrr t1, MSCRATCH
	sw t0, PCB_MEPC[a0]
	sw t1, PCB_MSCRATCH[a0]

	j %F1

fork_dmem_seg_fail:
	lw a0, [sp]
	call free_pcb
fork_pcb_fail:
	lw a0, 4[sp] ; pointer to user memory
	lw a1, PCB_PDATA_MEMORY_REGION[s0]
	lw a1, MEMORY_REGION_SIZE[a1]
	call ufree

fork_ualloc_fail:
lw a0, PCB_PTEXT_MEMORY_REGION[s0]
call release_memory_segment
li t0, -1
sw t0, TF_A0[s0]
1
	lw s0, 8[sp]
	lw ra, 12[sp]
	addi sp, sp, 16
ret

; execv
; 0x0 - _crt0 text (.text + .data + .bss)
; heap_base 
;   argv strings
;   argv array (pointers)
; heap_ptr 
; stack 

ecall_execv:
	addi sp, sp, -4 
	sw ra, [sp]

	lw ra, [sp]
	addi sp, sp, 4
	ret

