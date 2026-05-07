; should not be called on idle-> idle
; three possibilities
; IF KIDLE != 0
; idle -> proc
; switch current_pcb 
; setup_mmu
; load csr
; disable kidle

; ELSE IF a0 == NULL
; proc -> idle
; store csr
; enable kidle

; ELSE
; proc -> proc
; switch pcb
; setup_mmu
; load csr
; store old csr
USER_MODE_MASK EQU ~(0b11000_0000_0000) ; this is a clear mask
SUPERVISOR_MODE_MASK EQU 0b01000_0000_0000 ; this is a or mask

; if the target pcb is NULL then we are switching to the idle
; a0 - pointer to target pcb
context_switch:
  lw t6, current_pcb

	addi sp, sp, -12
	sw t6, 0[sp]
	sw a0, 4[sp]
	sw ra, 8[sp]

  beqz a0, switch_to_idle


  ; swap the current pcb to the target
	; a0 src
  la t0, current_pcb
  sw a0, [t0]

	; switch the MMU
	call mmu_set_proc

	; load new values of CSR 
	lw t0, PCB_MEPC[a0]
	lw t1, PCB_MSTATUS[a0]
  ; MAKE 100% sure it goes to user mode
  li t3, USER_MODE_MASK
  and t1, t1, t3
	lw t2, PCB_MSCRATCH[a0]

	; swap new and load old
	csrrw t0, MEPC, t0
	csrrw t1, MSTATUS, t1
	csrrw t2, MSCRATCH, t2


  lb t0, kidle
  bnez t0, switch_from_idle
	; store old values into current pcb

  1
	sw t0, PCB_MEPC[t6]
	sw t1, PCB_MSTATUS[t6]
	sw t2, PCB_MSCRATCH[t6]
  
  j context_switch_exit

switch_to_idle:
  ; set kidle = true
  la t0, kidle
  addi t1, zero, 1
  sb t1, [t0]

  ; load correct values of kidle

  la t0, kernel_idle_task
  ; set supervisor mode
  csrr t1, MSTATUS
  li t4, SUPERVISOR_MODE_MASK
  or t1, t1, t4

  la t2, kernel_stack_base

	csrrw t0, MEPC, t0
	csrrw t1, MSTATUS, t1
	csrrw t2, MSCRATCH, t2
  
  j %B1


switch_from_idle:
  la t0, kidle
  sb zero, [t0]



  context_switch_exit:
	lw ra, 8[sp]
	addi sp, sp, 12
	ret
