SERIAL_PORT             EQU     0x1_0500
SERIAL_CONTROL_OFFSET   EQU     0x4


ecall_poll_serial_read
    mv      a0, zero
    li      t0, SERIAL_PORT
    lb      t1, SERIAL_CONTROL_OFFSET[t0]           ; read control byte
    andi    t1, t1, 2                               ; 
    beqz    t1, ecall_poll_serial_read_end          ; check if the Receiver ready bit (RxRDY) is high
    lb      a0, [t0]                                ; read  byte from serial
    ecall_poll_serial_read_end:
    ret

ecall_poll_serial_write
    li      t0, SERIAL_PORT
    
    ecall_poll_serial_write_loop:
        lbu     t1, SERIAL_CONTROL_OFFSET[t0]        ; Read the status byte [2, 3]
        andi    t1, t1, 1                           ; Mask bit 0 (TxRDY) [4, 5]
        beqz    t1, ecall_poll_serial_write_loop    ; If 0, transmitter is busy; poll again [5]
        
        sb      a0, [t0]                            ; Write the byte to the Data register (offset 0) [3, 5]
        ret                                         ; Return to caller [6, 7]
