flowchart TD

    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =
    %% USER SPACE
    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =

    subgraph USER["User Processes"]

        UREAD["read(fd, buf, len)"]
        UWRITE["write(fd, buf, len)"]

    end

    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =
    %% ECALL LAYER
    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =

    subgraph ECALL["Syscall / ECALL Layer"]

        EREAD["ecall_read"]
        EWRITE["ecall_write"]

    end

    UREAD --> EREAD
    UWRITE --> EWRITE

    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =
    %% REQUEST MANAGEMENT
    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =

    subgraph REQUESTS["TTY Request Layer"]

        MAKE["tty_make_request()"]

        RWREQ["rw_request_t
        - USER_BUFFER
        - PROC_PCB
        - BYTES_REQUESTED
        - BYTES_FULFILLED
        - NEXT_REQUEST"]

        BLOCK["block_current_process()"]

        RQUEUE["TTY_READ_QUEUE"]

        WQUEUE["TTY_WRITE_QUEUE"]

    end

    EREAD --> MAKE
    EWRITE --> MAKE

    MAKE --> RWREQ

    RWREQ --> BLOCK

    BLOCK --> RQUEUE
    BLOCK --> WQUEUE

    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =
    %% TTY CORE
    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =

    subgraph TTY["TTY Layer"]

        RXFIFO["RX FIFO"]

        ECHOFIFO["Echo FIFO"]

        ENQUEUE["tty_enqueue_receive()"]

        DEQUEUE["tty_dequeue_write()"]

        ECHOLOGIC["Echo Logic
        - normal chars
        - backspace
        - newline"]

    end

    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =
    %% SERIAL INTERRUPTS
    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =

    subgraph SERIAL["Serial Interrupt Layer"]

        RXIRQ["irq_serial_read"]

        TXIRQ["irq_serial_write"]

        UART["UART Hardware"]

    end

    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =
    %% RECEIVE PATH
    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =

    UART --> |"RX interrupt"| RXIRQ

    RXIRQ --> |"read byte"| ENQUEUE

    ENQUEUE --> ECHOLOGIC

    ECHOLOGIC --> ECHOFIFO

    ENQUEUE --> |"waiting reader?"| RQUEUE

    RQUEUE --> |"copy byte into user buffer"| RWREQ

    RWREQ --> |"request complete"| UNBLOCKR["unblock_process()"]

    ENQUEUE --> |"no waiting reader"| RXFIFO

    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =
    %% TRANSMIT PATH
    %% == == == == == == == == == == == == == == == == == == == == == == == == == == =

    UART --> |"TX ready interrupt"| TXIRQ

    TXIRQ --> DEQUEUE

    DEQUEUE --> |"prioritise echo"| ECHOFIFO

    DEQUEUE --> |"otherwise user writes"| WQUEUE

    WQUEUE --> RWREQ

    RWREQ --> |"copy next byte"| UART

    RWREQ --> |"write complete"| UNBLOCKW["unblock_process()"]

    ECHOFIFO --> UART
