STRUCT
	SD_IO_REQUST_ELF_LOAD_TYPE BYTE
  SD_IO_REQUST_ELF_OPEN_FILE_TYPE BYTE
  SD_IO_REQUST_ELF_READ_FILE_TYPE BYTE
  SD_IO_REQUST_ELF_CLOSE_FILE_TYPE BYTE
  SD_IO_REQUST_ELF_LIST_DIR_TYPE BYTE



#pragma once
STRUCT IO_REQ_SIZE

SD_IO_REQ_LBA WORD ; the LBA of the process doing the request (so that paths can be found relative)
SD_IO_REQ_STATE_MACHINE BYTE ; tells us the current operation (eg find file, walk director, lookup FAT, parse directory, read memory ect)


STRUCT_ALIGN 4
SD_IO_REQ_SIZE ALIAS

; NOTE: THIS IS GOING TO CHANGE TO ADD MORE FIELDS LATER
