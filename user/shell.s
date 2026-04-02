user_main align

    main_loop
        jal poll_input
        jal handle_input
    j   main_loop



;--------------
; char poll_input()
; return char which is the latest character captured or NULL
poll_input
    li  a7, 1                           ; poll serial reg
    ecall
    ret

;-----------
; void handle_input(byte input)
; Registers
handle_input
    subi    sp, sp, 4
    sw      ra, [sp]
    
    ; check if a0 has a value
    beqz    a0, handle_input_end
    li      a7, 2                       ; serial write (write char back to the terminal which has the view of echo)
    ecall

    jal printChar                       ; write char to the lcd display

    handle_input_end
    lw      ra, [sp]
    addi    sp, sp, 4
    ret
    
printChar
    li      a1, 1                           ; set RS to 1
    li      t1, 32
    bge     a0, t1, call_lcd_control
    ; check \n char
    li      t1, '\n'
    bne     a0, t1, test_clear_control
    mv      a1, zero                        ; set a1 to zero 
    j       call_lcd_control

    test_clear_control
    li      t1, '\n'

    call_lcd_control
    li      a7, 0                           ; ecall for sendLCDControl 
    ecall
    ret


user_stack_base DEFS 400
user_stack
