#pragma once
#include <file.h>
#include <types.h>

void fs_close_submit(FILE* file, op_complete_cb callback,void* ctx);
