# Use values passed from parent, fallback if run standalone
CC ?= riscv64-unknown-elf-gcc
CFLAGS = -march=rv32im_zicsr -mabi=ilp32 -ffreestanding -Wall -Wextra -O2
LDFLAGS ?=
LIB ?= ../../RiscyLibc/riscylibc.a
INCLUDE ?= ../../RiscyLibc/include

TARGET = shell
SRC = main.c

all:
	$(CC) $(CFLAGS) $(SRC) $(LIB) $(LDFLAGS) -I$(abspath $(INCLUDE)) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
