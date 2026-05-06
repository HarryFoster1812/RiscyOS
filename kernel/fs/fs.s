; NOTE: This is FAT32 Only

#include <mbr.inc>
#include <partition.inc>
#include <bpb.inc>
#include <fs_running_info.inc>

FS_RUN_INFO DEFS FS_RUN_INFO_STRUCT_SIZE

; FAT_LFN_PARSER
SRUCT 
lfn_buffer BYTE 200
lfn_length WORD
lfn_active BYTE
short_name BYTE, 11

fat_init:
	addi sp, sp, -8  
	sw ra, 4[sp]

	; read the bpb
	li a0, 0
  ; write status
  call sd_read_block_blocking

  ; data should now be in the RX 512 byte buffer
  ; parse master boot record 
  li t0, SPI_BASE + SPI_RX_RAM
  
  ; check mbr signature integrity
  lhu t1, BOOT_SIGNATURE[t0]
  li t2, MBR_SIGNATURE
  bne t1, t2, mbr_invalid
  addi t1, t0, PARTITION_ENTRY_1
  ; check partition is valid
  lbu t2, PARTITION_STATUS[t1]
  li t3, PARTITION_STATUS_ACTIVE
  bne t2, t3, mbr_invalid

  ; Check the partition type is
  ; FAT is 0xB or 0xC
  lbu t2, PARTITION_TYPE[t1]
  subi t2, t2, PARTITION_TYPE_FAT
 li t3, 1
  bgtu t2, t3, mbr_invalid
  
  ; load the logical block of the parition
  lhu a0, PARTITION_LBA_FIRST[t1] ; need to load half because it is not aligned because it is a 16 bit system
  lhu t0, (PARTITION_LBA_FIRST+2)[t1]
  slli t0, t0, 16 ; shift up
  add a0, a0, t0
  sw a0, [sp] ; save the partion_begin

  ; read the FAT bpb
  call sd_read_block_blocking
  ; the bpb should be in the RX buffer
  li t0, SPI_BASE + SPI_RX_RAM

  ; check BS_Sign 
  li t1, FAT_BPB_SIGNATURE
  lhu t2, BS_Sign[t0]
  bne t1, t2, bpb_invalid
  
  ; make sure bytes per sector is 512 (if it is not then fail)
  li t1, 512
  lbu t2, BPB_BytsPerSec[t0]
  lbu t3, (BPB_BytsPerSec+1)[t0]
  slli t3, t3, 8 ; shift up
  add t2, t2, t3
  bne t1, t2, bpb_invalid

  ; fill out run time struct
  ; t0 - pointer tp bpb
  ; t1 - pointer to run-time struct
  ; t2 - partiton_begin LBA
  ; t3 - number of reserved sectors
  ; t4 - number of fats
  ; t5 - number of sectors per fat
  ; t6 - root cluster 
  la t1, FS_RUN_INFO
  lw t2, [sp]

  lhu t3, BPB_RsvdSecCnt[t0]
  lbu t4, BPB_NumFATs[t0]

	; these should be aligned
  lw t5, BPB_FATSz32[t0]
  lw t6, BPB_RootClus[t0]
  
  lbu t0, BPB_SecPerClus[t0]
  sb t0, SECTORS_PER_CLUSTER[t1]

  ; calculate fat begin
  add t0, t2, t3
  sw t0, FAT_BEGIN_LBA[t1]

  ; calculate cluster begin
  mul t4, t4, t5
  add t0, t0, t4
  sw t0, CLUSTER_BEGIN_LBA[t1]
  
  sw t5, FAT_SECTOR_COUNT[t1]
  sw t6, ROOT_DIR_FIRST_CLUSTER[t1]

  mbr_invalid:
  bpb_invalid:

	lw ra, 4[sp]
	addi sp, sp, 8
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

; a0 cluster number
; LBA = CLUSTER_BEGIN_LBA + (cluster - 2) * SECTORS_PER_CLUSTER
cluster_to_lba:
  la t0, FS_RUN_INFO
  lw t1, CLUSTER_BEGIN_LBA[t0]
  lbu t2, SECTORS_PER_CLUSTER[t0]
  addi a0, a0, -2 
  mul a0, a0, t2
  add a0, a0, t1
  ret

; a0 - current directory
dir_lookup:

dir_parse:


; this will read the fat table into memory
; update the next cluster info as well as if it is the last cluster 
; void fat_read(cluster_num) 
fat_start_read:
  la t0, FS_RUN_INFO
	lw t1, FAT_BEGIN_LBA[t0]
	lw t2, FAT_SECTOR_COUNT[t0]
	li t3, 512
	div a0, a0, t3 ; cluster num / 512 (gets the sector which the cluster fat entry will be in)
	add a0, a0, t1
	bgt a0, t2 fat_read_fail

	; start read operation
	tail sd_start_read

	fat_read_fail:
	ret

; NOTE: This is the second half of the fat_read operation where fat_read should of been previously called
; and so the fat table should be in the sd buffer
; a0 - current cluster
get_next_cluster:
  addi sp, sp, -8
	sw a0, [sp]
  sw ra, 4[sp]

	call sd_tail_read
	li t0, SPI_BASE+SPI_RX_RAM
	li t1, 512
	lw t2, [sp]

	; calcualte the index within the sector of the cluster number
	rem t2, t2, t1
	add t1, t0, t2

	lw a0, [t1]
  
	lw ra, [sp]
  addi sp, sp, 8
  ret


; this will do something idk bro
fat_write:

; a0 - current directory?
fat_mkdir:

; ok this should be the file task io state machine
; read the current task find out what stage
; go to the corresponding function to handle that stage
sd_irsq_handler:
	; get head of SD_IO_QUEUE
