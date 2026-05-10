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
ecall_write:

; int ecall_read(int fd, void* buf, uint32_t len)
ecall_read:

; fd open(path)
ecall_open:

