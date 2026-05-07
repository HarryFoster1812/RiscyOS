#pragma once
#include "io_request.h"

typedef struct {
	struct io_request request;
	int lba;
	char state_machine;
} sd_io_request;

