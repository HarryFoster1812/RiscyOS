#pragma once
#include "./io_request.inc"
#include "./rw_io_request.inc"
#include "./sd_io_request.inc"
#include "./sd_io_request_elf.inc"
#include "./sd_io_request_open.inc"
#include "./sd_io_request_read.inc"

RW_IO_REQ EQU 0
SD_IO_REQ_ELF EQU 1
SD_IO_REQ_OPEN EQU 2
SD_IO_REQ_READ EQU 3

IO_TYPE_TABLE:
  DEFH RW_IO_REQ_SIZE
	DEFH SD_IO_REQ_SIZE_ELF
	DEFH SD_IO_REQ_SIZE_OPEN
	DEFH SD_IO_REQ_SIZE_READ

  ALIGN
