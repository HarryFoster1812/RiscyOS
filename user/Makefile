RISCV_PREFIX ?= riscv64-unknown-elf
CC = $(RISCV_PREFIX)-gcc
AR = $(RISCV_PREFIX)-ar
LD = $(RISCV_PREFIX)-ld
OBJCOPY = $(RISCV_PREFIX)-objcopy

LDFLAGS = -T $(abspath link.ld) -nostdlib

BUILD_DIR = build
LIB = RiscyLibc/riscylibc.a

# User programs
USER_DIR = user_programs
PROGRAMS = init hello shell

# Final binaries
BINS = $(addprefix $(BUILD_DIR)/, $(PROGRAMS))

# Default
all: $(LIB) $(BINS)

# Build libc
$(LIB):
	$(MAKE) -C RiscyLibc

# Build user programs
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: $(LIB) | $(BUILD_DIR)
	$(MAKE) -C $(USER_DIR)/$* \
		CC=$(CC) \
		AR=$(AR) \
		LDFLAGS="$(LDFLAGS)" \
		LIB=$(abspath $(LIB)) \
		INCLUDE=$(abspath RiscyLibc/include)
	cp $(USER_DIR)/$*/$* $@

# Clean
clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C RiscyLibc clean
	for p in $(PROGRAMS); do \
		$(MAKE) -C $(USER_DIR)/$$p clean; \
	done

.PHONY: all clean
