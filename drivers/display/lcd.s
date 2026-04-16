; --------------------
; lcd.s
;
; Simple LCD driver for sending data/instructions and printing decimal numbers.
; Provides:
;   - lcdWaitFree()      : wait until LCD is ready
;   - ecall_sendLCDControl() : write a byte to LCD (data/instruction)
;   - LCD_print_decimal()    : print decimal number to LCD
;
; Last modified: 14 March 2026 (HWF)

PORT_BASE       EQU 0x10000
LED_PORT        EQU 0x0
BUTTONS_PORT    EQU 0x1



WORD_LEN        EQU 4


; ---------------------- LCD Driver ----------------------
LCD_ADDR        EQU 0x10100
LCD_CLEAR_INST  EQU 0x1
TIMING_DELAY    EQU 8

; --------------------
; void lcdWaitFree()
;
; Waits until the LCD controller is ready to accept new data.
;
; Registers Used:
; t0 - address of LCD controller
; t1 - control value
; t2 - bus read
lcdWaitFree:
    ; push return address 
    subi    sp, sp, WORD_LEN
    sw      ra, [sp]

    ; Set R/W to 1 and RS to 0
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


ecall_sendLCDControl:
    addi sp, sp, -12
    sw a1, [sp]
    sw a0, 4[sp]
    sw ra, 8[sp]
    lw a0, TF_A0[a0]
    call sendLCDControl
    lw a1, [sp]
    lw a0, 4[sp]
    lw ra, 8[sp]
    addi sp, sp, 12
    ret

; --------------------
; void sendLCDControl(byte data, bool reg_select)
;
; Sends a byte to the LCD as either data (RS=1) or instruction (RS=0).
;
; Registers Used:
; a0 - data / instruction to send
; a1 - register select (1) is write data, 0 is write instruction
; s0 - LCD controller address
; s1 - value of LCD control byte
; t0 - status byte read
sendLCDControl:
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
    lw      ra, 12[sp]
    lw      s0, 8[sp]
    lw      s1, 4[sp]
    lw      a0,  [sp]
    addi    sp, sp, 16
    ret


; --------------------
; void LCD_print_decimal(uint32_t num)
;
; Prints a decimal number to the LCD.
; Converts number to ASCII, pushes onto stack, then prints.
;
; a0 - trap frame input
; Registers Used:
; a0 - decimal number to print
; a1 - RS for LCD (always 1 for data)
; s0 - temporary copy of number
; s1 - number of digits (stack counter)
; t0 - divisor / loop temp
; t1 - remainder for conversion
; t2 - saved register
ecall_LCD_print_decimal:
    subi sp, sp, 16
    sw t2, 12[sp]
    sw a1, 8[sp]
    sw a0, 4[sp]
    sw ra, [sp]
    lw a0, TF_A0[a0]
    mv s0, a0
    li a0, 0b10000000               ; set cursor to home
    mv a1, zero
    call sendLCDControl

    mv s1, zero
    li t0, 10

convert_ascii:
    beqz s0, convert_ascii_end
    rem t1, s0, t0
    div s0, s0, t0
    addi t1, t1, '0'
    subi sp, sp, 4
    sw t1, [sp]
    addi s1, s1, 1
    j convert_ascii

convert_ascii_end:

print_dec_loop:
    beqz s1, print_dec_loop_end
    lw a0, [sp]
    addi sp, sp, 4
    li a1, 1
    call sendLCDControl
    subi s1, s1, 1
    j print_dec_loop

print_dec_loop_end:
    lw t2, 12[sp]
    lw a1, 8[sp]
    lw a0, 4[sp]
    lw ra, [sp]
    addi sp, sp, 16
    ret


