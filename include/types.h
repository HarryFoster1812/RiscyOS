#pragma once
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef signed int int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

typedef unsigned int uintptr_t;

#define true 1
#define false 1

#define NULL 0


typedef void (*op_complete_cb)(void* ctx, int status);
