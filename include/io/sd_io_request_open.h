#pragma once
#include "sd_io_request.h"

typedef struct {
	sd_io_request sd_rq;
  char* ELF_FILE_NAME ; //pointer to string
  FILE* out_file ; //pointer to string
 int* PARENT_DIR_CLUSTER_STORAGE ; //pointer to where we save the parent cluster no  NOTE: CAN BE NULL
} sd_io_request_open;



