ARCH v5

kernel_stack_size EQU 600

; Kernel Text
;; Kernel Boot / Arch
#include "boot/boot.s"
#include "./arch/riscv/timer.s"
#include "./arch/riscv/trap.s"
#include "./sys/ecall_handler.s"

;; Scheduler

;; File system

;; IO Drivers
#include "./drivers/drivers.s"

;; Memory
#include "./kernel/memory/umem.s"

; Kernel Static Data
#include "./arch/riscv/trap_table.s"
#include "./sys/syscall_table.s"
#include "./sys/syscalls.inc"
#include "./drivers/peripheral_table.s"

; Genrated includes (C compiled)
#include "_build/all_includes.s"

; Kernel BSS
#include "./kernel/memory/ualloc_array_def.s"

; Kernel HEAP
kernel_heap_start defb 0
; Kernel Stack
org 0x0_3FFF-kernel_stack_size-1
kernel_heap_end defb 0

org 0x0_3FFF
kernel_stack_base:
org 0x4_0000

#include "user/shell.s"
