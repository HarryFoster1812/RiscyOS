fat_init:
	addi sp, sp, -8  
	sw ra, 4[sp]

	; read the bpb
	li a0, 0
	mv a1, sp
	call sd_start_read_single_block
  
  la ra, %F1 
  li t2, li t2, (1<<IRQ_STATUS_BLOCK_BIT)
	li t0, SPI_BASE
  j  spi_poll_interrupt
  1
  call sd_tail_read

  ; data should now be in the RX 512 byte buffer
  ; parse bios 

  ret

fat_node_get:

cluster_to_sector:

next_cluster:

dir_lookup:

fat_read:

fat_write:

fat_mkdir:
