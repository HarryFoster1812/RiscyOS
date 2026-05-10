#include <tty/tty.inc>
#include <tty/rw_request.inc>
; this is the layer which will be called by the serial layer and will be responsible for capturing user input
; and echoing it back

ECHO_QUEUE_SIZE EQU 30
RX_QUEUE_SIZE EQU 100

TTY_INFO DEFW 0x0
; we are in init so s0 does not matter
tty_init:
	addi sp, sp, -4
	sw ra, [sp]
; create two queues tx and rx
	li a0, TTY_STRUCT_SIZE
	call kmalloc
	la s0, TTY_INFO
	sw zero, TTY_WRITE_QUEUE_HEAD[a0]
	sw zero, TTY_READ_QUEUE_HEAD[a0]
	sw a0, [s0]
	mv s0, a0

	li a0, RX_QUEUE_SIZE
	sb zero, FIFO_HEAD[s0]
	sb zero, FIFO_TAIL[s0]
	sb a0, FIFO_SIZE[s0]
	call kmalloc
	sw a0, FIFO_BASE[s0]

	addi s0, s0, ECHO_FIFO
	li a0, ECHO_QUEUE_SIZE
	sb zero, FIFO_HEAD[s0]
	sb zero, FIFO_TAIL[s0]
	sb a0, FIFO_SIZE[s0]
	call kmalloc
	sw a0, FIFO_BASE[s0]

	lw ra, [sp]
	addi sp, sp, 4	
	ret

tty_echo_backspace:
	call fifo_enqueue
	li a1, ' '
	call fifo_enqueue
	li a1, '\b'
	lw ra, [sp]
	addi sp, sp, 4
	tail fifo_enqueue

tty_echo_new_line:
	li a1, '\r'
	call fifo_enqueue
	li a1, '\n'
	lw ra, [sp]
	addi sp, sp, 4
	tail fifo_enqueue

tty_enqueue_recieve:
	addi sp, sp, -4
	sw ra, [sp]
	mv a1, a0
	la a0, TTY_INFO
	lw a0, [a0]
	addi a0, a0, ECHO_FIFO

	li t0, '\b'
	beq a1, t0, tty_echo_backspace
	li t0, '\n'
	beq a1, t0, tty_echo_new_line
	; else echo
	call fifo_enqueue
	lw t0, TTY_READ_QUEUE_HEAD-ECHO_FIFO[a0]
	lw t0, [t0]
	beqz t0, store_rx_fifo ; no read waiters
	; there is someone wating - store in buffer and handle done
	lw t1, USER_BUFFER[t0]
	sb a1, [t1] ; store in user buffer
	lbu t2, RW_BYTES_FUFILLED[t0]
	lbu t3, RW_BYTES_REQUESTED[t0]
	addi t2, t2, 1
	beq t2, t3, user_read_request_fufilled

	addi t1, t1, 1
	sw t1, USER_BUFFER[t0]
	sb t2, RW_BYTES_FUFILLED[t0]
	addi sp, sp, -4
	sw ra, [sp]
	ret

user_read_request_fufilled:
	lw t4, PROC_PCB[t0]
	sw t2, TF_A0[t4]
	mv a0, t4
	call unblock_process
	addi sp, sp, -4
	sw ra, [sp]
	tail kfree

store_rx_fifo:
	addi a0, a0, -ECHO_FIFO
	addi sp, sp, -4
	sw ra, [sp]
	tail fifo_enqueue


tty_dequeue_write:
	; try to get something from the echo
	addi sp, sp, -8
	sw s0, [sp]
	sw ra, 4[sp]
	la t0, TTY_INFO
	lw s0, [t0]
	addi a0, s0, ECHO_FIFO
	call fifo_dequeue
	bnez a0, tty_dequeue_write_end
	lw a0, TTY_WRITE_QUEUE_HEAD[s0]
	beqz a0, tty_dequeue_write_end
	; increment count of user request
	lw t0, USER_BUFFER[a0]
	lbu t1, RW_BYTES_FUFILLED[a0]
	lbu t2, RW_BYTES_REQUESTED[a0]
	addi t1, t1, 1
	beq t1,t2, user_write_request_fufilled
	; increment buffer
	lbu s0, [t0]
	addi t0, t0, 1
	sb t0, USER_BUFFER[a0]
	sb t1, RW_BYTES_FUFILLED[a0]
	mv a0, s0
	j tty_dequeue_write_end
	
user_write_request_fufilled:
	; modify  write head
	lw t2, RW_NEXT_REQUEST[a0]
	la t3, TTY_INFO
	sw t2, [t3]
	
	; read user buffer
	lbu s0, [t0]
	; save the request pointer 
	addi sp, sp, -4
	sw s1, [sp]
	mv s1, a0
	lw a0, PROC_PCB
	; update a0 to be the bytes fufilled
	sw t1, TF_A0[a0]
	call unblock_process
	mv a0, s1
	addi sp, sp, 4
	call kfree ; free the request
	mv a0, s0

tty_dequeue_write_end:
	lw s0, [sp]
	lw ra, 4[sp]
	addi sp, sp, 8
	ret

; a0 - buffer
; a1 - bytes to read
; a2 - head of queue to add to
tty_make_request:
	addi sp, sp, -16
	sw a0, [sp]
	sw a1, 4[sp]
	sw a2, 8[sp]
	sw ra, 12[sp]

	blez a1, tty_make_request_fail
	li a0, RW_REQUEST_SIZE
	call kmalloc
	blez a1, tty_make_request_fail
	lw t0, [sp] 
	sw t0, USER_BUFFER[a0]
	lw a1, 4[sp]
	sb a1, RW_BYTES_REQUESTED[a0]
	sb zero, RW_BYTES_FUFILLED[a0]
	sw zero, RW_NEXT_REQUEST[a0]
	la t0, current_pcb
	lw t0, [t0]
	sw t0, PROC_PCB[a0]

	call block_current_process
	; add request to the back of the queue
	lw t0, 8[sp] ; head pointer
rw_walk_list:
	lw t1, [t0]
	beqz t1, rw_add_to_back
	mv t0, t1
	addi t0, t0, RW_NEXT_REQUEST
	j rw_walk_list

	rw_add_to_back:
	sw a0, [t0]


tty_make_request_fail:
	mv a0, zero ; return null
tty_make_request_exit:
	lw ra, 12[sp]
	addi sp, sp, 16
	ret 

