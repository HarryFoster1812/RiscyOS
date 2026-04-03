timer_interrupt:
    li      t0, 200000              # interval

    li      t5, 0x0200BFF8          # mtime address (platform-specific)
    lw      t1, 0(t5)               # mtime low
    lw      t2, 4(t5)               # mtime high

    add     t3, t1, t0
    sltu    t4, t3, t1              # carry
    add     t2, t2, t4

    li      t6, 0x02004000          # mtimecmp base (platform-specific)

    li      t4, -1
    sw      t4, 4(t6)               # set high to max

    sw      t3, 0(t6)               # write low
    sw      t2, 4(t6)               # write high

		# need to decide
    ret
