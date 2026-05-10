#include <fifo.inc>
SERIAL_PORT             EQU     0x1_0500
SERIAL_CONTROL_OFFSET   EQU     0x4

// plan have tasks
// so serial read  -> check task head
// if there is a task then read into task struct which tells which process is waiting
// read is first come first serve so proc 1 read ou


irsq_serial_read:
    mv      a0, zero
    li      t0, SERIAL_PORT
    lb      t1, SERIAL_CONTROL_OFFSET[t0]           ; read control byte
    andi    t1, t1, 2                               ; 
    beqz    t1, irsq_serial_read_end          ; check if the Receiver ready bit (RxRDY) is high
    lb      a0, [t0]                                ; read  byte from serial
    tail tty_enqueue_recieve											; pass byte to tty
irsq_serial_read_end:
		ret

; a0 - char char_to_write
irsq_serial_write:
		addi sp, sp, -4
		sw ra, [sp]
		call tty_dequeue_write ; get a byte to write
		beqz a0, irsq_serial_write_end ; if we get nothing then return
    ; if so get string
    li t0, SERIAL_PORT
		sb      a0, [t0]                            ; Write the byte to the Data register
irsq_serial_write_end:
		ret                                         ; Return to caller 

; a0 - debug string
k_dbg_print:
	li      t0, SERIAL_PORT
	1
	lbu     t1, SERIAL_CONTROL_OFFSET[t0]        ; Read the status byte
	andi    t1, t1, 1                           ; Mask bit 0 (TxRDY)
	beqz    t1, %B1
	lb			t2, [a0]
	beqz    t2, %F2
	sb      t2, [t0]                            ; Write the byte to the Data register (offset 0)
	addi		a0, a0, 1
	j				%B1
	2
	ret                     

ALIGN 4

