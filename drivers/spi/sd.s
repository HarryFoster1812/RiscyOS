; https://onlinedocs.microchip.com/oxy/GUID-F9FE1ABC-D4DD-4988-87CE-2AFD74DEA334-en-US-3/GUID-48879CB2-9C60-4279-8B98-E17C499B12AF.html
; http://rjhcoding.com/avrc-sd-interface-1.php
CMD0 EQU 0
CMD0_ARG EQU 0
CMD0_CRC EQU 0x94

IRQ_STATUS_BYTE_BIT EQU 5
IRQ_STATUS_BLOCK_BIT EQU 6
IRQ_STATUS_ERROR_BIT EQU 7

init_sd:
	addi sp, sp, -4
	sw ra, [sp]
	
	call send_dummy_clocks
	; send command CMD0

	; enable sd
	la a0, 0
	call spi_set_cs
	
	li a0, CMD0
	li a1, CMD0_ARG
	li a2, CMD0_CRC
	call sd_send_command_crc
	call sd_readRes1
	
	mv s0, a0

	; disable sd
	la a0, 1
	call spi_set_cs


	lw ra, [sp]
	addi sp, sp, 4
	ret

sd_readRes1:
	addi sp, sp, -16
	sw s0, [sp]
	sw s1, 4[sp]
	sw s2, 8[sp]
	sw ra, 12[sp]

	li a0, 0xFF
	mv s0, a0
	mv s1, zero
	li s2, 10
	; while(spi_send_byte(0xFF)) == 0xFF && (attemps++<10));
	1
	call spi_send_byte
	beq a0, s0, %B1
	addi s1, s1, 1
	blt s1, s2, %B1

	lw s0, [sp]
	lw s1, 4[sp]
	lw s2, 8[sp]
	lw ra, 12[sp]
	addi sp, sp, 16
	ret

send_dummy_clocks:
	addi sp, sp, -4
	sw ra, [sp]
	; set up SPI for block transfer of 10 bytes
	li a0, 10
	call spi_set_block_len
	; load 10 FF bytes into TX_RAM
	li t0, SPI_BASE
	addi t1, zero, -1
	sw t1, SPI_TX_RAM[t0]			; 4 bytes
	sw t1, (SPI_TX_RAM+4)[t0] ; 8 bytes
	sw t1, (SPI_TX_RAM+8)[t0] ; 12 bytes
	
	; set clock divisor to be in the slow range
	li a0, 80 ; 250 Khz 
	call spi_set_clock_div

	; set cs to high
	li a0, 1
	call spi_set_cs
	call spi_send_block

	li t1, (1<<IRQ_STATUS_BLOCK_BIT) ; block done status bit

	; poll until block done (IRQ)
	1
	lw t2, SPI_STATUS[t0]
	and t3, t2, t1
	beqz t3, %B1

	; block has finished
	sw zero, SPI_STATUS[t0] ; clear IRQ

	lw ra, [sp]
	addi sp, sp, 4
	ret

; a0 - command
; a1 - args
sd_send_command:
; call calcualte_CRC7
; call sd_send_command_crc
ret

sd_send_command_crc:
	addi sp, sp, -4
	sw ra, [sp]

; all sd commands are in the format:
; 47		- 0 - start bit
; 46		- 1 - transmission bit
; 44:40 -	x	- command code
; 39:8	- x - argument
; 7:1		- 0 - crc7
; 0			- 1 - end bit

; bitwise or with 0x40 since 2 most significant bits of the command always set to 0b01
li t0, 0x40
or a0, a0, t0
call spi_send_byte

; send msb of arg
li t1, 24
srl a0, a1, t1
call spi_send_byte

addi t1, t1, -8
srl a0, a1, t1
call spi_send_byte

addi t1, t1, -8
srl a0, a1, t1
call spi_send_byte

addi t1, t1, -8
srl a0, a1, t1
call spi_send_byte

li t1, 1
or a0, a3, t1
call spi_send_byte

	lw ra, [sp]
	addi sp, sp, 4
ret

; return crc7
calcualte_CRC7:
ret
