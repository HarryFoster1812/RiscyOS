#pragma once
// NOTE: DO NOT TOUCH IF YOU WANT TO ADD ECALLS DO IT IN SYSCALL_TABLES.s

/*  Generate the index EQUs */
#define X(name, func) \
    .equ name##_INDEX , (ECALL_HANDLER_##name - ECALL_TABLE_START)/4 __NL__
ECALL_TABLE_LIST
#undef X

.equ ECALL_MAX_INDEX, (ECALL_TABLE_END - ECALL_TABLE_START)/4
