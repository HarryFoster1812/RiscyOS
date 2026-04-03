; a0 - source buffer
; a1 - destination buffer
; a2 - size (bytes)
; returns: a1 (destination), like standard memcpy

memcpy:
    mv t0, a0        ; t0 = src
    mv t1, a1        ; t1 = dst
    mv t2, a2        ; t2 = remaining bytes

    beqz t2, memcpy_done

memcpy_loop:
    lb t3, [t0]     ; load byte from src
    sb t3, [t1]     ; store byte to dst

    addi t0, t0, 1   ; src++
    addi t1, t1, 1   ; dst++
    addi t2, t2, -1  ; size--

    bnez t2, memcpy_loop

memcpy_done:
    mv a0, a1        ; return destination
    ret
