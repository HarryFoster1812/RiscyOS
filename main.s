ARCH v5

#define DEBUG 0

kernel_stack_size EQU 1000

; Kernel Text
;; Kernel Boot / Arch
#include "boot/boot.s"
#include "./arch/riscv/timer.s"
#include "./arch/riscv/trap.s"
#include "./sys/ecall_handler.s"

;; Scheduler
#include "./kernel/idle_task.s"
#include "./kernel/process.s"
#include "./kernel/scheduler.s"
#include "./arch/riscv/context_switch.s"

;; File system
#include "./kernel/fs/fs.s"
#include "./kernel/io/io.s"

;; IO Drivers
#include "./kernel/io/tty.s"
#include "./drivers/drivers.s"

;; Memory
#include "./kernel/memory/umem.s"
#include "./kernel/memory/heap.s"
#include "./kernel/memory/memory_segment.s"

;; Utility Functions
#include "./lib/fifo.s"
#include "./lib/maths.s"
#include "./lib/delay.s"
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
proc_init DEFB "/hello1\0"

ALIGN

; pointer to the current pcb/process that is executing
current_pcb DEFW 0x0

#include "./kernel/slab_def.s"
#include "./kernel/memory/ualloc_array_def.s"

kidle DEFB 0x0 ; bool telling the kernel if it is currently idle

ALIGN

; Kernel HEAP
kernel_heap_start DEFB 0
; Kernel Stack
org 0x0_4000-kernel_stack_size
kernel_heap_end DEFB 0

org 0x0_4000
kernel_stack_base:
org 0x4_0000

//#include "user/shell.s"
