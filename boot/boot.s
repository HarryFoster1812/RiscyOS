#-----------------------------------------------------
# 
# H. Foster
# Version 1.0
# 6th Feburary 2025
#
# This is the boot code for RiscyOS
#
# Last modified: 6/2/26 (HWF)
#
# Known bugs: None
#
#-----------------------------------------------------

boot:
    la      sp, kernel_stack_base   # Load address of kernel stack into stack pointer (SP)

    li      t0, 0x0000_1800         # MPP mask (Machine Previous Privilege) bits 12 and 11
    csrc    MSTATUS, t0             # Clear MPP bits sets next privilege level to User mode

    la      t0, mhandler            # Load address of machine trap handler
    csrw    MTVEC, t0               # Set trap vector base address to mhandler

    csrw    MSCRATCH, sp            # Save kernel stack pointer in MSCRATCH for trap handler use

    la      sp, user_stack          # Switch stack pointer to user-space stack
    la      ra, user_main           # Load address of user program entry point
    csrw    MEPC, ra                # Set MEPC (Machine Exception PC) to user program start

    li      t0, 0x800               # Bit 11: enable Machine External Interrupt
    csrs    MIE, t0                 # Set corresponding bit in Machine Interrupt Enable register

    li      t0, 0x80                # Bit 7: global Machine Interrupt Enable (MSTATUS.MIE)
    csrs    MSTATUS, t0             # Enable global machine-level interrupts

    li      t0, INT_CONTROLLER_BASE # Load base address of interrupt controller
    li      t1, 0x10                # Interrupt enable mask (device-specific value)
    sw      t1, INT_ENABLE(t0)      # Enable interrupt source in interrupt controller
    sw      zero, 12(t0)            # Clear/acknowledge any pending interrupts

		call kheap_init									# Initalise the kernel heap
		call ualloc_init								# initalise the user-space allocator

    call spi_init										# Initise the spi configuration
    call sd_init										# Set up and send sd commands

    mret                            # Return from machine mode -> jump to MEPC (user_main) in user mode
