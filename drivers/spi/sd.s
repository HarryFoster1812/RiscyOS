; https://onlinedocs.microchip.com/oxy/GUID-F9FE1ABC-D4DD-4988-87CE-2AFD74DEA334-en-US-3/GUID-48879CB2-9C60-4279-8B98-E17C499B12AF.html
; http://rjhcoding.com/avrc-sd-interface-1.php

#define SD_CS_ENABLE() \
    li a0, 0 __NL__         \
    call spi_set_cs
#define SD_CS_DISABLE() \
    li a0, 1 __NL__         \
    call spi_set_cs

#define SPI_TRANSFER(x) \
    li a0, x __NL__ \
    call spi_send_byte


CMD0			EQU 0
CMD0_ARG	EQU 0
CMD0_CRC	EQU 0x94

CMD8			EQU 8
CMD8_ARG	EQU 0x1AA
CMD8_CRC	EQU 0x86 //(1000011 << 1)

CMD58     EQU 58
CMD58_ARG EQU  0x00000000
CMD58_CRC EQU 0x00

CMD55      EQU 55
CMD55_ARG  EQU 0x00000000
CMD55_CRC  EQU 0x00

ACMD41     EQU 41
ACMD41_ARG EQU 0x40000000
ACMD41_CRC EQU 0x00

CMD17                EQU  17
CMD17_CRC            EQU 0x00
SD_MAX_READ_ATTEMPTS EQU 1563

IRQ_STATUS_BYTE_BIT EQU 5
IRQ_STATUS_BLOCK_BIT EQU 6
IRQ_STATUS_ERROR_BIT EQU 7

SD_SUCCESSFUL_READ EQU 0xFE
SD_DATA_ERROR EQU 0x00 ; 0x0X (the higher nibble of the byte is 0 then there is an error)
SD_TIMEOUT EQU 0xFF

SD_INFO DEFS  SD_INFO_T_SIZE

SD_STATE_IDLE EQU 0
SD_STATE_WAIT_READ EQU 1
SD_STATE_WAIT_WRITE EQU 2
SD_STATE_ERROR EQU 3

; SD_INFO
STRUCT
SD_TYPE BYTE
SD_BLOCK_ADDRESSSING BYTE
SD_STATE BYTE
SD_INFO_T_SIZE ALIAS

; SD_RES7
SD_RES7_T_SIZE EQU 5 +3 ; 3 bytes of padding

ALIGN 4
sd_init:
	addi sp, sp, -(4+	SD_RES7_T_SIZE)

	sw ra, SD_RES7_T_SIZE[sp]
	
	call send_dummy_clocks
	; send command CMD0

	; enable sd
  SPI_TRANSFER(0xFF)
	SD_CS_ENABLE()
  SPI_TRANSFER(0xFF)
	
	li a0, CMD0
	li a1, CMD0_ARG
	li a2, CMD0_CRC
	call sd_send_command_crc
	call sd_readRes1

	li t0, 1
	bne a0, t0, %F1


	li a0, CMD8
	li a1, CMD8_ARG
	li a2, CMD8_CRC
	call sd_send_command_crc
	mv a0, sp ; fill the res 7 struct
	call sd_readRes3_7

	
	mv s0, a0
  j %F2

	1
  la a0, sd_failure_str
  call k_dbg_print
  2
	; disable sd
	SD_CS_DISABLE()


	lw ra, SD_RES7_T_SIZE[sp]
	addi sp, sp, (4+	SD_RES7_T_SIZE)
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
	li s2, 100
	; while(spi_send_byte(0xFF)) == 0xFF && (attemps++<10));
	1
	call spi_send_byte
	addi s1, s1, 1
	sltu t1, s1, s2
  subi t2, a0, 0xFF
	seqz t2, t2
  and t1, t1, t2
  bnez t1, %B1

	lw s0, [sp]
	lw s1, 4[sp]
	lw s2, 8[sp]
	lw ra, 12[sp]
	addi sp, sp, 16
	ret


; sd_readRes3_7(uint8_t *res)
; both res3 and res7 are 5 bytes long
sd_readRes3_7:
	addi sp, sp, -8
	sw s0, [sp]
	sw ra, 4[sp]

	mv s0, a0


	call sd_readRes1
	; if(res1 != 1) return
	li t0, 1
	bne a0, t0, %F1
	sb a0, [s0]

	; if it is 1 then we should be able to read the next 4 bytes
	li a0, 0xFF
	call spi_send_byte
	sb a0, 1[s0]


	li a0, 0xFF
	call spi_send_byte
	sb a0, 2[s0]

	li a0, 0xFF
	call spi_send_byte
	sb a0, 3[s0]

	li a0, 0xFF
	call spi_send_byte
	sb a0, 4[s0]

	1
	lw s0, [sp]
	lw ra, 4[sp]
	addi sp, sp, 8
	ret

send_dummy_clocks:
	addi sp, sp, -4
	sw ra, [sp]
	; set up SPI for block transfer of 10 bytes
	li a0, 12
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

  SD_CS_DISABLE()

  la a0, 10000
  call DELAY

	call spi_send_block

	li t1, (1<<IRQ_STATUS_BLOCK_BIT) ; block done status bit

	; poll until block done (IRQ)
	1
	lw t2, SPI_STATUS[t0]
	and t3, t2, t1
	beqz t3, %B1

	; block has finished
	sw zero, SPI_STATUS[t0] ; clear IRQ

  SD_CS_DISABLE()
  SPI_TRANSFER(0xFF)

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
srli a0, a1, 24
call spi_send_byte

srli a0, a1, 16
call spi_send_byte

srli a0, a1, 8
call spi_send_byte

andi a0, a1, 0xFF
srl a0, a1, t1
call spi_send_byte

ori a0, a2, 1
call spi_send_byte

	lw ra, [sp]
	addi sp, sp, 4
ret

; return crc7
calcualte_CRC7:
ret


; sd_start_read_single_block(uint32_t addr, uint8_t *token)
sd_start_read_single_block: 
	addi sp, sp, -20
	sw s0, [sp]
	sw s1, 4[sp]
	sw s2, 8[sp]
	sw s2, 12[sp]
	sw ra, 16[sp]

	mv s0, a0
	mv s1, a1

	; *token = 0xFF
	li t0, 0xFF
	sb t0, [s1]

	// assert chip select
	SD_CS_ENABLE()

	// send CMD17
	li a0, CMD17
	mv a1, s0
	li a2, CMD17_CRC
	call sd_send_command_crc

	// read R1
	call sd_readRes1
	mv s2, a0 ; save res 1 (this is the return value)

    // if response received from card
	li t0, 0xFF
	beq a0, t0, sd_invalid_read_response
        // wait for a response token (timeout = 100ms)
        ; readAttempts = 0;

		; wait for start byte of read frame (or timeout)
		li t4, SD_MAX_READ_ATTEMPTS
		mv t5, zero
		li t6, 0xFF
		1
		mv a0, t6
		call spi_send_byte
		; if spi_send_byte(FF) != FF break
		bne a0, t6, %F1
		
		addi t5, t5, 1 ; attempts++;
		; while attempts < SD_MAX_READ_ATTEMPTS
		blt t5, t4, %B1

		1

		;/ set token to card response
		;*token = read;
		sb a0, [s1] 
		li t0, SD_SUCCESSFUL_READ

		bne a0, t0, %F2  
		li a0, 512
		call spi_set_block_len
		call spi_send_block_blocking

	2
	mv a0, s2 ; return res 1
	sd_invalid_read_response:
	SD_CS_DISABLE()

	lw s0, [sp]
	lw s1, 4[sp]
	lw s2, 8[sp]
	lw s2, 12[sp]
	lw ra, 16[sp]
	addi sp, sp, 20
	ret

  sd_failure_str DEFB "Failed to init sd\0"
  ALIGN 4
