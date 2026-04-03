exception_table:
    .word ecall_x                    # Instruction address misaligned 
    .word ecall_x                    # Instruction access fault
    .word ecall_x                    # Illegal instruction
    .word ecall_x                    # Breakpoint
    .word ecall_x                    # Load address misaligned
    .word ecall_x                    # Load access fault
    .word ecall_x                    # Store address misaligned
    .word ecall_x                    # Store access fault
    .word ecall_handler              # Environment call from U-mode
    .word ecall_handler              # Environment call from S-mode
    .word ecall_x                    # Reserved
    .word ecall_handler              # Environment call from M-mode
    .word ecall_x                    # Instruction page fault
    .word ecall_x                    # Load page fault
    .word ecall_x                    # Reserved for future standard use
    .word ecall_x                    # Store page fault

interrupt_table:
    .word ecall_x                    # User software interrupt
    .word ecall_x                    # Supervisor software interrupt
    .word ecall_x                    # Reserved
    .word ecall_x                    # Machine software interrupt
    .word timer_interrupt						# User timer interrupt
    .word timer_interrupt						# Supervisor timer interrupt
    .word ecall_x                    # Reserved
    .word timer_interrupt						# Machine timer interrupt
    .word external_interrupt_handler # User external interrupt
    .word external_interrupt_handler # Supervisor external interrupt
    .word ecall_x                    # Reserved
    .word external_interrupt_handler # Machine external interrupt
