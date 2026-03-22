GPIO_BASE EQU 0x0001_0300
GPIO_DIR_OFFSET EQU 0x4
GPIO_CLEAR_OFFSET EQU 0x8
GPIO_SET_OFFSET EQU 0xC

COL_NO EQU 4
ROW_NO EQU 4
COLUMN_BIT_START EQU 8

ROW_CLEAR_BITMASK EQU 0xF00

keyboard_init:
	; init FIFO
	la t0, fifo_base
	la t1, FIFO_HEAD
	la t2, FIFO_TAIL
	sw t0, [t1]
	sw t0, [t2]

	; init GPIO Pins
	li t1, GPIO_BASE
	; set bits F0FF
	li t0, 0xFFFFF0FF
	sw t0, GPIO_DIR_OFFSET[t1]
	ret

ecall_poll_keyboard:
	; Non-blocking (can return null)
	addi sp, sp, -4
	sw ra, [sp]
	mv s0, a0 ; store the trap frame
	call fifo_pop
	sw a0, TF_A0[s0]
	lw ra, [sp]
	addi sp, sp, 4
	ret


; calculate bit mask of row
; read columns
; add nibble
; a0 - row number to scan
; a1 - pointer to result
; void scan_row(int row, int* result_bit_pattern)
; Note: Does not produce the correct bit pattern for more than 32 keys
scan_row:
  ; clear the gpio enable pins
  li t0, GPIO_BASE
  li t1, ROW_CLEAR_BITMASK 
  sw t1, GPIO_CLEAR_OFFSET[t0]

  ; calculate column bit mask
  li t2, COLUMN_BIT_START
  add t3, a0, t2
  li t4, 1
  sll t3, t4, t3
  sw t3, GPIO_SET_OFFSET[t0]

  ; calculate result bitmask
  li t3, COL_NO
  add t2, t3, t2       ; column start + no of columns = row start
  li t3, ROW_NO
  mul t5, a0, t3        ; result start bit
  sll  t3, t4, t3
  addi  t3, t3, -1
  
  lw t4, [t0]       ; load pin state
  srl t4, t4, t2    ; shit by row start to get row in bits [ROW_NO:0]
  and t4, t4, t3   ; bit mask to only get those bits
  
  lw t0, [a1] ; load result location 
  sll t4, t4, t5 ; append bits
  or t0, t4, t0
  sw t0, [a1]  ; restore

  ret

; a0 = raw scan (16-bit)
; Registers used: 
; t0-t5 = intermediate button processing
; s0 = key history array pointer
; s1 = index of current button
keypad_debounce:
	addi sp, sp, -12
	sw s1, [sp]
	sw s0, 4[sp]
	sw ra, 8[sp]

	la s0, KEY_HISTORY      ; array of 16 bytes, one per key
	li s1, 15                ; key index 0-15

	debounce_loop:
		srl t1, a0, s1          ; shift to LSB byt key index
		andi t2, t1, 1          ; extract raw bit

		add t4, s0, s1					; get byte offset
		lb t3, [t4]				      ; load key history
		slli t3, t3, 1
		or t3, t3, t2           ; shift in new sample
		sb t3, [t4]		          ; store history

		; check if history == 0xFF or 0x00
		li t4, 0xFF
		beq t3, t4, key_pressed
		beq t3, zero, key_released
		j next_key

		key_pressed:
		mv a0, s1									; a0 = index of key pressed
		call fifo_push_key
		j next_key

		key_released:

		next_key:
		addi s1, s1, -1 ; decrement key index
		bgez s1, debounce_loop

	lw s1, [sp]
	lw s0, 4[sp]
	lw ra, 8[sp]
	addi sp, sp, 12
	ret

; void scan_matrix
scan_matrix:
	addi sp, sp, -12
	sw zero, [sp] ; resultant bit field of scan
	sw s0, 4[sp]
	sw ra, 8[sp]

	mv a0, zero
	mv a1, sp												; a1 = &raw_scan_result
	li s0, COL_NO

	scan_matrix_loop:
	beq a0, s0, exit_scan_matrix_loop				; check if curr row == COL_NO
		call scan_row
		addi a0, a0, 1
		j scan_matrix_loop

	exit_scan_matrix_loop:
		lw a0, [sp]												; a0 = *raw_scan_result
		call keypad_debounce

	lw ra, 8[sp]
	lw s0, 4[sp]
	addi sp, sp, 12
	ret


; a0 - index of key to push
fifo_push_key:
	addi sp, sp, -4
	sw ra, [sp]

	; translate to keypress index -> ascii char
	call translate_matrix_code

	; push ascii char to buffer
	call fifo_push

	lw ra, [sp]
	addi sp, sp, 4
	ret

; a0 byte to push
; t0 - int* FIFO_HEAD
; t1 - FIFO_HEAD
; t2 - int* FIFO_TAIL
; t3 - FIFO_TAIL
; if overflow then the oldest data is overwritten and the head is incremented
fifo_push:

	la t0, FIFO_HEAD
	lw t1, [t0]

	la t2, FIFO_TAIL
	lw t3, [t2]

	sb a0, [t3] ; store byte

	li t4, FIFO_SIZE 
	li t5, fifo_base
	add t4, t5, t4 ; pointer to last value 

	; increment tail
	addi t3, t3, 1
	bne t4, t3, tail_wrap_false 
	mv t3, t5

	tail_wrap_false:

	sw t3, [t2] ; store new tail

	bne t1, t3, fifo_push_exit ; check if tail == head

	; overflow has occured (increment head)
	addi t1, t1, 1
	bne t4, t1, %F1  ; if head == last then wrap 
	mv t1, t5
	1
	sw t1, [t0] ; store new head

	fifo_push_exit:
	ret

; t0 - int* FIFO_HEAD
; t1 - FIFO_HEAD
; t2 - int* FIFO_TAIL
; t3 - FIFO_TAIL
fifo_pop:
	la t0, FIFO_HEAD
	lw t1, [t0]

	la t2, FIFO_TAIL
	lw t3, [t2]

	beq t1, t3, fifo_pop_fail ; if head == tail fail
	; else return item
	li t2, FIFO_SIZE ; overwrite tail since no longer needed
	li t3, fifo_base
	add t2, t2, t3
	lb a0, [t1]       ; load contents of buffer
	addi t1, t1, 1    ; increment head and rollover
	bne t2, t1, %F1 ; if head == last then wrap 
	mv t1, t3

	1
	sw t1, [t0]
	ret

	fifo_pop_fail:
	mv a0, zero
	ret

; a0 - index of key
; output a0 - byte it correponds to
translate_matrix_code:
	la t0, keyboard_lookup_table
	add t0, t0, a0
	lb a0, [t0]
	ret

keybord_scan_irsq:
	addi sp, sp, -4
	sw ra, [sp]
	call scan_matrix
	lw ra, [sp]
	addi sp, sp, 4
	ret

FIFO_SIZE EQU 32
FIFO_HEAD DEFW fifo_base
FIFO_TAIL DEFW fifo_base
fifo_base
DEFS 32

KEY_HISTORY DEFS 16

DEBOUNCED_STATE DEFB 0x0
keyboard_lookup_table:
    DEFB '*','7','4','1'
    DEFB '0','8','5','2'
    DEFB '#','9','6','3'
    DEFB 'C','=','-','+'

align

