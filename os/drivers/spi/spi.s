SPI_BASE EQU 0x2_0000

SPI_RAM_SIZE_BYTES EQU 512

STRUCT
SPI_CONTROL			WORD
SPI_STATUS			WORD
SPI_CONFIG			WORD
SPI_CS					WORD
SPI_TXDATA			WORD
SPI_RXDATA			WORD
SPI_BLOCK_LEN		WORD
SPI_IRQ_ENABLE	WORD
RECORD					0x200
SPI_TX_RAM					ALIAS
RECORD          SPI_TX_RAM+SPI_RAM_SIZE_BYTES ; 0x200 + 0x800 (2048)
SPI_RX_RAM					ALIAS

spi_init:
	addi sp, sp, -4
	sw ra, [sp]
	; make sure tx is setup
	li t0, SPI_BASE
	
	; initialise TX with dummy byte FF
	addi t1, zero, -1
	sw t1, SPI_TXDATA[t0]

	; enable interrupts
	sw t1, SPI_IRQ_ENABLE[t0]

	; set cs high
	li t1, 1
	sw t1, SPI_CS[t0]

	; set mode to be SPI mode 0  (cpol=0 and cpha=0)
	mv a0, zero
	call spi_set_mode
	
	lw ra, [sp]
	addi sp, sp, 4
	ret

; blocking (the amount of time to retrun get the interrupt and then send another one is just not worth being non-blocking)
	; a0 byte to send
spi_send_byte:
	li t0, SPI_BASE
	sw a0, SPI_TXDATA[t0]
	li t1, 0b101				; [block mode][stop][start] 
	sw t1, SPI_CONTROL[t0]
	li t2, (1<<1) ; rx_valid bit

	; poll status until rx vaild
	1
	lw t3, SPI_STATUS[t0]
	and t3, t3, t2 
	beqz t3, %B1
	; rx valid high
	lw a0, SPI_RXDATA[t0]
	ret 

spi_send_block:
	li t0, SPI_BASE
	li t1, 0b101				; [block mode][stop][start] 
	sw t1, SPI_CONTROL[t0]
	ret

spi_set_block_len:
	li t0, SPI_BASE
	sw a0, SPI_BLOCK_LEN[t0]
	ret

; a0 - 0 low
; a0 - 1 high
spi_set_cs:
	li t0, SPI_BASE
	sw t1, SPI_CS[t0]
	ret

; CHPOL - bit 0 CHPA - bit 1
; mode 0 CPOL - 0 CPHA - 0
; mode 1 CPOL - 0 CPHA - 1
; mode 2 CPOL - 1 CPHA - 0
; mode 3 CPOL - 1 CPHA - 1
spi_set_mode:
	li t1, 3
	bgt a0, t1, set_spi_mode_exit
	li t0, SPI_BASE 
	; read current config
	lw t2, SPI_CONFIG[t0]
	; clear lower two bits
	not t3, t1
	and t2, t2, t3
	; set lower two bits to a0
	or t2, t2, a0
	; rewrite reg
	sw t2, SPI_CONFIG[t0]

set_spi_mode_exit:
	ret

; a0 - byte (anything after lsb is ignored)
spi_set_clock_div:
	li t0, SPI_BASE 
	; read current config
	lw t2, SPI_CONFIG[t0]
	; clear lower two bits
	li t1, 0xFF00
	not t3, t1
	and t2, t2, t3
	; set clock div
	li t1, 8
	sll t4, a0, t1
	or t2, t2, t4
	; rewrite reg
	sw t2, SPI_CONFIG[t0]
	ret
