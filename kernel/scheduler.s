; get pid
; int ecall_getpid(void)

; exit
; void ecall_exit(int status)

; yield
; void ecall_yield(void)

; fork
; int fork(void);

; execv
; 0x0 - _crt0 text (.text + .data + .bss)
; heap_base ↑
;   argv strings
;   argv array (pointers)
; heap_ptr ↑
; stack ↓
