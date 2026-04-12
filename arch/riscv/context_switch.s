; a0 - trap frame
; a1 - pointer to target pcb
; a2 - current pcb
context_switch:
	addi sp, sp, -16
	sw a2, [sp]
	sw a1, 4[sp]
	sw a0, 8[sp]
	sw ra, 12[sp]

	; copy trap frame to current pcb
	; a0 src
	mv a1, a2 
	; a1 bytes
	li a2, TRAP_FRAME_SIZE
	call memcpy

	; overwrite trap frame with target pcb

	lw a0, [sp] ; src = target pcb
	lw a1, 8[sp] ; dest = trap frame
	call memcpy

	lw a2, [sp]
	lw a1, 4[sp]
	lw a0, 8[sp]
	; trap frame contents should of been changed

	; load new values of CSR 
	lw t0, PCB_MEPC[a1]
	lw t1, PCB_MSTATUS[a1]
	lw t2, PCB_MSCRATCH[a1]

	; swap new and load old
	csrrw t0, MEPC, t0
	csrrw t1, MSTATUS, t1
	csrrw t2, MSCRATCH, t2

	; store old values into current pcb

	sw t0, PCB_MEPC[a2]
	sw t1, PCB_MSTATUS[a2]
	sw t2, PCB_MSCRATCH[a2]

	; switch the MMU
	mv a0, a1
	call mmu_set_proc

	lw ra, 12[sp]
	addi sp, sp, 16
	ret
