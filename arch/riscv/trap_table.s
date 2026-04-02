exception_table:
    defw ecall_x                    ; Instruction address misaligned 
    defw ecall_x                    ; Instruction access fault
    defw ecall_x                    ; Illegal instruction
    defw ecall_x                    ; Breakpoint
    defw ecall_x                    ; Load address misaligned
    defw ecall_x                    ; Load access fault
    defw ecall_x                    ; Store address misaligned
    defw ecall_x                    ; Store access fault
    defw ecall_handler              ; Environment call from U-mode
    defw ecall_handler              ; Environment call from S-mode
    defw ecall_x                    ; Reserved
    defw ecall_handler              ; Environment call from M-mode
    defw ecall_x                    ; Instruction page fault
    defw ecall_x                    ; Load page fault
    defw ecall_x                    ; Reserved for future standard use
    defw ecall_x                    ; Store page fault

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
