PORT_BASE       EQU 0x10000
LED_PORT        EQU 0x0
BUTTONS_PORT    EQU 0x1

TIMING_DELAY    EQU 8

WORD_LEN        EQU 4



; ---------------------- LCD Driver ----------------------
LCD_ADDR        EQU 0x10100

; --------------------
; void lcdWaitFree()
;
; Registers Used:
; t0 - address of LCD controller
; t1 - control value
; t2 - bus read
lcdWaitFree
    ; push return address 
    subi    sp, sp, WORD_LEN
    sw      ra, [sp]

    ; Set R/¬W to 1 and RS to 0
    li      t0, LCD_ADDR
    li      t1, 0b01
    sb      t1, 1[t0]

    LCD_busy_loop
        xori t1, t1, 4              ; set enable bus
        sb t1, 1[t0]
        
        li a0, TIMING_DELAY         ; call delay to streach pulse width
        jal DELAY

        lb t2, [t0]                 ; read status byte

        xori t1, t1, 4              ; take bus enable low
        sb t1, 1[t0]

        li a0, TIMING_DELAY         ; add another delay for pluse enable
        jal DELAY

        andi t2, t2, 128            ; check msb of bus
        bnez t2, LCD_busy_loop      ; loop back if not free
    
    lw ra, [sp]
    addi sp, sp, WORD_LEN
    ret

; --------------------
; void sendLCDControl(byte data, bool reg_select)
;
; Registers Used:
; a0 - data / instruction to send
; a1 - register select (1) is write data, 0 is write instruction
; s0 - LCD controller address
; s1 - value of LCD control byte
; t0 - status byte read
ecall_sendLCDControl
    ; push return address
    subi    sp, sp, 16
    sw      a0, [sp]                ; push data to set
    sw      s1, 4[sp]
    sw      s0, 8[sp]
    sw      ra, 12[sp]

    jal     lcdWaitFree
    
    ; start write
    li s0, LCD_ADDR

    ; set enable low and set rs to a1
    mv      s1, a1
    slli    s1, s1, 1
    sb      s1, 1[s0]

    ; pop original a0
    lw      a0, [sp]
    addi    sp, sp, 4

    ; copy data onto the bus
    sb      a0, [s0]

    ; set enable to high
    xori    s1, s1, 4
    sb      s1, 1[s0]

    ; extend the enable pulse
    li      a0, TIMING_DELAY
    jal     DELAY

    ; set enable to low
    xori    s1, s1, 4
    sb      s1, 1[s0]
    
    ; return
    lw      ra, 8[sp]
    lw      s0, 4[sp]
    lw      s1,  [sp]
    addi    sp, sp, 12
    ret



CLEAR_CHAR              DEFB "\f\0"                 
RIGHT_CURSOR_CHAR       DEFB "\t\0"                 
LEFT_CURSOR_CHAR        DEFB "\r\0"                 
align
