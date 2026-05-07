#pragma once
#include "./io_request.inc"
#include "./rw_io_request.inc"
#include "./sd_io_request.inc"
#include "./sd_io_request_elf.inc"

IO_REQ EQU 0
RW_IO_REQ EQU 1
SD_IO_REQ EQU 2
SD_IO_REQ_ELF EQU 3

IO_TYPE_TABLE:
  DEFH IO_REQ_SIZE
  DEFH RW_IO_REQ_SIZE
  DEFH SD_IO_REQ_SIZE
	DEFH SD_IO_REQ_SIZE_ELF
