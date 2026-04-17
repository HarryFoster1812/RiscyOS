#!/usr/bin/env python3

import serial
import struct
import time

# ----------------------------
# Constants (Strictly matching RISCProtocol.h)
# ----------------------------
class RISCMemSpace:
    Memory = 0x00
    Registers = 0x10
    CSR = 0x20

class Protocol:
    MEM_OP_MASK = 0x40
    CMD_READ = 0x00
    CMD_WRITE = 0x08
    
    # Type masks
    TYPE_8 = 0x00
    TYPE_16 = 0x01
    TYPE_32 = 0x02

# ----------------------------
# Serial layer
# ----------------------------
class SerialPort:
    def __init__(self, port="/dev/ttyUSB1", baudrate=115200):
        # Using a small timeout to avoid CPU pegging and match C++ VTIME
        self.ser = serial.Serial(port, baudrate, timeout=0.1)

    def write8(self, val):
        self.ser.write(struct.pack("B", val & 0xFF))

    def read8(self):
        while True:
            data = self.ser.read(1)
            if data:
                return data[0]
            # Small sleep to prevent burning CPU if data hasn't arrived
            time.sleep(0.001)

# ----------------------------
# ACS protocol implementation
# ----------------------------
class ACSComms:
    def __init__(self, port="/dev/ttyUSB1"):
        self.io = SerialPort(port)

    def write8(self, v): self.io.write8(v)
    def read8(self): return self.io.read8()

    # Protocol helper methods for multi-byte values (Little Endian)
    def write32(self, val):
        self.io.ser.write(struct.pack("<I", val))

    def read16(self):
        raw = bytes([self.read8(), self.read8()])
        return struct.unpack("<H", raw)[0]

    def read32(self):
        raw = bytes([self.read8() for _ in range(4)])
        return struct.unpack("<I", raw)[0]

    def build_cmd(self, fmt, memspace, is_write=False):
        cmd = Protocol.MEM_OP_MASK | memspace
        
        if is_write:
            cmd |= Protocol.CMD_WRITE
        else:
            cmd |= Protocol.CMD_READ

        if fmt == "B":   cmd |= Protocol.TYPE_8
        elif fmt == "H": cmd |= Protocol.TYPE_16
        elif fmt == "I": cmd |= Protocol.TYPE_32
        else: raise ValueError("Unsupported format")
            
        return cmd

    def slave_read_memory(self, address, count, memspace, fmt):
        if count > 255:
            raise ValueError("Count must be <= 255")

        cmd = self.build_cmd(fmt, memspace)

        # 1. Send Command
        self.write8(cmd)
        # 2. Send Address (32-bit LE)
        self.write32(address)
        # 3. Send Count
        self.write8(count)

        out = []
        for _ in range(count):
            if fmt == "B":
                out.append(self.read8())
            elif fmt == "H":
                out.append(self.read16())
            elif fmt == "I":
                out.append(self.read32())

        return out

# ----------------------------
# Execution Logic
# ----------------------------
def read_memory_dump():
    # Use the port your device is actually on
    try:
        acs = ACSComms("/dev/ttyUSB1")
    except serial.SerialException as e:
        print(f"Error: Could not open port: {e}")
        return []

    base_addr = 0x20400
    total_bytes = 512
    data = []
    
    print(f"Starting dump from 0x{base_addr:X}...")

    remaining = total_bytes
    current_addr = base_addr
    
    while remaining > 0:
        chunk_size = min(remaining, 255)
        
        # Read as Bytes ("B")
        block = acs.slave_read_memory(
            address=current_addr,
            count=chunk_size,
            memspace=RISCMemSpace.Memory,
            fmt="B"
        )

        data.extend(block)
        current_addr += chunk_size
        remaining -= chunk_size
        print(f"Read {len(data)}/{total_bytes} bytes...")

    return data

def write_dump(data, base=0x20400):
    if not data:
        return
    
    with open("dump.bin", "wb") as f:
        f.write(bytes(data))

    with open("dump.hex", "w") as f:
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_string = " ".join(f"{b:02X}" for b in chunk)
            f.write(f"{base+i:08X}: {hex_string}\n")

if __name__ == "__main__":
    dump_data = read_memory_dump()
    if dump_data:
        write_dump(dump_data)
        print(f"Successfully wrote {len(dump_data)} bytes to dump.bin and dump.hex")
