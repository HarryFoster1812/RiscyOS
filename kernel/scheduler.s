#include <process.inc>
; get pid
; int ecall_getpid(void)

; exit
; void ecall_exit(int status)

; yield
; void ecall_yield(void)

; void schedule(trap_frame_t * tf)
; this is the top level schedule that is called by timer interrupt
; it will try to schedule the next process otherwise it will switch to the idle process
schedule:
	addi sp, sp, -4
	sw ra, [sp]

	mv s0, a0
	la a2, current_pcb

	call schedule_next
	bnez a0, %F1
	la a0, IDLE_TASK_PCB
	mv a2, a0
	1

	; a2 - current pcb
	; a1 - pointer to target pcb
	mv a1, a0
	; a0 - trap frame
	mv a0, s0
	call context_switch

	sw ra, [sp]
	addi sp, sp, 4
	ret

; pcb_t* schedule_next();
; returns null on error (no availible process to schedule)
schedule_next:
	addi sp, sp, -4
	sw s0, [sp]

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

	; the process is the one we started with then we need to check if it is currently running
	; we know it is not ready so it must be either RUNNING or BLOCKED
	li t2, STATE_RUNNING
	beq t1, t2, %F1
	mv t0, zero ; return null (failure to schedule a process)

	1
	mv a0, t0

	lw s0, [sp]
	addi sp, sp, 4
	ret



block_current_process:
	lw t0, current_pcb
	li t1, STATE_BLOCKED
	sb t1, PCB_STATUS[t0]
	ret

; void unblock_process(pcb_t *proc)
unblock_process:
	li t0, STATE_READY
	sb a0, PCB_STATUS[t0]
	ret

