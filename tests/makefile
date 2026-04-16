BUILD_DIR = build
CC = gcc

CFLAGS = -g -m32 -I../include

# find all test files
C_SRCS := $(shell find . -name '*_test.c')

# convert test.c → build/test
TARGETS := $(C_SRCS:./%.c=$(BUILD_DIR)/%)

all: $(TARGETS)

# ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# compile rule
$(BUILD_DIR)/%: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
