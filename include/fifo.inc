#pragma once
STRUCT
FIFO_BASE WORD ; Pointer to FIFO array
FIFO_HEAD BYTE ; index into array
FIFO_TAIL BYTE ; index into array
FIFO_SIZE BYTE ; size of array
STRUCT_ALIGN 4
FIFO_STRUCT_SIZE ALIAS
