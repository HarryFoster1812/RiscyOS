#pragma once
#include "../../include/trap.inc"
mhandler:
	csrrw   sp, MSCRATCH, sp  # swap user and machine stack pointer
    # save the context
    addi sp, sp, -TRAP_FRAME_SIZE          # allocate trap frame

    # save registers
    sw ra,	TF_RA(sp)
    sw gp,	TF_GP(sp)
    sw tp,	TF_TP(sp)
    sw t0,	TF_T0(sp)
    sw t1,	TF_T1(sp)
    sw t2,	TF_T2(sp)
    sw s0,	TF_S0(sp)
    sw s1,	TF_S1(sp)
    sw a0,	TF_A0(sp)
    sw a1,	TF_A1(sp)
    sw a2,	TF_A2(sp)
    sw a3,	TF_A3(sp)
    sw a4,	TF_A4(sp)
    sw a5,	TF_A5(sp)
    sw a6,	TF_A6(sp)
    sw a7,	TF_A7(sp)
    sw s2,	TF_S2(sp)
    sw s3,	TF_S3(sp)
    sw s4,	TF_S4(sp)
    sw s5,	TF_S5(sp)
    sw s6,	TF_S6(sp)
    sw s7,	TF_S7(sp)
    sw s8,	TF_S8(sp)
    sw s9,	TF_S9(sp)
    sw s10,	TF_S10(sp)
    sw s11,	TF_S11(sp)
    sw t3,	TF_T3(sp)
    sw t4,	TF_T4(sp)
    sw t5,	TF_T5(sp)
    sw t6,	TF_T6(sp)

    mv a0, sp               # pass trap frame pointer
    call trap_handler

    lw ra,	TF_RA(sp)
    lw gp,	TF_GP(sp)
    lw tp,	TF_TP(sp)
    lw t0,	TF_T0(sp)
    lw t1,	TF_T1(sp)
    lw t2,	TF_T2(sp)
    lw s0,	TF_S0(sp)
    lw s1,	TF_S1(sp)
    lw a0,	TF_A0(sp)
    lw a1,	TF_A1(sp)
    lw a2,	TF_A2(sp)
    lw a3,	TF_A3(sp)
    lw a4,	TF_A4(sp)
    lw a5,	TF_A5(sp)
    lw a6,	TF_A6(sp)
    lw a7,	TF_A7(sp)
    lw s2,	TF_S2(sp)
    lw s3,	TF_S3(sp)
    lw s4,	TF_S4(sp)
    lw s5,	TF_S5(sp)
    lw s6,	TF_S6(sp)
    lw s7,	TF_S7(sp)
    lw s8,	TF_S8(sp)
    lw s9,	TF_S9(sp)
    lw s10,	TF_S10(sp)
    lw s11,	TF_S11(sp)
    lw t3,	TF_T3(sp)
    lw t4,	TF_T4(sp)
    lw t5,	TF_T5(sp)
    lw t6,	TF_T6(sp)

    addi sp, sp, TRAP_FRAME_SIZE
		csrrw   sp, MSCRATCH, sp  # swap user and machine stack pointer
    mret


trap_handler:
    addi sp, sp, -4
    sw ra, (sp)
    csrr    t0, MCAUSE 
    bgez    t0, exceptions # Branch if >= 0 (MSB clear)
    andi    t0, t0, 0xFF

    la      t1, interrupt_table
    slli    t0, t0, 2   # multiply by 4 to get word offset
    add     t1, t1, t0
    lw      t1, (t1)
    jalr    t1
    j %F1

    exceptions:
    la      t1, exception_table
    slli    t0, t0, 2   # multiply by 4 to get word offset
    add     t1, t1, t0
    lw      t1, (t1)
    jalr      t1
1
    lw ra, (sp)
    addi sp, sp, 4
		ret

