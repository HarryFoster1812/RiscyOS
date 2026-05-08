#include <io/io_request_table.inc>
#include <process.inc>

STDIN_FILENO EQU 0
STDOUT_FILENO EQU 1
STDERR_FILENO EQU 2
STDLCD_FILENO EQU 3

; a0 - type
; returns pointer to object
make_io_request:
  addi sp, sp, -4
  sw ra, [sp]
  mv s0, a0
  la t0, IO_TYPE_TABLE
  add t0, t0, a0
  ; depending on type we create different sizes of objects (all inherit from the base class)
  lhu a0, [t0]
  call kmalloc

  beqz a0, make_io_request_fail

  ; fill out basic info
  sh s0, IO_REQ_TYPE[a0]
  la t0, current_pcb
  lbu t1, PCB_PID[t0]
  sb t1, IO_REQ_PROC_ID[a0]
  sh zero, IO_REQ_NEXT[a0]

  make_io_request_fail
  lw ra, [sp]
  addi sp, sp, 4
  ret


; int ecall_write(int fd, void* buf, uint32_t len)

; int ecall_read(int fd, void* buf, uint32_t len)
