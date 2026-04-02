; --------------------
; void external_interrupt_handler()
;
; Handles external interrupts from peripherals.
;
; Registers Used:
; sp - stack pointer
; ra - return address
; a0 - interrupt status
; t0 - table base / temporary
external_interrupt_handler:
    subi sp, sp, 8
    sw a0, 4[sp]
    sw ra, [sp]

    li t0, INT_CONTROLLER_BASE
    lw a0, INT_OUT[t0]           ; Read pending interrupts

    call logBaseTwo               ; Get index of first set bit

    la t0, peripheral_interrupt_table
    slli a0, a0, 2
    add a0, a0, t0
    lw t0, [a0]
    jalr t0                       ; Dispatch handler

    lw a0, 4[sp]
    lw ra, [sp]
    addi sp, sp, 8
    ret
	
; --------------------
; int logBaseTwo(int value)
;
; Computes log2 of input (assumes power of two)
;
; Registers Used:
; a0 - input / result
; t0 - temporary for shifting
; t1 - temporary for comparison
logBaseTwo:
    beqz a0, logBaseTwo_bad_input
    mv t0, a0
    mv a0, zero
    li t1, 1

logBaseTwo_innerloop:
    beq t1, t0, logBaseTwo_end
    srli t0, t0, 1
    addi a0, a0, 1
    j logBaseTwo_innerloop

logBaseTwo_end:
    ret

logBaseTwo_bad_input:
    li a0, -1
    ret
