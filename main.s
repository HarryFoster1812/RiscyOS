.equ kernel_stack_size, 600

# Kernel Text
#; Kernel Boot / Arch
#include "boot/boot.s"
#include "./arch/riscv/timer.s"
#include "./arch/riscv/trap.s"
#include "./sys/ecall_handler.s"

#; Scheduler

#; File system

#; IO Drivers
#include "./drivers/drivers.s"

#; Memory
#include "./kernel/memory/umem.s"

# Kernel Static Data
#include "./arch/riscv/trap_table.s"
#include "./sys/syscall_table.s"
#include "./sys/syscalls.inc"
#include "./drivers/peripheral_table.s"

# Genrated includes (C compiled)
#include "_build/all_includes.s"

# Kernel BSS
# I know I have limited space but naming the kernel is very important
.section .rodata
kernel_name:
    .string "Sleep-Deprived Squirrel"   # automatically null-terminated

# pointer to the current pcb/process that is executing
.section .data
current_pcb:
    .word 0x0   # 32-bit word for RISC-V# use .hword for 16-bit if needed

#include "./kernel/memory/ualloc_array_def.s"

# Kernel HEAP
.section .bss
kernel_heap_start:
    .byte 0

.org 0x3FFF - kernel_stack_size  # adjust to your stack size
kernel_heap_end:
    .byte 0

.org 0x3FFF
kernel_stack_base:

.org 0x40000
#include "user/shell.s"
