MMU_BASE EQU 0x0001_0900
STRUCT
MMU_OFFSET WORD
MMU_CTRL WORD

USER_RAM_BASE EQU 0x0004_0000

mmu_init:
	li t0, MMU_BASE
	li a0, USER_RAM_BASE

	; Enable address translation (simplistic virt+base)
	li t1, 1
	sw t1, MMU_STATUS[t0]


mmu_set_offset:
	li t0, MMU_BASE
	; set base
	sw a0, MMU_OFFSET[t0]
	ret

