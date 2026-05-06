#pragma once
#include "../../include/trap.inc"
mhandler:
	csrrw   sp, MSCRATCH, sp  ; swap user and machine stack pointer
    ; save the context
    addi sp, sp, -4
    sw t0, [sp]
    lb t0, kidle
    bnez t0, mhandler_call_trap_handler
    lw t0, current_pcb


    ; save registers
    sw ra,	TF_RA[t0]
    sw gp,	TF_GP[t0]
    sw tp,	TF_TP[t0]
    sw t1,	TF_T1[t0]
    sw t2,	TF_T2[t0]
    sw s0,	TF_S0[t0]
    sw s1,	TF_S1[t0]
    sw a0,	TF_A0[t0]
    sw a1,	TF_A1[t0]
    sw a2,	TF_A2[t0]
    sw a3,	TF_A3[t0]
    sw a4,	TF_A4[t0]
    sw a5,	TF_A5[t0]
    sw a6,	TF_A6[t0]
    sw a7,	TF_A7[t0]
    sw s2,	TF_S2[t0]
    sw s3,	TF_S3[t0]
    sw s4,	TF_S4[t0]
    sw s5,	TF_S5[t0]
    sw s6,	TF_S6[t0]
    sw s7,	TF_S7[t0]
    sw s8,	TF_S8[t0]
    sw s9,	TF_S9[t0]
    sw s10,	TF_S10[t0]
    sw s11,	TF_S11[t0]
    sw t3,	TF_T3[t0]
    sw t4,	TF_T4[t0]
    sw t5,	TF_T5[t0]
    sw t6,	TF_T6[t0]
    
    mv a0, t0
    lw t0, [sp]

    sw t0,	TF_T0[a0]
  
mhandler_call_trap_handler:
    addi sp, sp, 4

    call trap_handler
    lw t6, current_pcb

    lw ra,	TF_RA[t6]
    lw gp,	TF_GP[t6]
    lw tp,	TF_TP[t6]
    lw t0,	TF_T0[t6]
    lw t1,	TF_T1[t6]
    lw t2,	TF_T2[t6]
    lw s0,	TF_S0[t6]
    lw s1,	TF_S1[t6]
    lw a0,	TF_A0[t6]
    lw a1,	TF_A1[t6]
    lw a2,	TF_A2[t6]
    lw a3,	TF_A3[t6]
    lw a4,	TF_A4[t6]
    lw a5,	TF_A5[t6]
    lw a6,	TF_A6[t6]
    lw a7,	TF_A7[t6]
    lw s2,	TF_S2[t6]
    lw s3,	TF_S3[t6]
    lw s4,	TF_S4[t6]
    lw s5,	TF_S5[t6]
    lw s6,	TF_S6[t6]
    lw s7,	TF_S7[t6]
    lw s8,	TF_S8[t6]
    lw s9,	TF_S9[t6]
    lw s10,	TF_S10[t6]
    lw s11,	TF_S11[t6]
    lw t3,	TF_T3[t6]
    lw t4,	TF_T4[t6]
    lw t5,	TF_T5[t6]
    lw t6,	TF_T6[t6]

		csrrw   sp, MSCRATCH, sp  ; swap user and machine stack pointer
    mret


trap_handler:
    addi sp, sp, -4
    sw ra, [sp]
    csrr    t0, MCAUSE 
    bgez    t0, exceptions ; Branch if >= 0 (MSB clear)
    andi    t0, t0, 0xFF

    la      t1, interrupt_table
    slli    t0, t0, 2   ; multiply by 4 to get word offset
    add     t1, t1, t0
    lw      t1, [t1]
    jalr    t1
    j %F1

    exceptions:
    la      t1, exception_table
    slli    t0, t0, 2   ; multiply by 4 to get word offset
    add     t1, t1, t0
    lw      t1, [t1]
    jalr      t1
1
    lw ra, [sp]
    addi sp, sp, 4
		ret

