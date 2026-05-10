#include <fifo.inc>

; a0 pointer to fifo struct
; a1 byte to enqueue
; if overflow then it will be silently rejected
fifo_enqueue:
	lw t0, FIFO_BASE[a0]

	lb t1, FIFO_HEAD[a0]
	lb t2, FIFO_TAIL[a0]

	lb t3, FIFO_SIZE[a0]

  add t4, t0, t2

	; increment tail
	addi t2, t2, 1
  remu t2, t2, t3
	bne t1, t2, fifo_push_exit ; check if tail == head

	sb a1, [t4] ; store byte
	sb t2, FIFO_TAIL[a0] ; store new tail
	fifo_push_exit:
	ret

; a0 pointer to fifo struct
fifo_dequeue:

	lw t0, FIFO_BASE[a0]

	lb t1, FIFO_HEAD[a0]
	lb t2, FIFO_TAIL[a0]


	beq t1, t2, fifo_dequeue_fail ; if head == tail fail
	; else return item
	lb t2, FIFO_SIZE[a0] ; overwrite tail since no longer needed
	add t3, t1, t0
	lb a0, [t3]       ; load contents of buffer
	addi t1, t1, 1    ; increment head and rollover
  remu t1, t1, t2 
	sw t1, FIFO_HEAD[a0] ; store new head
	ret

	fifo_dequeue_fail:
	mv a0, zero
	ret
