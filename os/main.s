ARCH v5

; Kernel Text
;; Kernel Boot / Arch
#include "boot/boot.s"
;#include "./arch/riscv/timer.s"
#include "./arch/riscv/trap.s"
#include "./sys/ecall_handler.s"

;; Scheduler

;; File system

;; IO Drivers
#include "./drivers/drivers.s"

;; Memory

; Kernel Static Data
#include "./arch/riscv/trap_table.s"
#include "./sys/syscall_table.s"
#include "./drivers/peripheral_table.s"

; Kernel BSS

; Kernel HEAP

; Kernel Stack
org 0x0_3FFF
kernel_stack_base DEFW 0x0
org 0x4_0000

#include "user/shell.s"
