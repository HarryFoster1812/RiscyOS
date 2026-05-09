; NOTE: This is FAT32 Only

#include <mbr.inc>
#include <partition.inc>
#include <bpb.inc>
#include <fs_running_info.inc>
#include <io/sd_io_request.inc>
#include <io/sd_io_request_elf.inc>
#include <io/sd_io_request_open.inc>

fs_running_info DEFS FS_RUN_INFO_STRUCT_SIZE
ALIGN

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
  la t1, fs_running_info
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

	; if everything was successful then we can create our load init task
	li a0, SD_IO_REQ_ELF
	call make_io_request
	; i know i shouldnt but i am going to assume that kmalloc succeds on the first call...
	sb zero, IO_REQ_PROC_ID[a0] ; set task proc id to 0 (kernel)
  sw zero, IO_REQ_NEXT[a0]
  sw zero, SD_IO_REQ_STATE_MACHINE[a0]
  la t0, sd_request_queue
  sw a0, [t0]
  call sd_irsq_handler

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
  la t0, fs_running_info
  lw t1, CLUSTER_BEGIN_LBA[t0]
  lbu t2, SECTORS_PER_CLUSTER[t0]
  addi a0, a0, -2 
  mul a0, a0, t2
  add a0, a0, t1
  ret

; this is here because i want to save on space and not store both a cluster and sector
lba_to_cluster:
  la t0, fs_running_info
  lw t1, CLUSTER_BEGIN_LBA[t0]
  lbu t2, SECTORS_PER_CLUSTER[t0]

	sub a0, a0, t1
  div a0, a0, t2
  addi a0, a0, 2 
  ret

lba_to_sector:
  addi sp, sp, -4
  sw ra, [sp]

  mv t3, a0  ; t3 is not used by either functions
  call lba_to_cluster

  ; a0 has a cluster num
  call cluster_to_lba
  ; a0 is now the lba of the start of the cluster 
  sub a0, s0, a0 ; this will give us the lba difference which is the sector number

  lw ra, [sp]
  addi sp, sp, 4
  ret

; a0 - current directory
dir_lookup:

dir_parse:


; this will read the fat table into memory
; update the next cluster info as well as if it is the last cluster 
; void fat_read(cluster_num) 
fat_calcualte_lba:
  la t0, fs_running_info
	lw t1, FAT_BEGIN_LBA[t0]
	lw t2, FAT_SECTOR_COUNT[t0]
	li t3, 512
	div a0, a0, t3 ; cluster num / 512 (gets the sector which the cluster fat entry will be in)
	add a0, a0, t1
	bgt a0, t2, fat_read_fail

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

// a0 - open_file_req*
set_initial_dir:
  addi sp, sp, -8
  sw s0, [sp]
  sw ra, 4[sp]
  mv s0, a0
  // read the path
  lw t0, ELF_FILE_NAME[a0]
  lb t0, [t0] ; read first character
  li t1, '/'
  beq  t0, t1, set_inital_dir_root
  // check if the pid is 0 (kernel)
  lh a0, IO_REQ_PROC_ID[a0]
  beqz a0, set_inital_dir_root
  
  call get_pcb_from_id
  ; a0 has pcb pointer
  lw a0, PCB_PARENT_DIR_CLUSTER[a0]

  j get_inital_dir_exit
  
set_inital_dir_root:
  la t0, fs_running_info
	lw a0, ROOT_DIR_FIRST_CLUSTER[t0]

get_inital_dir_exit:
  call cluster_to_lba
  sw a0, SD_IO_REQ_LBA[s0]

  mv a0, s0
  lw s0, [sp]
  lw ra, 4[sp]
  addi sp, sp, 8
  ret


file_seek_ecall:


; a0 - FILE*
; a1 - new position
file_seek:
  la t0, fs_running_info
	lw t1, SECTORS_PER_CLUSTER[t0]
	lw t2, FILE_FIRST_CLUSTER[a0]
	lw t3, FILE_SIZE[a0]
	
	li t4, 512
	bgt a1, t3, file_seek_end

	; (new position/512) - new sector
file_seek_end:
	ret
