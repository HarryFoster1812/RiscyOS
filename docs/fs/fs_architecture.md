flowchart TD

    %% =====================================================
    %% USER / KERNEL REQUEST SOURCES
    %% =====================================================

    USER["User Process"]
    KERNEL["Kernel Subsystem"]
    ELF["ELF Loader FSM"]
    OPEN["fs_open FSM"]
    READ["fs_read FSM"]
    SEEK["fs_seek FSM"]

    USER --> ELF
    USER --> OPEN
    USER --> READ
    USER --> SEEK

    KERNEL --> ELF
    KERNEL --> OPEN

    %% =====================================================
    %% FILESYSTEM FSM LAYER
    %% =====================================================

    subgraph FSMLAYER["Layer 3: Cooperative FSM Layer"]

        ELFSTATE["ELF FSM States:
        OPENING
        READ_HEADER
        LOAD_PHDR
        LOAD_SEGMENT
        CLOSING"]

        OPENSTATE["Open FSM States:
        PARSE_DIR_SECTOR
        PROCESS_FAT
        DONE / ERROR"]

        READSTATE["Read FSM States:
        LOAD_CONTENTS
        COPY_CONTENTS"]

        SEEKSTATE["Seek FSM States:
        WALK_FAT
        DONE / ERROR"]

    end

    ELF --> ELFSTATE
    OPEN --> OPENSTATE
    READ --> READSTATE
    SEEK --> SEEKSTATE

    %% =====================================================
    %% IO SCHEDULER
    %% =====================================================

    subgraph IOSCHED["Layer 2: IO Scheduler"]

        SUBMIT["io_sched_submit(req)"]

        QUEUE["Request Queue"]

        PUMP["io_sched_pump()"]

        INFLIGHT["Current In-Flight Request"]

        CALLBACK["callback(ctx, status)"]

    end

    ELFSTATE --> SUBMIT
    OPENSTATE --> SUBMIT
    READSTATE --> SUBMIT
    SEEKSTATE --> SUBMIT

    SUBMIT --> QUEUE

    PUMP --> QUEUE

    QUEUE --> INFLIGHT

    INFLIGHT --> SDSTART["sd_start_read(lba)"]

    CALLBACK --> ELFSTATE
    CALLBACK --> OPENSTATE
    CALLBACK --> READSTATE
    CALLBACK --> SEEKSTATE

    %% =====================================================
    %% HARDWARE / IRQ LAYER
    %% =====================================================

    subgraph HWLAYER["Layer 1: Hardware + Interrupt Layer"]

        SDHW["SD Card Hardware"]

        IRQ["sd_irq_handler()"]

        RING["Completion Ring Buffer"]

    end

    SDSTART --> SDHW

    SDHW -->|"IRQ"| IRQ

    IRQ -->|"write completion byte"| RING

    RING --> PUMP

    PUMP --> CALLBACK
