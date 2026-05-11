stateDiagram-v2

    [*] --> NEW

    NEW --> READY : alloc_pcb()\nexecve/kexecve
    READY --> RUNNING : scheduler selects process

    RUNNING --> BLOCKED : read/write/sleep/wait
    BLOCKED --> READY : unblock_process()

    RUNNING --> READY : timer preemption\nyield()

    RUNNING --> EXITED : ecall_exit()

    EXITED --> REAPED : parent wait()\nreap_child()

    REAPED --> [*]
