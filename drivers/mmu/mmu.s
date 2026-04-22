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

  lw t1, PCB_PENTRY[a0] ; load Text section base
  sw t1, MMU_IMMU_BASE[t0]
  
  lw t2, PCB_PTEXT_SIZE[a0] 
  add t2, t2, t1 ; immu limit = text base + size
  sw t2, MMU_IMMU_LIMIT[t0]

  // dmmu base
  lw t1, PCB_DMMU_PHYS[a0]
  sw t1, MMU_DMMU_BASE[t0]

  // dmmu virtual start
  lw t2, PCB_PDATA_START[a0]
  sw t2, MMU_DMMU_VIRTUAL_START[t0]

  lw t3, PCB_PHEAP_START[a0]
  li t4, STACK_SIZE
  add t5, t3, t4 ;  heap_start + STACK = virt limit 
  sub t5, t5, t2 ; virt_limit - .rodata = data section size
  add t5, t5, t1 ; physical limit = pyhs base + data size 
  sw t5, MMU_DMMU_LIMIT[t0]
	ret

