; --------------------
; void delay(int time)
;
; Simple busy-wait loop.
;
; Registers Used:
; a0 - countdown value
DELAY:
delay_loop:
    subi a0, a0, 1
    bnez a0, delay_loop
    ret
