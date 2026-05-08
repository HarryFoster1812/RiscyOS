#pragma once
typedef enum {
  RW_IO_REQUEST,
  SD_IO_REQUEST_ELF,
  SD_IO_REQUEST_OPEN,
  SD_IO_REQUEST_READ,
} io_request_type; 

typedef struct io_request{
    struct io_request *next;
    unsigned short type;
    unsigned char proc_id;
} io_request;
