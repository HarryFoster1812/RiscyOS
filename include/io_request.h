#pragma once
#include "types.h"
struct io_request {
    void *user_buffer;
    unsigned short length;
    unsigned short progress;
    struct io_request *next;
    unsigned char proc_id;
};
