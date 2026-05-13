# RiscyOS

A minimalist RISC-V multitasking kernel developed as a final project for the **University of Manchester Microcontrollers (COMP22111)** unit. RiscyOS demonstrates a full vertical slice of OS design from bare-metal boot and hardware initialisation to loading and executing user-space programs from a FAT32 filesystem.

---

##  University of Manchester Project Context

### Non-Conventional Toolchain

RiscyOS uses a **bespoke assembly syntax** tailored for the project's specific assembler (found in `tools/`) created by Dr James Garside.

* **The Assembler:** Unlike standard GNU Assembler (GAS), this dialect uses square brackets unlike conventional parentheses.
* **Transpilation:** A custom Python utility (`tools/gas-to-jim/`) is included to convert GAS RISC-V assembly patterns into the specific dialect required by the university's internal teaching tools.

---

## Core Design

### Trap Handling & Context Switching

* **Trap Frame Model:** Machine-mode traps save all 31 general-purpose registers into a `trap_frame` structure. The kernel operates directly on this frame, and return values are written into the saved `a0/a1` slots.
* **Memory-Constrained Stack:** To save RAM, the kernel utilizes a single global stack; there are no per-process kernel stacks.
* **State Restoration:** Context switching swaps the `mepc`, `mstatus`, and register sets stored directly in the **Process Control Block (PCB)**.

### Memory Management

* **Slab Allocator:** Fast, fixed-size object pools for frequent kernel structures (PCBs, file descriptors).
* **Kernel Heap (`kmalloc`):** A header-based allocator with coalescing for general kernel tasks.
* **Base/Limit MMU:** Implements process isolation via a lightweight custom design:
* **IMMU:** Base + Limit.
* **DMMU:** Base + Limit + Virtual Start.
* **Translation:** $physical = target - virtual\_start + base$



---

## Process & Execution Model

### ELF Loading Pipeline

1. **Locate:** Find file via FAT32 directory traversal.
2. **Fetch:** Read from SD card via SPI interrupt-driven state machine.
3. **Parse:** Validate ELF headers and segments.
4. **Allocate:** Map segments using the flat bitfield allocator.
5. **Run:** Read and copy segment contents and schedule the process.

### Process Operations

* **`fork()`:** Shares the text segment (read-only) and duplicates the data segment.
* **`execve()`:** Overwrites current process state with a new ELF binary from disk.
* **Scheduler:** A PID-based round-robin scheduler. If no processes are `READY`, the kernel enters a `kidle` state to conserve cycles.

---

## Filesystem & I/O

* **FAT32 Support:** Implements cluster walking, sector-to-LBA mapping, and file read operations over SPI.
* **Interrupt-Driven I/O:** Serial (UART) and SD card operations are managed via non-blocking Event-Driven FSM queues.
* **UART/TTY Layer:** Provides a terminal interface with line-discipline support (handling backspace and newline echos).

---

## Hardware Notes

* **Architecture:** 32-bit RISC-V (RV32I).
* **Storage:** SD card connected via SPI.
* **Implementation Note:** Developed on physical hardware

---

## Status

**Working:**

* Multitasking & Context Switching.
* FAT32 Driver (Read-only) & SPI FSM.
* Async ELF loading and execution.
* Base/Limit memory isolation.
* Slab & Bitfield allocators.

**In Progress:**

* Process termination (`exit`) and Signal handling.
* Filesystem write support (`mkdir`, `write`).
* Shell environment (Internal commands like `cd`, `ls`).
* Enhanced QEMU parity for faster testing.

---

*Developed as part of the University of Manchester COMP22111 Microcontrollers Project.*
