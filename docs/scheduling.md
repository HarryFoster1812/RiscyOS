flowchart TD
    Start([Start]) --> Ready[Ready Queue]

    Ready --> |Dispatch| Running[Running]

    Running --> |Time Slice Expired| Ready
    Running --> |I/O Request| Waiting[Waiting / Blocked]
    Waiting --> |I/O Complete| Ready

    Running --> |Process Exit| Terminated[Terminated]
    Ready --> |Killed / Abort| Terminated
