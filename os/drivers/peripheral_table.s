; --------------------
; Peripheral Interrupt Table
; --------------------
peripheral_interrupt_table:
    defw ecall_x
    defw ecall_x
    defw ecall_x
    defw ecall_x
    defw timer_peripheral_handle_interrupt ; timer interrupt
    defw ecall_x               ; Button
    defw ecall_x               ; Video
    defw ecall_x               ; Video
    defw ecall_x               ; Serial Tx
    defw ecall_x               ; Serial Rx
    defw ecall_x               ; Serial Error
