.equ SERIAL_PORT,      0x10500
.equ SERIAL_CONTROL_OFFSET,      0x4


serial_irsq_read:
    mv      a0, zero
    li      t0, SERIAL_PORT
    lb      t1, SERIAL_CONTROL_OFFSET(t0)           # read control byte
    andi    t1, t1, 2                               # 
    beqz    t1, ecall_poll_serial_read_end          # check if the Receiver ready bit (RxRDY) is high
    lb      a0, (t0)                                # read  byte from serial
    ecall_poll_serial_read_end:
    ret

ecall_poll_serial_write:
    li      t0, SERIAL_PORT
    
    ecall_poll_serial_write_loop:
        lbu     t1, SERIAL_CONTROL_OFFSET(t0)        # Read the status byte [2, 3]
        andi    t1, t1, 1                           # Mask bit 0 (TxRDY) [4, 5]
        beqz    t1, ecall_poll_serial_write_loop    # If 0, transmitter is busy; poll again (5)
        
        sb      a0, (t0)                            # Write the byte to the Data register (offset 0) [3, 5]
        ret                                         # Return to caller [6, 7]

# a0 - debug string
k_dbg_print:
	li      t0, SERIAL_PORT
	1
	lbu     t1, SERIAL_CONTROL_OFFSET(t0)        # Read the status byte [2, 3]
	andi    t1, t1, 1                           # Mask bit 0 (TxRDY) [4, 5]
	beqz    t1, %B1
	lb			t2, (a0)
	beqz    t2, %F2
	sb      t2, (t0)                            # Write the byte to the Data register (offset 0) [3, 5]
	addi		a0, a0, 1
	j				%B1
	2
	ret                                         # Return to caller [6, 7]

ecall_poll_serial_read:
	# Non-blocking (can return null)
	addi sp, sp, -4
	sw ra, (sp)
	mv s0, a0 # store the trap frame
	call fifo_pop
	sw a0, TF_A0(s0)
	lw ra, (sp)
	addi sp, sp, 4
	ret


# a0 byte to push
# t0 - int* FIFO_HEAD
# t1 - FIFO_HEAD
# t2 - int* FIFO_TAIL
# t3 - FIFO_TAIL
# if overflow then the oldest data is overwritten and the head is incremented
fifo_push:

	la t0, FIFO_HEAD
	lw t1, (t0)

	la t2, FIFO_TAIL
	lw t3, (t2)

	sb a0, (t3) # store byte

	li t4, FIFO_SIZE 
	li t5, fifo_base
	add t4, t5, t4 # pointer to last value 

	# increment tail
	addi t3, t3, 1
	bne t4, t3, tail_wrap_false 
	mv t3, t5

	tail_wrap_false:

	sw t3, (t2) # store new tail

	bne t1, t3, fifo_push_exit # check if tail == head

	# overflow has occured (increment head)
	addi t1, t1, 1
	bne t4, t1, %F1  # if head == last then wrap 
	mv t1, t5
	1
	sw t1, (t0) # store new head

	fifo_push_exit:
	ret

# t0 - int* FIFO_HEAD
# t1 - FIFO_HEAD
# t2 - int* FIFO_TAIL
# t3 - FIFO_TAIL
fifo_pop:
	la t0, FIFO_HEAD
	lw t1, (t0)

	la t2, FIFO_TAIL
	lw t3, (t2)

	beq t1, t3, fifo_pop_fail # if head == tail fail
	# else return item
	li t2, FIFO_SIZE # overwrite tail since no longer needed
	li t3, fifo_base
	add t2, t2, t3
	lb a0, (t1)       # load contents of buffer
	addi t1, t1, 1    # increment head and rollover
	bne t2, t1, %F1 # if head == last then wrap 
	mv t1, t3

	1
	sw t1, (t0)
	ret

	fifo_pop_fail:
	mv a0, zero
	ret

fifo_base:
  .zero 10
.align 2

STRUCT_BEGIN
WORD(FIFO_BASE)
WORD(FIFO_HEAD)
WORD(FIFO_TAIL)
WORD(FIFO_SIZE)
STRUCT_END(FIFO_STRUCT_SIZE)
