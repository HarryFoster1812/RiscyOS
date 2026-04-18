; NOTE: This is FAT32 Only

#include <mbr.inc>

fat_init:
	addi sp, sp, -4  
	sw ra, [sp]

	; read the bpb
	li a0, 0
  ; write status
  call sd_read_block_blocking

  ; data should now be in the RX 512 byte buffer
  ; parse master boot record 
  li t0, SPI_BASE + SPI_RX_RAM
  
  ; check mbr signature integrity
  lhu t0, BOOT_SIGNATURE[t0]
  li t1, 0xaa55
  bne t0, t1, mbr_invalid
  addi t1, t0, PARTITION_ENTRY_1
  ; check partition is valid
  lbu t2, PARTITION_STATUS[t1]
  li t3, 0x80
  bne t2, t4, mbr_invalid

  ; Check the partition type is
  ; FAT is 0xB or 0xC
  lbu t2, PARTITION_TYPE[t1]
  subi t2, t2, 0xB 
  li t3, 1
  bgtu t2, t1, mbr_invalid
  
  ; load the logical block of the parition
  lw a0, PARTITION_LBA_FIRST[t1]

  ; read the FAT bpb
  call sd_read_block_blocking
  ; the bpb should be in the RX buffer
  li t0, SPI_BASE + SPI_RX_RAM



  mbr_invalid:

	lw ra, [sp]
	addi sp, sp, 4
  ret

sd_read_block_blocking:
  addi sp, sp, -4
  sw ra, [sp]
	call sd_start_read
  ; TODO: Parse a0 (res1 for failure)
  
  li t2, (1<<IRQ_STATUS_BLOCK_BIT)
	li t0, SPI_BASE
  call  spi_poll_interrupt

  call sd_tail_read
  lw ra, [sp]
  addi sp, sp, 4
  ret

fat_node_get:

cluster_to_sector:

next_cluster:

dir_lookup:

fat_read:

fat_write:

fat_mkdir:


sd_irsq_handler:
