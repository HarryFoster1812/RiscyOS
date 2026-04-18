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

#define SD_SET_STATE(x) \
    la t0, SD_INFO __NL__ \
    li t1, x __NL__ \
    sb t1, SD_STATE[t0] __NL__ \

CMD0			EQU 0
CMD0_ARG	EQU 0
CMD0_CRC	EQU 0x94

CMD8			EQU 8
CMD8_ARG	EQU 0x1AA
CMD8_CRC	EQU 0x86 //(1000011 << 1)

; NOTE: CRC is only required for CMD0 and CMD8 all other commands accept dummy CRC
; it could be useful for debugging but calculating crc16 for 512 r/w is quite expensive

CMD58     EQU 58
CMD58_ARG EQU  0x00000000
CMD58_CRC EQU 0x00
CCS_BIT   EQU 3 ; used to determine the block mode

CMD55      EQU 55
CMD55_ARG  EQU 0x00000000
CMD55_CRC  EQU 0x00

ACMD41     EQU 41
ACMD41_ARG EQU 0x40000000
ACMD41_CRC EQU 0x00

CMD17                EQU  17
CMD17_CRC            EQU 0x00
SD_MAX_READ_ATTEMPTS EQU 250000 ; (0.1s *40000000 Mhz)/(2(clock div) * 8 byte)

CMD24                EQU 24
CMD24_CRC            EQU 0x00
SD_MAX_WRITE_ATTEMPTS EQU 625000
SD_START_TOKEN      EQU 0xFE

IRQ_STATUS_BYTE_BIT EQU 5
IRQ_STATUS_BLOCK_BIT EQU 6
IRQ_STATUS_ERROR_BIT EQU 7

SD_SUCCESSFUL_READ EQU 0xFE
SD_DATA_ERROR EQU 0x00 ; 0x0X (the higher nibble of the byte is 0 then there is an error)
SD_DATA_ACCEPTED EQU 0x05
SD_TIMEOUT EQU 0xFF

; SD_INFO
STRUCT
SD_BLOCK_ADDRESSSING BYTE ; bool 1 = use 512 byte addressing 0 = use byte addressing
SD_STATE BYTE             ; READING, WRITING IDK yet
STRUCT_ALIGN 4 
SD_INFO_T_SIZE ALIAS

SD_INFO DEFS  SD_INFO_T_SIZE

SD_STATE_IDLE EQU 0
SD_STATE_WAIT_READ EQU 1
SD_STATE_WAIT_WRITE EQU 2
SD_STATE_ERROR EQU 3

SD_ERR_OK            EQU 0
SD_ERR_TIMEOUT       EQU 1
SD_ERR_BAD_RESP      EQU 2
SD_ERR_UNSUPPORTED   EQU 3
SD_ERR_INIT_FAILED   EQU 4

; SD_RES7
SD_RES7_T_SIZE EQU 5 + 3 ; 3 bytes of padding

ALIGN 4

; bool sd_init()
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
	bne a0, t0, sd_init_failure


	li a0, CMD8
	li a1, CMD8_ARG
	li a2, CMD8_CRC
	call sd_send_command_crc
	mv a0, sp ; fill the res 7 struct
	call sd_readRes3_7

	
  ; res buffer is at sp
  lb t0, 0[sp]      ; R1
  li t1, 1
  bne t0, t1, sd_init_failure

  ; check echo pattern (last 2 bytes should be 0x01AA)
  lbu t2, 3[sp]
  lbu t3, 4[sp]
  li t4, 0x01
  bne t2, t4, sd_init_failure
  li t4, 0xAA
  bne t3, t4, sd_init_failure

  sd_acmd_loop:
      ; CMD55

      li a0, CMD55
      li a1, CMD55_ARG
      li a2, CMD55_CRC
      call sd_send_command_crc
      call sd_readRes1

    li a0, ACMD41
    li a1, ACMD41_ARG
    li a2, ACMD41_CRC
    call sd_send_command_crc
    call sd_readRes1

    bnez a0, sd_acmd_loop


    li a0, CMD58
    li a1, CMD58_ARG
    li a2, CMD58_CRC
    call sd_send_command_crc

    mv a0, sp
    call sd_readRes3_7

    ; parse this command
    lbu t0, 1[sp] ; read ms byte of OCR 
    srli t0, t0, CCS_BIT
    andi t0, t0, 1 ; mask lsb
    li t1, SD_INFO
    sb t0, SD_BLOCK_ADDRESSSING[t1]

    SD_CS_DISABLE()
    SPI_TRANSFER(0xFF)


    ; if we are here then everything is good and we can increase sclk speed
    li a0, 2 ; 20 MHz
    call spi_set_clock_div

  li a0, 1 ; init succesful

	lw ra, SD_RES7_T_SIZE[sp]
	addi sp, sp, (4+	SD_RES7_T_SIZE)
	ret

; FAILURE
sd_init_failure:
	SD_CS_DISABLE()
  mv a0, zero ; return false
	lw ra, SD_RES7_T_SIZE[sp]
	addi sp, sp, (4+	SD_RES7_T_SIZE)
  ret

; returns:
; a0 = response OR 0xFF if timeout
; a1 = 0 (OK) or SD_ERR_TIMEOUT
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
call spi_send_byte

ori a0, a2, 1
call spi_send_byte

	lw ra, [sp]
	addi sp, sp, 4
ret

; return crc7
calcualte_CRC7:
ret


; sd_start_read(uint32_t addr)
; NOTE: addr should be pre-configured (byte addresses or block addressed)
sd_start_read: 
	addi sp, sp, -8
	sw s0, [sp]
	sw ra, 4[sp]

	mv s0, a0

	// assert chip select
	SD_CS_ENABLE()

	// send CMD17
	li a0, CMD17
	mv a1, s0
	li a2, CMD17_CRC
	call sd_send_command_crc

	// read R1
	call sd_readRes1
	mv s0, a0 ; save res 1 (this is the return value)

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

    SD_SET_STATE(SD_STATE_WAIT_READ)
		li t0, SD_SUCCESSFUL_READ

		bne a0, t0, %F2  
		li a0, 512
		call spi_set_block_len
		call spi_send_block
    j %F3
	2
	sd_invalid_read_response:
	SD_CS_DISABLE()
  3
	mv a0, s0 ; return res 1
	lw s0, [sp]
	lw ra, 4[sp]
	addi sp, sp, 8
	ret

sd_tail_read:
	addi sp, sp, -4
	sw ra, [sp]
  ; this is the irq when the read block is finished
  ; the last two bytes are the crc (ignored)
  SPI_TRANSFER(0xFF)
  SPI_TRANSFER(0xFF)


  SPI_TRANSFER(0xFF)
	SD_CS_DISABLE()
  SPI_TRANSFER(0xFF)

  SD_SET_STATE(SD_STATE_IDLE)

	lw ra, [sp]
	addi sp, sp, 4
  ret


; sd_start_write(uint32_t addr)
; NOTE: addr should be pre-configured (byte addresses or block addressed)
sd_start_write: 
	addi sp, sp, -8
	sw s0, [sp]
	sw ra, 8[sp]

	mv s0, a0

	// assert chip select
  SPI_TRANSFER(0xFF)
	SD_CS_ENABLE()
  SPI_TRANSFER(0xFF)

	// send CMD17
	li a0, CMD24
	mv a1, s0
	li a2, CMD24_CRC
	call sd_send_command_crc

	// read R1
	call sd_readRes1
	mv s0, a0 ; save res 1 (this is the return value)

    // if response received from card
	li t0, 0xFF
	beq a0, t0, sd_invalid_write_response

  SD_SET_STATE(SD_STATE_WAIT_WRITE)

    li a0, SD_START_TOKEN
    call spi_send_byte

		li a0, 512
		call spi_set_block_len
		call spi_send_block

	2
	mv a0, s0 ; return res 1
	sd_invalid_write_response:
  SPI_TRANSFER(0xFF)
	SD_CS_DISABLE()
  SPI_TRANSFER(0xFF)

	lw s0, [sp]
	lw ra, 4[sp]
	addi sp, sp, 8
	ret

; returns write response from sd:
; 0x00 - busy timeout
; 0x05 - data accepted
; 0xFF - response timeout
sd_tail_write:
	addi sp, sp, -4
	sw ra, [sp]
  ; this is the irq when the read block is finished

   ;while(++readAttempts != SD_MAX_WRITE_ATTEMPTS)
  ;   if((read = SPI_transfer(0xFF)) != 0xFF) { *token = 0xFF; break; }

		li t4, SD_MAX_WRITE_ATTEMPTS
		mv t5, zero

    sd_write_response_loop:
    SPI_TRANSFER(0xFF)
		; if spi_send_byte(FF) != FF break
		bne a0, t6, %F1
		
		addi t5, t5, 1 ; attempts++;
		; while attempts < SD_MAX_READ_ATTEMPTS
		bne t5, t4, sd_write_response_loop

    1
    ; check if data accepted
    andi t0, a0, 0x1F ; a write response is xxx0<status>1
    li t1, SD_DATA_ACCEPTED
    bne t0, t1, sd_write_error

		mv t5, zero

    sd_write_busy_loop:
    SPI_TRANSFER(0xFF)
		; if spi_send_byte(FF) != FF break
		bne a0, t6, %F1
		
		addi t5, t5, 1 ; attempts++;
		; while attempts < SD_MAX_READ_ATTEMPTS
		bne t5, t4, sd_write_busy_loop
    ; time out waiting for busy
    mv a0, zero
    j %F3

    2
    li a0, 0x5
    j %F3

  sd_write_error:
  li a0, 0xFF

  3
  SPI_TRANSFER(0xFF)
	SD_CS_DISABLE()
  SPI_TRANSFER(0xFF)
	
  SD_SET_STATE(SD_STATE_IDLE)

  lw ra, [sp]
	addi sp, sp, 4
  ret

