.PHONY: run-rva
run-rva:
	touch $(abspath $(OUT_DIR))/main.elf
	touch $(abspath $(OUT_DIR))/main.kmd
	./rva -e $(abspath $(OUT_DIR))/main.elf ../$(notdir $(OUT_PRE))
	./rva -lk $(abspath $(OUT_DIR))/main.kmd ../$(notdir $(OUT_PRE))
