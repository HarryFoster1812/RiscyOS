typedef enum {
  BASE_IO_REQUEST,
  RW_IO_REQUEST,
  SD_IO_REQUEST,
  SD_IO_REQUEST_ELF,
} io_request_type; 

struct io_request{
    struct io_request *next;
    unsigned short type;
    unsigned char proc_id;
};
