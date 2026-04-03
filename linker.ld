/* Simple RISC-V linker script */

OUTPUT_FORMAT("elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(boot)           /* Entry point symbol in your assembly */

SECTIONS
{
    /* Place code (.text) at 0x0 */
    . = 0x00000000;

    .text : 
    {
        *(.text)          /* All .text sections from input files */
    }

    /* Read-only data */
    .rodata : 
    {
        *(.rodata)        /* ECALL table, constants */
    }

    /* Initialized data */
    .data :
    {
        *(.data)          /* Any initialized globals */
    }

    /* Uninitialized data */
    .bss :
    {
        *(.bss)
        *(COMMON)
    }
}
