#include <process.inc>
; int ecall_getpid(trap_frame_t* tf)
; returns -1 on failure
ecall_getpid:
	lw t0, current_pcb 
	beqz t0, %F1

	lw a0, PCB_PID[t0]
	j %F2

	1
	li a0, -1
	2
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

alloc_pcb:
	la a0, pcb_slab_head
	tail slab_get

free_pcb:
		mv a1, a0
		la a0, pcb_slab_head
		tail slab_free
; fork
; int fork(void);
; failure = -1
fork:
; allocate rodata+data+bss+heap+stack
; make a new pcb
la t0, pcb_slab_head
; IF NOTHING FAILS THEN
; get a new proc_id
; set a0 - proc_id
; set a0 of new pcb trap to be 0
; modify pcb chain (maybe make a remove pcb func)
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

