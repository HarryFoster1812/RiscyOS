;-----------------------------------------------------
; 
; H. Foster
; Version 1.0
; 6th Feburary 2025
;
; This is the boot code for RiscyOS
;
; Last modified: 6/2/26 (HWF)
;
; Known bugs: None
;
;-----------------------------------------------------

boot:
    la      sp, kernel_stack_base   ; Load address of kernel stack into stack pointer (SP)

    li      t0, 0x0000_1800         ; MPP mask (Machine Previous Privilege) bits 12 and 11
    csrc    MSTATUS, t0             ; Clear MPP bits sets next privilege level to User mode

    la      t0, mhandler            ; Load address of machine trap handler
;		ori			t0, t0, 1								; enable vectored interrupts
    csrw    MTVEC, t0               ; Set trap vector base address to mhandler


    li      t0, (0x800|(1<<7))              ; Bit 11: enable Machine External Interrupt
    csrs    MIE, t0                 ; Set corresponding bit in Machine Interrupt Enable register

    li      t0, 0x80                ; Bit 7: global Machine Interrupt Enable (MSTATUS.MIE)
    csrs    MSTATUS, t0             ; Enable global machine-level interrupts

    li      t0, INT_CONTROLLER_BASE ; Load base address of interrupt controller
    li      t1, PERIPHERAL_ENABLE_BITMASK  ; Interrupt enable mask (device-specific value) [][][][][User]
    sw      t1, INT_ENABLE[t0]      ; Enable interrupt source in interrupt controller
    sw      zero, 12[t0]            ; Clear/acknowledge any pending interrupts

		call kheap_init									; Initalise the kernel heap
    call k_slab_init                ; Initalise the slabs with queues within each one
		call ualloc_init								; initalise the user-space allocator
		; reset the pid allocator
		la t0, pcb_next_pid
		li t1, 1
		sw t1, [t0]

    la t0, kidle
    sb zero, [t0]

		call tty_init										; initalise serial read/write
		call mmu_init										; enable mmu
    call spi_init										; Initise the spi configuration
    call sd_init										; Set up and send sd commands
    call fat_init

		; create init process
		la a0, proc_init
		call kexecve

  
    #if DEBUG==1
      la t0, kernel_stack_base
      bne sp, t0, .                   ; catch a stack leak
    #endif
  
    csrw    MSCRATCH, sp            ; Save kernel stack pointer in MSCRATCH for trap handler use

    call schedule
