; --------------------
; Peripheral Interrupt Table
; --------------------
peripheral_interrupt_table:
    defw sd_irsq_handler ; these are for the user peripheral
    defw sd_irsq_handler
    defw sd_irsq_handler
    defw sd_irsq_handler
    defw ecall_x ; timer interrupt
    defw ecall_x               ; Button
    defw ecall_x               ; Video
    defw ecall_x               ; Video
    defw ecall_x               ; Serial Tx
    defw ecall_x               ; Serial Rx
    defw ecall_x               ; Serial Overrun


PERIPHERAL_ENABLE_BITMASK  EQU 0b11100001111
