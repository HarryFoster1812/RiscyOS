#include <process.inc>

slabs_to_init EQU 2
slabs_init_size EQU 3

k_slab_init:
  addi sp, sp, -8
  sw s0, [sp]
  sw ra, 4[sp]

  li s0, slabs_to_init
  
init_loop:

  la a0, slab_defs
  li t0, slabs_init_size
  mv t1, s0
  ; calculate the location of the foest
  addi t1, t1, -1 ; -1 since we are going backwards
  mul t2, t1, t0
  add a0, a0, t2

  
  ; (slab_header_t **head, uint32_t object_size, uint32_t num_objects)
  lw a2, 8[a0]
  lw a1, 4[a0]
  call slab_alloc_new
  
  addi s0, s0, -1
  bnez s0, init_loop

  lw s0, [sp]
  lw ra, 4[sp]
  addi sp, sp, 8

slab_defs:
pcb_slab_head DEFW 0x0
pcb_slab_object_size DEFW PCB_SIZE
pcb_slab_object_count DEFW 5

file_slab_head DEFW 0x0
file_slab_object_size DEFW PCB_SIZE
file_slab_object_count DEFW 5
