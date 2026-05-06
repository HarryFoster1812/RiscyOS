#pragma once
#include "types.h"
#include "io_request.h"
struct rw_io_request {
		struct io_request base;
    void *user_buffer;
    unsigned short length;
    unsigned short progress;
};
