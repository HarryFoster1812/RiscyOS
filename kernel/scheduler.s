#include "../include/process.inc"
; get pid
; int ecall_getpid(void)

; exit
; void ecall_exit(int status)

; yield
; void ecall_yield(void)

; this is the top level schedule that is called by timer interrupt
schedule:

; pcb_t* schedule_next();
; returns null on error (no availible process to schedule)
schedule_next:
	lw t0, current_pcb
	; check if current_pcb is null (This should never happen but maybe)
	beqz t0, %F1
	mv s0, t0 ; store the current pcb so we can detect if we go round in a circle
	li t2, STATE_READY
	2
	lw t0, PCB_NEXT[t0] ; get next pcb in queue
	lw t1, PCB_STATUS[t0] ; read status of next process
	beq t1, t2, %F1 ; if the process is ready then return that pcb 

	bne t0, s0, %B2 ; if the process is not the one we started at then read the next one
	mv t0, zero ; the process looped around so we should return null (failure to schedule a process)

	1
	mv a0, t0
	ret
