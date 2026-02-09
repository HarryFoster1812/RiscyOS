;-----------------------------------------------------
; 
; H. Foster
; Version 1.0
; 6th Feburary 2025
;
; This is the boot code for RiscyOS
;
; Last modified: 6/2/26 (HWF)
;
; Known bugs: None
;
;-----------------------------------------------------
J boot

INCLUDE "global_vars.s"

boot align
    la sp, kernel_stack
    addi sp, sp, STACK_SIZE
	li t0, 0x0000_1800 ; Load MPP mask - bits 12 & 11
	csrc MSTATUS, t0 ; Clear MPP bits in status
	la t0, mhandler ; Point at trap handler code start
	csrw MTVEC, t0 ; Save address in system CSR
	csrw MSCRATCH, sp ; Copy machine SP for use in handler
	la sp, user_stack ; Change SP to user space
    addi sp, sp, STACK_SIZE
	la ra, user_main ; Point at user code start
	csrw MEPC, ra ; 'Save' as return address
	mret ; 'Return' to programme start


; 100*(n)-75


INCLUDE "LCD_Driver.s"


kernel_stack DEFS 400

org 0x4_0000

; Start of user ram


my_str DEFB "Hello, World!\0"

user_main align
	; ------- set up
	lw SP, STACK_START ; set sp to user stack (custom defined)
	main_loop
		la a0, my_str ; load pointer to string
		jal PrintString 
		
		lw a0, ONE_S_DELAY
		jal DELAY

		li a0, LCD_CLEAR_INST ; load Clear
		jal sendLCDInstruction 

	j main_loop

; a0 is pointer to null terminated string
; a0 returns len of string
strlen
	mv t0, a0 
	mv a0, zero

	strlen_loop
	lb t1, [t0] ; load character
	beqz t1, strlen_exit ; compare with null char
	addi a0, a0, 1 
	addi t0, t0, 1
	j strlen_loop

	strlen_exit
	ret



PrintString
	subi sp, sp, 4
	sw ra, [sp]
	mv t3, a0
	jal strlen
	mv t4, a0 ; t4 is strlen 

	string_print_loop
		beqz t4, string_print_loop_exit
		; there is still a charater to print
		lb a0, [t3]
		jal PrintChar
		addi t3, t3, 1
		subi t4, t4, 1
		j string_print_loop

	string_print_loop_exit

	lw ra, [sp]
	addi sp, sp, 4
	ret



user_stack DEFS 400

