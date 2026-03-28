# Makefile

# Compiler
CC = gcc

# Paths
ASSEMBLER_DIR = assembler
RVA_SRC = $(ASSEMBLER_DIR)/rva.c
RVA_BIN = $(ASSEMBLER_DIR)/rva
MAIN_SRC = main.s
OUT_PRE = out.s
OUT_DIR = out

# Output format: -lk (kmd) or -e (ELF)
FORMAT ?= -lk

# Default target
all: $(RVA_BIN) assemble

# Compile rva if needed
$(RVA_BIN): $(RVA_SRC)
	$(CC) $< -o $@

# Preprocess main.s (always runs)
.PHONY: preprocess
preprocess:
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
	cd $(ASSEMBLER_DIR) && rm -f rva
	rm -rf $(OUT_DIR)
