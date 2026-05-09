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

; a0 - destination buffer
; a1 - bye to set
; a2 - number of bytes
memset:
	mv t0, a0
	mv t2, a2

	j memset_condition

	memset_loop
	sb a1, [t0]
	addi t2, t2, -1
	addi t0, t0, 1

	memset_condition
	bnez t2, memset_loop
	ret

; a0 = ptr1
; a1 = ptr2
; a2 = count
; returns:
;   a0 = 0   if equal
;   a0 < 0   if ptr1 < ptr2
;   a0 > 0   if ptr1 > ptr2
memcmp:
    mv t0, a0          ; p1
    mv t1, a1          ; p2
    mv t2, a2          ; count

memcmp_loop:

    beqz t2, memcmp_equal

    lbu t3, [t0]
    lbu t4, [t1]

    bne t3, t4, memcmp_diff

    addi t0, t0, 1
    addi t1, t1, 1
    addi t2, t2, -1

    j memcmp_loop

memcmp_diff:

    sub a0, t3, t4
    ret

memcmp_equal:

    li a0, 0
    ret
