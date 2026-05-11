flowchart TD

    START["elf_load_submit(path, pcb)"]

    OPEN["fs_open_submit()"]

    HDR["Read ELF Header"]

    VALIDATE["Validate ELF Header"]

    PHDRSEEK["Seek to Program Header Table"]

    PHDRREAD["Read Program Header"]

    LOADCHECK{"PT_LOAD Segment?"}

    ALLOC["Allocate Memory Region"]

    ZERO["Zero p_memsz Region"]

    CLASSIFY{"Executable Segment?"}

    TEXT["Store as Text Segment"]

    DATA["Store as Data Segment"]

    SEEKSEG["Seek to Segment Payload"]

    READSEG["Read Segment Data"]

    NEXTSEG{"More Program Headers?"}

    CLOSE["fs_close_submit()"]

    INSTALL["Install Process Image into PCB"]

    DONE["Process Ready"]

    ERROR["ELF Error"]

    START --> OPEN
    OPEN --> HDR
    HDR --> VALIDATE

    VALIDATE -->|Invalid| ERROR
    VALIDATE -->|Valid| PHDRSEEK

    PHDRSEEK --> PHDRREAD

    PHDRREAD --> LOADCHECK

    LOADCHECK -->|No| NEXTSEG
    LOADCHECK -->|Yes| ALLOC

    ALLOC --> ZERO

    ZERO --> CLASSIFY

    CLASSIFY -->|PF_X| TEXT
    CLASSIFY -->|PF_W| DATA

    TEXT --> SEEKSEG
    DATA --> SEEKSEG

    SEEKSEG --> READSEG

    READSEG --> NEXTSEG

    NEXTSEG -->|Yes| PHDRREAD
    NEXTSEG -->|No| CLOSE

    CLOSE --> INSTALL

    INSTALL --> DONE
