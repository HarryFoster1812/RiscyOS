#include <fifo.inc>

; a0 pointer to fifo struct
; a1 byte to push
; if overflow then the oldest data is overwritten and the head is incremented
fifo_push:
	lw t0, FIFO_BASE[a0]

	lb t1, FIFO_HEAD[a0]
	lb t2, FIFO_TAIL[a0]

	lb t3, FIFO_SIZE[a0]

  add t4, t0, t2
	sb a1, [t4] ; store byte

	; increment tail
	addi t2, t2, 1
  remu t2, t2, t3

	sb t2, FIFO_TAIL[a0] ; store new tail

	bne t1, t2, fifo_push_exit ; check if tail == head

	; overflow has occured (increment head)
	addi t1, t1, 1
  remu t1, t1, t3
	1
	sb t1, FIFO_HEAD[a0] ; store new head

	fifo_push_exit:
	ret

; a0 pointer to fifo struct
fifo_pop:

	lw t0, FIFO_BASE[a0]

	lb t1, FIFO_HEAD[a0]
	lb t2, FIFO_TAIL[a0]


	beq t1, t2, fifo_pop_fail ; if head == tail fail
	; else return item
	lb t2, FIFO_SIZE[a0] ; overwrite tail since no longer needed
	add t3, t1, t0
	lb a0, [t3]       ; load contents of buffer
	addi t1, t1, 1    ; increment head and rollover
  remu t1, t1, t2 
	sw t1, FIFO_HEAD[a0] ; store new head
	ret

	fifo_pop_fail:
	mv a0, zero
	ret
