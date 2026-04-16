ARCH v5

#define DEBUG 0

kernel_stack_size EQU 600

; Kernel Text
;; Kernel Boot / Arch
#include "boot/boot.s"
#include "./arch/riscv/timer.s"
#include "./arch/riscv/trap.s"
#include "./sys/ecall_handler.s"

;; Scheduler
#include "./kernel/process.s"
#include "./kernel/scheduler.s"
#include "./arch/riscv/context_switch.s"

;; File system
#include "./kernel/fs/fs.s"

;; IO Drivers
#include "./drivers/drivers.s"

;; Memory
#include "./kernel/memory/umem.s"
#include "./lib/memutils.s"

; Kernel Static Data
#include "./arch/riscv/trap_table.s"
#include "./sys/syscall_table.s"
#include "./sys/syscalls.inc"
#include "./drivers/peripheral_table.s"

; Genrated includes (C compiled)
#include "_build/all_includes.s"

; Kernel BSS
; I know I have limited space but naming the kernel is very important
kernel_name DEFB "Sleep-Deprived Squirrel\0"

; pointer to the current pcb/process that is executing
current_pcb DEFW 0x0
IDLE_TASK_PCB DEFW 0x0 ; holds a reference to the idle_program pcb which the scheduler will switch to if there is no availble program
#include "./kernel/slab_def.s"
#include "./kernel/memory/ualloc_array_def.s"

; Kernel HEAP
kernel_heap_start DEFB 0
; Kernel Stack
org 0x0_3FFF-kernel_stack_size-1
kernel_heap_end DEFB 0

org 0x0_3FFF
kernel_stack_base:
org 0x4_0000

#include "user/shell.s"
