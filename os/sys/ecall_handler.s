
ecall_handler
  subi sp, sp, 4
  sw   ra, [sp]
	li      t0, ecall_max
	bgeu    a7, t0, ecall_x ; a7 will be the syscall number
	la      t0, ecall_table
	slli    t1, a7, 2       ; word align
	add     t0, t0, t1      ; calculate offset from table start
	lw      t0, [t0]        ; load address of ecall
	jr      t0              ; jump to ecall

    ; add 4 to mret to get the correct return address
    csrrw   t0, MEPC, t0
    addi    t0, t0, 4
    csrrw   t0, MEPC, t0
  lw   ra, [sp]
  addi sp, sp, 4
    ret

ecall_x
 ret                 
