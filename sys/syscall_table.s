; Table of ECALL handlers
ECALL_TABLE_START:
;    ECALL_HANDLER_SEND_LCD_CONTROL      defw ecall_sendLCDControl
;    ECALL_HANDLER_LCD_PRINT_DECIMAL    defw ecall_LCD_print_decimal
;    ECALL_HANDLER_POLL_KEYBOARD        defw ecall_poll_keyboard
ECALL_TABLE_END:
