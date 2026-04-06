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


; fork
; int fork(void);
; failure = -1
fork:
; allocate rodata+data+bss+heap+stack
; make a new pcb
la pcb_slab_head
; IF NOTHING FAILS THEN
; get a new proc_id
; set a0 - proc_id
; set a0 of new pcb trap to be 0
; modify pcb chain (maybe make a remove pcb func)
ret

; execv
; 0x0 - _crt0 text (.text + .data + .bss)
; heap_base ↑
;   argv strings
;   argv array (pointers)
; heap_ptr ↑
; stack ↓

ecall_execv:
	addi sp, sp, -4 
	sw ra, [sp]

	lw ra, [sp]
	addi sp, sp, 4
	ret

