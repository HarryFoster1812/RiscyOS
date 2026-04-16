INT_CONTROLLER_BASE EQU 0x10400
INT_ENABLE          EQU 0x4
INT_OUT             EQU 0x8

; --------------------
; void external_interrupt_handler()
;
; Handles all external interrupts from peripherals.
;
; Registers Used:
; sp - stack pointer
; ra - return address
; a0 - interrupt status
; t0 - table base / temporary
external_interrupt_handler:
    addi sp, sp, -8           ; Adjust stack pointer
    sw a0, 4[sp]                      ; Save interrupt status
    sw ra, [sp]                        ; Save return address

handle_interrupts:
    li t0, INT_CONTROLLER_BASE        ; Load interrupt controller base
    lw a0, INT_OUT[t0]                ; Read pending interrupts
    beqz a0, done_handling            ; If no interrupts, exit the loop

    call logBaseTwo                   ; Get index of first set bit

    la t0, peripheral_interrupt_table  ; Load base address of interrupt handlers
    slli a0, a0, 2                    ; Multiply index by 4 (word size)
    add a0, a0, t0                    ; Calculate address of the handler
    lw t0, [a0]                       ; Load interrupt handler address
    jalr t0                            ; Dispatch handler

    j handle_interrupts              ; Continue handling the next interrupt

done_handling:
    lw a0, 4[sp]                      ; Restore interrupt status
    lw ra, [sp]                        ; Restore return address
    addi sp, sp, 8                    ; Restore stack pointer
    ret

