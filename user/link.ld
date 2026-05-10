ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5); /* R + X */
    data PT_LOAD FLAGS(6); /* R + W */
}

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
    } > RAM :text

    .rodata : ALIGN(4)
    {
        *(.rodata*)
    } > RAM :data

    .data : ALIGN(4)
    {
        *(.data*)
    } > RAM :data

    .bss : ALIGN(4)
    {
        __bss_start = .;

        *(.bss*)
        *(COMMON)

        __bss_end = .;
    } > RAM :data

    _end = .;

    . = ALIGN(16);

    __heap_stack_start = .;
    __heap_stack_end   = __heap_stack_start + HEAP_STACK_SIZE;
}
