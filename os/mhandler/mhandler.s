mhandler
	csrrw sp, MSCRATCH, sp ; swap user and machine stack pointer
    ; save the context
    subi sp, sp, 12
	sw s1, 8[sp]
	sw s0, 4[sp]
	sw ra, [sp]


	csrr t0, MCAUSE 
    mret



; must be > 0 unsigned
DELAY 
delay_loop
	subi a0, a0, 1 ; 25 ns
	bnez a0, delay_loop ; if branch then 75 ns otherwise 25

ret ; 75
