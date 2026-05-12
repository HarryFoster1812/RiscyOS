#include <process.inc>

MMU_BASE EQU 0x0001_0900
STRUCT
MMU_IMMU_BASE WORD
MMU_IMMU_LIMIT WORD
MMU_DMMU_BASE WORD
MMU_DMMU_LIMIT WORD
MMU_DMMU_VIRTUAL_START WORD
MMU_ENABLE WORD
MMU_STATUS WORD

USER_RAM_BASE EQU 0x0004_0000

mmu_init:
	li t0, MMU_BASE
	li a0, USER_RAM_BASE

	; Enable address translation (simplistic virt+base)
	li t1, 1
	sw t1, MMU_ENABLE[t0]
  ret


; a0 - pcb_t* pcb
mmu_set_proc:
	li t0, MMU_BASE

  lw t1, PCB_PTEXT_MEMORY_REGION[a0] ; load Text section base
	lw t2,  MEMORY_REGION_PHYSICAL_BASE[t1]
  sw t2, MMU_IMMU_BASE[t0]
  
  lw t3, MEMORY_REGION_SIZE[t1] 
  add t3, t3, t2 ; immu limit = text base + size
  sw t3, MMU_IMMU_LIMIT[t0]

  // dmmu base
  lw t1, PCB_PDATA_MEMORY_REGION[a0]
	lw t2, MEMORY_REGION_PHYSICAL_BASE[t1]
  sw t2, MMU_DMMU_BASE[t0]

  // dmmu virtual start
  lw t3, PCB_PDATA_START[a0]
  sw t3, MMU_DMMU_VIRTUAL_START[t0]

  lw t4, MEMORY_REGION_SIZE[t1]
  add t4, t4, t2 ; physical limit = pyhs base + data size 
  sw t4, MMU_DMMU_LIMIT[t0]
	ret

