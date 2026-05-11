/*  Define the ECALL handlers table */
// NOTE: Add ecalls here
#define ECALL_TABLE_LIST \
    X(ECALL_READ, ecall_read) \
    X(ECALL_WRITE, ecall_write) \
    X(ECALL_OPEN, ecall_open) \
    X(ECALL_CLOSE, ecall_close) \
    X(ECALL_LSEEK, ecall_lseek) \
    X(ECALL_BRK, ecall_brk) \
    X(ECALL_NANOSLEEP, ecall_nanosleep) \
    X(ECALL_GETPID, ecall_getpid) \
    X(ECALL_FORK, ecall_fork) \
    X(ECALL_EXECVE, ecall_execve) \
    X(ECALL_EXIT, ecall_exit) \
    X(ECALL_WAIT, ecall_wait) \

/* Generate the table labels */
ECALL_TABLE_START:
#define X(name, func) ECALL_HANDLER_##name defw func __NL__
ECALL_TABLE_LIST
#undef X
ECALL_TABLE_END:

