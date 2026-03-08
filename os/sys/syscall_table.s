ecall_table 
            defw ecall_sendLCDControl
            defw ecall_poll_serial_read
            defw ecall_poll_serial_write

            defw timer_start
            defw timer_stop
            defw timer_reset

            defw ecall_poll_buttons
