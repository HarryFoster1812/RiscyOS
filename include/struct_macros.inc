#define STRUCT_BEGIN \
    .equ __offset, 0

#define WORD(name) \
    .equ name, __offset __NL__ \
    .equ __offset, __offset + 4


#define BYTE(name) \
    .equ name, __offset __NL__ \
    .equ __offset, __offset + 1

#define RECORD(val) \
    .equ __offset, val

#define ALIAS(name) \
    .equ name, __offset

#define STRUCT_END(size_name) \
    .equ size_name, __offset
