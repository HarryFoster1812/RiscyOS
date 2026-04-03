# --------------------
# Peripheral Interrupt Table
# --------------------
peripheral_interrupt_table:
    .word ecall_x
    .word ecall_x
    .word ecall_x
    .word ecall_x
    .word timer_peripheral_handle_interrupt # timer interrupt
    .word ecall_x               # Button
    .word ecall_x               # Video
    .word ecall_x               # Video
    .word ecall_x               # Serial Tx
    .word ecall_x               # Serial Rx
    .word ecall_x               # Serial Error
