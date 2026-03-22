; --------------------
; void ecall_handler()
;
; Handles system calls (ECALL) from user code.
; Dispatches to the appropriate routine in ecall_table.
;
; Registers Used:
; sp - stack pointer
; ra - return address
; a7 - syscall number
; t0 - temporary for table base / limit
; t1 - temporary for table offset
ecall_handler:
    subi sp, sp, 4
    sw ra, [sp]

		; increment the return address of ecall
		csrr t0, MEPC
		addi t0, t0, 4
		csrw MEPC, t0

    li t0, ecall_max
    bgeu a7, t0, %F1          ; invalid syscall

    la t0, ecall_table
    slli t1, a7, 2                ; offset in table
    add t0, t0, t1
    lw t0, [t0]
    jalr t0                          ; jump to ECALL routine

		1
    lw ra, [sp]
    addi sp, sp, 4
    ret
