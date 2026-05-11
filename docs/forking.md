flowchart TD

    A["ecall_fork()"] --> B[Acquire shared text segment]
    B --> C[Allocate new user data memory]
    C --> D[Allocate child PCB]

    D --> E[Create child data memory region]
    E --> F[Copy parent PCB state]

    F --> G[Copy trap frame]
    G --> H[Set child TF_A0 = 0]
    G --> I[Set parent TF_A0 = child PID]

    I --> J[Share text segment]
    J --> K[Duplicate data segment]

    K --> L[Insert child into PCB list]
    L --> M[Set child READY]
