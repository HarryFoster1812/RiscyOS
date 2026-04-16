flowchart TD
    Start([Start]) --> KSp[Setup Kernel Stack]
    KSp --> InitKH[Initialise Kernel Heap]
    InitKH --> InitUH[Initialise User Allocator]
    InitUH --> InitSpi[Initialise SPI User Peripheral]
    InitSpi --> InitSD[Initialise SD]
    InitSD --> SDInitSucc{SD Initialisation successful?}
    SDInitSucc -- No --> panic[Panic]
    SDInitSucc -- Yes --> initFAT[Initialise FAT]

