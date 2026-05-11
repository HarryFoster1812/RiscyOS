#include <process.inc>

STDIN_FILENO EQU 0
STDOUT_FILENO EQU 1
STDERR_FILENO EQU 2
STDLCD_FILENO EQU 3


make_file_struct:
	li a0, FILE_STRUCT_SIZE
	tail kmalloc
	// TODO: Make a manager so i can addd fd

; int ecall_write(int fd, void* buf, uint32_t len)
; since there is no file writing we just assume write requests are to serial
ecall_write:
addi sp, sp, -4
sw ra, [sp]
la a3, TTY_INFO
lw a3, [a3]
addi a3, a3, TTY_WRITE_QUEUE_HEAD
call tty_make_request

; if we get zero back then the request could not be made and we return failure
; else the process is blocked as we call schedule
lw ra, [sp]
addi sp, sp, 4

beqz a0, schedule
la t0, current_pcb
lw t0, [t0]
sw zero, TF_A0[t0]
ret

; int ecall_read(int fd, void* buf, uint32_t len)
ecall_read:
call tty_make_request
ret

; fd open(path)
ecall_open:
	ret


; fd open(path)
ecall_close:
	ret


ecall_lseek:
	ret
