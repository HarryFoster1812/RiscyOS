; --------------------
; int logBaseTwo(int value)
;
; Computes log2 of input (assumes power of two)
;
; Registers Used:
; a0 - input / result
; t0 - temporary for shifting
; t1 - temporary for comparison
logBaseTwo:
    beqz a0, logBaseTwo_bad_input     ; Handle zero input
    mv t0, a0                         ; Copy input to temporary register
    li a0, 0                           ; Initialize result (log2)

logBaseTwo_loop:
    srli t0, t0, 1                    ; Shift t0 right by 1 (divide by 2)
    addi a0, a0, 1                    ; Increment log2 result
    bnez t0, logBaseTwo_loop          ; Continue until t0 becomes 0

logBaseTwo_end:
    ret

logBaseTwo_bad_input:
    li a0, -1                          ; Return error code for invalid input
    ret
