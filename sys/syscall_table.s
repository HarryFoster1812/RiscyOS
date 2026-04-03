/*  Define the ECALL handlers table */
// NOTE: Add ecalls here
#define ECALL_TABLE_LIST \
    X(ECALL_SEND_LCD_CONTROL, ecall_sendLCDControl)  \
    X(ECALL_LCD_PRINT_DECIMAL, ecall_LCD_print_decimal) \

/* Generate the table labels */
ECALL_TABLE_START:
#define X(name, func) ECALL_HANDLER_##name defw func __NL__
ECALL_TABLE_LIST
#undef X
ECALL_TABLE_END:

