#pragma once
#include "./io_request.inc"
#include "./rw_io_request.inc"
#include "./sd_io_request.inc"

IO_TYPE_TABLE:
  DEFH IO_REQ_SIZE
  DEFH RW_IO_REQ_SIZE
  DEFH SD_IO_REQ_SIZE
