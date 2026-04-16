; --------------------
; Peripheral Interrupt Table
; --------------------
peripheral_interrupt_table:
    defw sd_irsq_handler ; these are for the user peripheral
    defw sd_irsq_handler
    defw sd_irsq_handler
    defw sd_irsq_handler
    defw timer_peripheral_handle_interrupt ; timer interrupt
    defw ecall_x               ; Button
    defw ecall_x               ; Video
    defw ecall_x               ; Video
    defw ecall_x               ; Serial Tx
    defw ecall_x               ; Serial Rx
    defw ecall_x               ; Serial Overrun
