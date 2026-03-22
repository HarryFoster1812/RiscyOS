TIMER_PORT EQU 0x10200

TIMER_COUNTER EQU 0x0
TIMER_LIMIT EQU 0x4
TIMER_STATUS EQU 0xC
TIMER_CLEAR EQU 0x10
TIMER_SET EQU 0x14

TIMER_MODULUS_DEFAULT EQU (10000-1) ; 10 ms

timer_init:
    subi sp, sp, 4
    sw ra, [sp]

    li a0, 1
    call timer_set_interrupt

    li a0, 2
    call timer_set_mode

    call timer_reset
    li a0, TIMER_MODULUS_DEFAULT
    call timer_set_modulus

    call timer_start
    
    lw ra, [sp]
    addi sp, sp, 4

    ret

; a0 - enable / disable
timer_set_interrupt:
    li t0, TIMER_PORT

    beqz a0, disable_int

enable_int:
    li t1, 8
    sw t1, TIMER_SET[t0]      ; control set
    ret

disable_int:
    li t1, 8
    sw t1, TIMER_CLEAR[t0]      ; control clear
    ret

timer_stop:
    li t0, TIMER_PORT
    li t1, 1 
    sw t1, TIMER_CLEAR[t0]
    ret

timer_start:
    li t0, TIMER_PORT
    li t1, 1 
    sw t1, TIMER_SET[t0]
    ret

; a0 - timer mode input:
; 0 - Free running
; 1 - One-Shot
; 2 - Reloadable (need to call timer_set_modulus)
; Other - ignored / slient failure
timer_set_mode:
    li t0, TIMER_PORT

    li t1, 3
    sw t1, TIMER_CLEAR[t0]

    li t2, 1
    beq a0, zero, mode_done 

    beq a0, t2, one_shot

    li t2, 2
    beq a0, t2, reloadable

    ret

one_shot:
    li t1, 4
    sw t1, TIMER_SET[t0]
    ret

reloadable:
    li t1, 2 
    sw t1, TIMER_SET[t0]

mode_done:
    ret

; a0 = modulus value (already -1 adjusted)
timer_set_modulus:
    li t0, TIMER_PORT
    sw a0, TIMER_LIMIT[t0]
    ret

timer_reset:
    subi sp, sp, 4
    sw ra, [sp]

    li t0, TIMER_PORT
    sw zero, TIMER_COUNTER[t0]
    call timer_stop

    lw ra, [sp]
    addi sp, sp, 4
    ret

timer_handle_interrupt:
    subi sp, sp, 8
    sw a0, 0[sp]
    sw ra, 4[sp]

    ; clear sticky bit
    li t0, TIMER_PORT
    li t1, 16
    sw t1, TIMER_CLEAR[t0]

		call keybord_scan_irsq

    lw a0, 0[sp]
    lw ra, 4[sp]
    addi sp, sp, 8
    ret
