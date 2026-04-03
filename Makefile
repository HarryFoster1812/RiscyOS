# Top-level Makefile (CPP + single final assembly)

RISCV_PREFIX ?= riscv64-unknown-elf

RISCV_CC = $(RISCV_PREFIX)-gcc
RISCV_OBJDUMP = $(RISCV_PREFIX)-objdump
RISCV_OBJCOPY = $(RISCV_PREFIX)-objcopy

RISCV_FLAGS = -march=rv32im_zicsr -mabi=ilp32

MAIN_SRC = main.s
OUT_DIR = out
BUILD_DIR = _build

# Find all C sources
C_SRCS := $(shell find kernel drivers sys arch boot -name '*.c')
S_SRCS := $(patsubst %.c,$(BUILD_DIR)/%.s,$(C_SRCS))

# Final outputs
KERNEL_ELF = $(OUT_DIR)/kernel.elf
KERNEL_BIN = $(OUT_DIR)/kernel.bin
MAIN_PRE = $(BUILD_DIR)/main_pre.s

# C -> assembly flags
CFLAGS = -Oz -ffreestanding -nostdlib \
         -march=rv32i -mabi=ilp32 \
         -fno-pic -mno-relax \
         -fno-asynchronous-unwind-tables \
         -fno-exceptions -fno-ident

LDFLAGS = -nostdlib -T linker.ld

# -------------------------
# Default target
# -------------------------
all: $(KERNEL_ELF) $(KERNEL_BIN)

# -------------------------
# Step 1: C -> .s
# -------------------------
$(BUILD_DIR)/%.s: %.c
	@mkdir -p $(dir $@)
	$(RISCV_CC) $(CFLAGS) -S $< -o $@

# -------------------------
# Step 2: Generate include list
# -------------------------
$(BUILD_DIR)/all_includes.s: $(S_SRCS)
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)
	@echo "# Auto-generated include list for all .s files" > $@
	@for f in $(filter-out $(BUILD_DIR)/user/%,$(S_SRCS)); do \
	    echo "#include \"$${f#$(BUILD_DIR)/}\"" >> $@; \
	done

# -------------------------
# Step 3: Preprocess main.s (THIS IS KEY)
# -------------------------
$(MAIN_PRE): $(MAIN_SRC) $(BUILD_DIR)/all_includes.s
	@mkdir -p $(BUILD_DIR)
	@echo "Preprocessing main.s..."
	$(RISCV_CC) -E -x assembler-with-cpp -P $(MAIN_SRC) | \
	sed 's/__NL__/\
/g' > $@

# -------------------------
# Step 4: Assemble ONLY main
# -------------------------
$(BUILD_DIR)/main.o: $(MAIN_PRE)
	$(RISCV_CC) -c $< -o $@ $(RISCV_FLAGS)

# -------------------------
# Step 5: Link
# -------------------------
$(KERNEL_ELF): $(BUILD_DIR)/main.o
	@mkdir -p $(OUT_DIR)
	$(RISCV_CC) $< $(LDFLAGS) -o $@ $(RISCV_FLAGS)

# -------------------------
# Binary
# -------------------------
$(KERNEL_BIN): $(KERNEL_ELF)
	$(RISCV_OBJCOPY) -O binary $< $@

# -------------------------
# Debug
# -------------------------
.PHONY: disasm
disasm: $(KERNEL_ELF)
	$(RISCV_OBJDUMP) -d $<

# -------------------------
# Clean
# -------------------------
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(OUT_DIR)
