"""
Protocol reverse-engineered from ACSComms / ACSProtocol C++ source.
All multi-byte values are little-endian.

Typical usage
-------------
    from board_comms import BoardComms

    with BoardComms("/dev/ttyUSB0") as b:
        b.reset()
        b.load_memory(0x0000_0000, my_words)   # list[int] of 32-bit words
        b.run()
        status = b.status()
        print(status)
        b.stop()
        regs = b.read_registers()
        b.memory_dump(0x0000_0000, 256)

CLI usage
---------
    python board_comms.py --port /dev/ttyUSB0 status
    python board_comms.py --port /dev/ttyUSB0 run
    python board_comms.py --port /dev/ttyUSB0 stop
    python board_comms.py --port /dev/ttyUSB0 reset
    python board_comms.py --port /dev/ttyUSB0 step [N]
    python board_comms.py --port /dev/ttyUSB0 regs
    python board_comms.py --port /dev/ttyUSB0 dump <addr_hex> <num_words>
    python board_comms.py --port /dev/ttyUSB0 writemem <addr_hex> <value_hex>
    python board_comms.py --port /dev/ttyUSB0 readmem <addr_hex>
    python board_comms.py --port /dev/ttyUSB0 readcsr <csr_addr_hex>
    python board_comms.py --port /dev/ttyUSB0 profile <seconds>
    python board_comms.py --port /dev/ttyUSB0 breakpoint set <id> <addr_hex>
    python board_comms.py --port /dev/ttyUSB0 breakpoint list
    python board_comms.py --port /dev/ttyUSB0 uarttest
    python board_comms.py --port /dev/ttyUSB0 echo <byte_hex>
"""

import argparse
import struct
import sys
import time
from collections import Counter
from dataclasses import dataclass
from enum import IntEnum
from typing import List, Optional

try:
    import serial
except ImportError:
    sys.exit("pyserial is required:  pip install pyserial")


# ---------------------------------------------------------------------------
# Protocol constants  (from ACSProtocol.h)
# ---------------------------------------------------------------------------

BAUD_RATE = 115_200

class CMD(IntEnum):
    NoOp             = 0x00
    Ping             = 0x01
    ResetPeriph      = 0b0000_0010   # 0x02
    Reset            = 0b0000_0100   # 0x04
    WhatIsExecuting  = 0b0000_0101   # 0x05
    Stop             = 0b0010_0001   # 0x21
    Pause            = 0b0010_0010   # 0x22
    Continue         = 0b0010_0011   # 0x23
    BreakPointsRead  = 0b0011_0000   # 0x30
    BreakPointsSet   = 0b0011_0001   # 0x31
    PeripheralRead   = 0b0001_0000   # 0x10
    PeripheralWrite  = 0b0001_0001   # 0x11
    # Memory ops: base 0x40, OR space | size | rw
    MemRead32        = 0x40 | 0x00 | 0x02 | 0x00   # 0x42
    MemRead16        = 0x40 | 0x00 | 0x01 | 0x00   # 0x41
    MemRead8         = 0x40 | 0x00 | 0x00 | 0x00   # 0x40
    MemWrite32       = 0x40 | 0x00 | 0x02 | 0x08   # 0x4A
    MemWrite16       = 0x40 | 0x00 | 0x01 | 0x08   # 0x49
    MemWrite8        = 0x40 | 0x00 | 0x00 | 0x08   # 0x48
    RegRead32        = 0x40 | 0x10 | 0x02 | 0x00   # 0x52
    RegWrite32       = 0x40 | 0x10 | 0x02 | 0x08   # 0x5A
    CSRRead32        = 0x40 | 0x20 | 0x02 | 0x00   # 0x62
    CSRWrite32       = 0x40 | 0x20 | 0x02 | 0x08   # 0x6A
    WatchPointsRead  = 0xF4
    WatchPointsSet   = 0xF5
    SystemFeatures   = 0xF9
    SystemFlash      = 0xFA
    SystemRead       = 0xFD
    SystemWrite      = 0xFC
    SystemRun        = 0xFB
    SystemUartTest   = 0xFE
    SystemEcho       = 0xFF
    Run              = 0b1000_0000   # 0x80  – write32(steps): 0=free-run


class ProcStatus(IntEnum):
    Reset                = 0x00
    Busy                 = 0x01
    BusyStepping         = 0x02
    StoppedBreakpoint    = 0x40
    StoppedWatchpoint    = 0x42
    StoppedProgrammeReq  = 0x44
    StoppedHardwareReq   = 0x45
    StoppedUserReq       = 0x46
    HandlingEcall        = 0x47
    Running              = 0x80


# RISC-V register ABI names (x0–x31 then pc at index 32)
REG_NAMES = [
    "zero","ra","sp","gp","tp",
    "t0","t1","t2",
    "s0/fp","s1",
    "a0","a1","a2","a3","a4","a5","a6","a7",
    "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
    "t3","t4","t5","t6",
    "pc"
]


@dataclass
class StatusResult:
    status_raw:       int
    status_name:      str
    priv_mode:        int
    steps_remaining:  int
    steps_since_reset: int

    def __str__(self):
        priv = {0:"User", 1:"Supervisor", 3:"Machine"}.get(self.priv_mode, f"?{self.priv_mode}")
        return (
            f"Status          : {self.status_name} (0x{self.status_raw:02X})\n"
            f"Privilege mode  : {priv}\n"
            f"Steps remaining : {self.steps_remaining}\n"
            f"Steps since rst : {self.steps_since_reset}"
        )


# Core class
class BoardComms:
    """Low-level and high-level interface to the RISC-V debug board."""

    def __init__(self, port: str, baud: int = BAUD_RATE, timeout: float = 2.0):
        self._port   = port
        self._baud   = baud
        self._timeout = timeout
        self._ser: Optional[serial.Serial] = None

    # ---- context manager --------------------------------------------------

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, *_):
        self.close()

    def open(self):
        self._ser = serial.Serial(
            port     = self._port,
            baudrate = self._baud,
            bytesize = serial.EIGHTBITS,
            parity   = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            timeout  = self._timeout,
            xonxoff  = False,
            rtscts   = False,
        )
        # Small drain – discard any stale bytes
        time.sleep(0.05)
        self._ser.reset_input_buffer()

    def close(self):
        if self._ser and self._ser.is_open:
            self._ser.close()

    # ---- primitives -------------------------------------------------------

    def _w8(self, v: int):
        self._ser.write(bytes([v & 0xFF]))

    def _w16(self, v: int):
        self._ser.write(struct.pack("<H", v & 0xFFFF))

    def _w32(self, v: int):
        self._ser.write(struct.pack("<I", v & 0xFFFF_FFFF))

    def _r8(self) -> int:
        d = self._ser.read(1)
        if not d:
            raise TimeoutError("Timed out waiting for byte")
        return d[0]

    def _r16(self) -> int:
        d = self._ser.read(2)
        if len(d) < 2:
            raise TimeoutError("Timed out waiting for u16")
        return struct.unpack("<H", d)[0]

    def _r32(self) -> int:
        d = self._ser.read(4)
        if len(d) < 4:
            raise TimeoutError("Timed out waiting for u32")
        return struct.unpack("<I", d)[0]

    # ---- high-level API ---------------------------------------------------

    def uart_test(self) -> bool:
        """Quick UART loopback integrity check."""
        self._w8(CMD.SystemUartTest)
        ok  = self._r8()  == ord('!')
        ok &= self._r16() == 0xF00D
        ok &= self._r32() == 0xDEAD_BEEF
        return ok

    def echo(self, byte: int) -> int:
        """Echo a single byte; returns the echoed value."""
        self._w8(CMD.SystemEcho)
        self._w8(byte)
        return self._r8()

    # -- processor control --

    def reset(self, periph: bool = False) -> bool:
        """Reset the processor (and optionally peripherals)."""
        self._w8(CMD.Reset)
        assert self._r8() == 0xFF, "Reset ACK failed"
        if periph:
            self._w8(CMD.ResetPeriph)
            assert self._r8() == 0xFF, "Periph reset ACK failed"
        return True

    def run(self) -> bool:
        """Free-run the processor."""
        self._w8(CMD.Run)
        self._w32(0)          # 0 = free run
        return self._r8() == CMD.Run

    def stop(self) -> bool:
        """Halt the processor."""
        self._w8(CMD.Stop)
        return self._r8() == CMD.Stop

    def step(self, count: int = 1) -> bool:
        """Step N instructions."""
        self._w8(CMD.Run)
        self._w32(count)
        return self._r8() == CMD.Run

    def status(self) -> StatusResult:
        """Return current processor status."""
        self._w8(CMD.WhatIsExecuting)
        raw      = self._r8()
        priv     = self._r8()
        rem      = self._r32()
        since    = self._r32()
        try:
            name = ProcStatus(raw).name
        except ValueError:
            name = f"Unknown(0x{raw:02X})"
        return StatusResult(raw, name, priv, rem, since)

    # -- memory / registers --

    def read_memory(self, address: int, count: int = 1) -> List[int]:
        """Read `count` 32-bit words from memory starting at `address`."""
        results = []
        self._w8(CMD.MemRead32)
        self._w32(address)
        self._w32(count)
        for _ in range(count):
            results.append(self._r32())
        return results

    def write_memory(self, address: int, value: int):
        """Write a single 32-bit word to memory."""
        self._w8(CMD.MemWrite32)
        self._w32(address)
        self._w32(value)

    def load_memory(self, base_address: int, words: List[int]):
        """Bulk-write a list of 32-bit words into memory."""
        for i, word in enumerate(words):
            self.write_memory(base_address + i * 4, word)

    def read_register(self, reg: int) -> int:
        """Read a single RISC-V register (0-31 = GPRs, 32 = PC)."""
        self._w8(CMD.RegRead32)
        self._w32(reg)
        self._w32(1)
        return self._r32()

    def read_registers(self, count: int = 33) -> List[int]:
        """Read all GPRs + PC (registers 0..32 by default)."""
        self._w8(CMD.RegRead32)
        self._w32(0)
        self._w32(count)
        return [self._r32() for _ in range(count)]

    def read_pc(self) -> int:
        return self.read_register(32)

    def read_csr(self, csr_addr: int) -> int:
        """Read a CSR by its 12-bit address."""
        self._w8(CMD.CSRRead32)
        self._w32(csr_addr)
        self._w32(1)
        return self._r32()

    # -- breakpoints --

    def list_breakpoints(self) -> List[tuple]:
        """Return list of (id, address) tuples."""
        self._w8(CMD.BreakPointsRead)
        count = self._r8()
        return [(self._r8(), self._r32()) for _ in range(count)]

    def set_breakpoint(self, bp_id: int, address: int):
        self._w8(CMD.BreakPointsSet)
        self._w8(bp_id)
        self._w32(address)

    # -- system registers (io module) --

    def system_read(self, address: int) -> int:
        self._w8(CMD.SystemRead)
        self._w16(address)
        return self._r32()

    def system_write(self, address: int, value: int):
        self._w8(CMD.SystemWrite)
        self._w16(address)
        self._w32(value)

    def get_features(self) -> dict:
        self._w8(CMD.SystemFeatures)
        count = self._r16()
        return {self._r16(): self._r32() for _ in range(count)}

    # -- memory dump (pretty-print) --

    def memory_dump(self, base_address: int, num_words: int = 64):
        """Read and pretty-print a region of memory."""
        words = self.read_memory(base_address, num_words)
        print(f"\nMemory dump  0x{base_address:08X}  ({num_words} words)\n")
        for i, w in enumerate(words):
            addr = base_address + i * 4
            bar  = i % 4
            sep  = "  " if bar else "\n" if i else ""
            if bar == 0:
                print(f"  {addr:08X}:  ", end="")
            print(f"{w:08X}", end="  ")
        print()

    # -- profiling --

    def profile(self, duration_s: float = 1.0, sample_interval_s: float = 0.01) -> dict:
        """
        Sample the PC repeatedly while the processor runs.
        Returns a Counter {pc_value: hit_count} and prints a summary.
        """
        print(f"Profiling for {duration_s}s  (interval {sample_interval_s*1000:.0f}ms) …")
        self.run()
        pc_counts: Counter = Counter()
        end = time.monotonic() + duration_s
        samples = 0
        while time.monotonic() < end:
            try:
                pc = self.read_pc()
                pc_counts[pc] += 1
                samples += 1
            except TimeoutError:
                pass
            time.sleep(sample_interval_s)
        self.stop()

        print(f"\nProfile complete – {samples} samples\n")
        print(f"  {'PC':>10}   {'Hits':>6}   {'%':>6}")
        print("  " + "-" * 30)
        for pc, hits in pc_counts.most_common(20):
            pct = 100 * hits / samples if samples else 0
            print(f"  0x{pc:08X}   {hits:>6}   {pct:5.1f}%")
        return dict(pc_counts)


# CLI

def _print_regs(regs: List[int]):
    print("\n  RISC-V Registers\n")
    for i, v in enumerate(regs):
        name = REG_NAMES[i] if i < len(REG_NAMES) else f"x{i}"
        print(f"  x{i:<2} ({name:<6}) = 0x{v:08X}  ({v})")
    print()


def main():
    p = argparse.ArgumentParser(description="RISC-V board communication tool")
    p.add_argument("--port", default="/dev/ttyUSB0", help="Serial port")
    p.add_argument("--baud", type=int, default=BAUD_RATE)
    sub = p.add_subparsers(dest="cmd", required=True)

    sub.add_parser("status",   help="Show processor status")
    sub.add_parser("run",      help="Free-run the processor")
    sub.add_parser("stop",     help="Halt the processor")
    r = sub.add_parser("reset", help="Reset processor")
    r.add_argument("--periph", action="store_true", help="Also reset peripherals")
    st = sub.add_parser("step", help="Step N instructions")
    st.add_argument("n", nargs="?", type=int, default=1)
    sub.add_parser("regs",    help="Dump all registers")
    sub.add_parser("uarttest", help="UART integrity test")
    ec = sub.add_parser("echo", help="Echo a byte")
    ec.add_argument("byte", type=lambda x: int(x, 0))

    dm = sub.add_parser("dump", help="Hex dump memory region")
    dm.add_argument("addr",  type=lambda x: int(x, 0))
    dm.add_argument("words", type=int, default=64, nargs="?")

    rm = sub.add_parser("readmem",  help="Read one 32-bit word")
    rm.add_argument("addr", type=lambda x: int(x, 0))
    wm = sub.add_parser("writemem", help="Write one 32-bit word")
    wm.add_argument("addr",  type=lambda x: int(x, 0))
    wm.add_argument("value", type=lambda x: int(x, 0))

    rc = sub.add_parser("readcsr", help="Read a CSR")
    rc.add_argument("addr", type=lambda x: int(x, 0))

    bp = sub.add_parser("breakpoint", help="Breakpoint commands")
    bpsub = bp.add_subparsers(dest="bpcmd", required=True)
    bpsub.add_parser("list")
    bpset = bpsub.add_parser("set")
    bpset.add_argument("id",   type=int)
    bpset.add_argument("addr", type=lambda x: int(x, 0))

    pf = sub.add_parser("profile", help="Sample PC to profile execution")
    pf.add_argument("seconds", type=float, default=2.0, nargs="?")
    pf.add_argument("--interval", type=float, default=0.01)

    sub.add_parser("features", help="List board features")

    args = p.parse_args()

    with BoardComms(args.port, args.baud) as b:
        if args.cmd == "status":
            print(b.status())

        elif args.cmd == "run":
            ok = b.run()
            print("Running" if ok else "Run command failed")

        elif args.cmd == "stop":
            ok = b.stop()
            print("Stopped" if ok else "Stop command failed")

        elif args.cmd == "reset":
            b.reset(periph=args.periph)
            print("Reset OK")

        elif args.cmd == "step":
            ok = b.step(args.n)
            print(f"Stepped {args.n}" if ok else "Step failed")

        elif args.cmd == "regs":
            _print_regs(b.read_registers())

        elif args.cmd == "uarttest":
            ok = b.uart_test()
            print("UART test PASSED" if ok else "UART test FAILED")

        elif args.cmd == "echo":
            ret = b.echo(args.byte)
            print(f"Sent 0x{args.byte:02X}  →  received 0x{ret:02X}"
                  + ("  ✓" if ret == args.byte else "  ✗ MISMATCH"))

        elif args.cmd == "dump":
            b.memory_dump(args.addr, args.words)

        elif args.cmd == "readmem":
            v = b.read_memory(args.addr)[0]
            print(f"[0x{args.addr:08X}] = 0x{v:08X}  ({v})")

        elif args.cmd == "writemem":
            b.write_memory(args.addr, args.value)
            print(f"[0x{args.addr:08X}] ← 0x{args.value:08X}")

        elif args.cmd == "readcsr":
            v = b.read_csr(args.addr)
            print(f"CSR[0x{args.addr:03X}] = 0x{v:08X}  ({v})")

        elif args.cmd == "breakpoint":
            if args.bpcmd == "list":
                bps = b.list_breakpoints()
                if not bps:
                    print("No breakpoints set")
                for bid, addr in bps:
                    print(f"  BP {bid}: 0x{addr:08X}")
            elif args.bpcmd == "set":
                b.set_breakpoint(args.id, args.addr)
                print(f"Breakpoint {args.id} set at 0x{args.addr:08X}")

        elif args.cmd == "profile":
            b.profile(args.seconds, args.interval)

        elif args.cmd == "features":
            feats = b.get_features()
            for k, v in feats.items():
                print(f"  0x{k:04X} = {v}")


if __name__ == "__main__":
    main()
