ecall_handler
	li t0, ecall_max
	bgeu a7, t0, ecall_x ; a7 will be the syscall number
	la t0, ecall_jump
	slli t1, a7, 2 ; word align
	add t0, t0, t1 ; calculate offset from table start
	lw t0, [t0] ; load address of ecall
	jr t0 ; jump to ecall

ecall_x
; BAD ECALL
