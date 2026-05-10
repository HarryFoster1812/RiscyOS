#pragma once 
#include "fifo.inc"

STRUCT
RX_FIFO BYTE FIFO_STRUCT_SIZE
ECHO_FIFO BYTE FIFO_STRUCT_SIZE

TTY_WRITE_QUEUE_HEAD WORD
TTY_READ_QUEUE_HEAD WORD
STRUCT_ALIGN 4
TTY_STRUCT_SIZE ALIAS

;struct tty {
;    fifo_t rx_fifo;
;    fifo_t echo_fifo;
;    queue_t waiting_writers;
;    queue_t waiting_readers;
;};
