; --------------------
; buttons.s
;
; Driver for reading button input from the hardware port.
; Provides a simple interface for the kernel/user code
; to poll buttons.
;
; Last modified: 27 Feb 2026 (HWF)
;--------------------


; --------------------
; uint8_t ecall_poll_buttons(void)
;
; Reads the current state of the buttons from the hardware port.
; Returns a byte with button bits set (1 = pressed, 0 = not pressed)
;
; Registers Used:
; a0 - returned button state
; t0 - temporary base address of the buttons port
ecall_poll_buttons
    li t0, PORT_BASE
    lb a0, BUTTONS_PORT[t0]
    ret
