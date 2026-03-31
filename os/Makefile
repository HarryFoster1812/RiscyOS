# Top-level Makefile

# Compiler
CC = gcc
RISCV_CC = riscv64-elf-gcc  # your RISC-V cross-compiler

# Paths
ASSEMBLER_DIR = assembler
RVA_SRC = $(ASSEMBLER_DIR)/rva.c
RVA_BIN = $(ASSEMBLER_DIR)/rva

MAIN_SRC = main.s
OUT_PRE = out.s
OUT_DIR = out

# Output format: -lk (kmd) or -e (ELF)
FORMAT ?= -lk

# Build folder for compiled assembly
BUILD_DIR = _build

# Default: run conversion
SKIP_CONVERT ?= 0

# All C sources in kernel, drivers, and user
C_SRCS := $(shell find kernel drivers user -name '*.c')
S_SRCS := $(patsubst %.c,$(BUILD_DIR)/%.s,$(C_SRCS))
S_CONV_SRCS := $(patsubst %.c,$(BUILD_DIR)/%.out.s,$(C_SRCS))

# Default target
all: $(RVA_BIN) assemble

# Compile rva if needed
$(RVA_BIN): $(RVA_SRC)
	$(CC) $< -o $@

# Compile C -> RISC-V assembly in _build folder
$(BUILD_DIR)/%.s: %.c
	@mkdir -p $(dir $@)
	# -Oz optimise aggressively for size
	$(RISCV_CC) -Oz -S -ffreestanding -nostdlib \
			-march=rv32i -mabi=ilp32 \
			-fno-pic \
			-mno-relax \
			-fno-asynchronous-unwind-tables \
			-fno-exceptions \
			-fno-ident \
			-fverbose-asm \
			$< -o $@

# Generate include aggregator for all generated .s files
$(BUILD_DIR)/all_includes.s: $(S_SRCS)
	@mkdir -p $(BUILD_DIR)
	@echo "; Auto-generated include list for all .s files" > $@
	@for f in $(S_CONV_SRCS); do \
	    echo "#include \"$${f#$(BUILD_DIR)/}\"" >> $@; \
	done

# Build all _build/*.s files from .c
.PHONY: compileC
compileC: $(S_SRCS)
	@echo "Compiled all C sources to .s files"

.PHONY: convert
convert: compileC
ifeq ($(SKIP_CONVERT),0)
	@for f in $(S_SRCS); do \
	    echo "Converting $$f..."; \
	    python3 toolchain/convert_generated_asm.py "$$f"; \
	done
else
	@echo "Skipping conversion step"
endif

# Preprocess main.s (always runs) – now depends on convert
.PHONY: preprocess
preprocess: convert $(BUILD_DIR)/all_includes.s
	@echo "Preprocessing main.s..."
	$(CC) -E -x assembler-with-cpp -P $(MAIN_SRC) | \
	sed 's/__NL__/\
/g' > $(OUT_PRE)

# Run assembler (must run from assembler dir)
assemble: $(RVA_BIN) preprocess
	@mkdir -p $(OUT_DIR)
	cd $(ASSEMBLER_DIR) && ./rva $(FORMAT) $(abspath ../$(OUT_DIR)) ../$(notdir $(OUT_PRE))

# Clean generated files
.PHONY: clean
clean:
	rm -f $(OUT_PRE)
	rm -rf $(BUILD_DIR)
	cd $(ASSEMBLER_DIR) && rm -f rva
	rm -rf $(OUT_DIR)
