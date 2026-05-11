sequenceDiagram

    participant PROC as User Process
    participant CPU as CPU
    participant MH as mhandler
    participant IRQ as timer_interrupt
    participant SCH as scheduler
    participant CS as context_switch
    participant NEXT as Next Process

    PROC->>CPU: Running

    CPU->>MH: Timer interrupt
    MH->>MH: Save trap frame

    MH->>IRQ: timer_interrupt()

    IRQ->>IRQ: Update timer
    IRQ->>SCH: schedule()

    SCH->>SCH: Find READY PCB

    SCH->>CS: context_switch(next)

    CS->>CS: Save old CSR state
    CS->>CS: Load new CSR state
    CS->>CS: Switch MMU

    CS-->>MH: Return

    MH->>NEXT: Restore registers
    MH->>CPU: mret

    CPU-->>NEXT: Resume execution
