ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 256K
}

HEAP_STACK_SIZE = 8K;

SECTIONS
{
    . = ORIGIN(RAM);

    .text : ALIGN(4)
    {
        *(.text._start)
        *(.text*)
    } > RAM

    .rodata : ALIGN(4)
    {
        *(.rodata*)
    } > RAM

    .data : ALIGN(4)
    {
        *(.data*)
    } > RAM

    .bss : ALIGN(4)
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > RAM
    _end = .;
    . = ALIGN(16);
    /*  END OF STATIC IMAGE */
    __heap_stack_start = .;
    __heap_stack_end   = __heap_stack_start + HEAP_STACK_SIZE;
}
