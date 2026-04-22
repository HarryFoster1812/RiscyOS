exception_table:
    defw ecall_x                    ; Instruction address misaligned 
    defw ecall_x                    ; Instruction access fault - This can be either a swap on demand or a real access fault (check if section target in swap space)
    defw ecall_x                    ; Illegal instruction
    defw ecall_x                    ; Breakpoint
    defw ecall_x                    ; Load address misaligned 
    defw ecall_x                    ; Load access fault (Segfault and kill process)
    defw ecall_x                    ; Store address misaligned
    defw ecall_x                    ; Store access fault (Segfault and kill process)
    defw ecall_handler              ; Environment call from U-mode
    defw ecall_handler              ; Environment call from S-mode (S mode unused for this kernel implementaion although it should be)
    defw ecall_x                    ; Reserved
    defw ecall_handler              ; Environment call from M-mode 
    defw ecall_x                    ; Instruction page fault (I dont know where in this system the trap comes from since there is no paging)
    defw ecall_x                    ; Load page fault 
    defw ecall_x                    ; Reserved for future standard use
    defw ecall_x                    ; Store page fault  (I dont know where in this system the trap comes from since there is no paging)

interrupt_table:
    defw ecall_x                    ; User software interrupt
    defw ecall_x                    ; Supervisor software interrupt
    defw ecall_x                    ; Reserved
    defw ecall_x                    ; Machine software interrupt
    defw timer_interrupt						; User timer interrupt
    defw timer_interrupt						; Supervisor timer interrupt
    defw ecall_x                    ; Reserved
    defw timer_interrupt						; Machine timer interrupt
    defw external_interrupt_handler ; User external interrupt
    defw external_interrupt_handler ; Supervisor external interrupt
    defw ecall_x                    ; Reserved
    defw external_interrupt_handler ; Machine external interrupt
