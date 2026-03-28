ARCH v5

; Kernel Text
;; Kernel Boot / Arch
#include "boot/boot.s"
#include "./arch/riscv/timer.s"
#include "./arch/riscv/trap.s"
#include "./sys/ecall_handler.s"

;; Scheduler

;; File system

;; IO Drivers

;; Memory

; Kernel Static Data

; Kernel BSS

; Kernel HEAP

; Kernel Stack

org 0x4_0000

#include "user/shell.s"
