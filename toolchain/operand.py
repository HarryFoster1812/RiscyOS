import re
from dataclasses import dataclass
from typing import Optional
from enum import Enum, auto



class OperandKind(Enum):
    REGISTER   = auto()   # a5, sp, ra, s0 …
    IMMEDIATE  = auto()   # 16, -36, 0xff …
    MEMORY     = auto()   # 12(sp), 0(a5) …
    SYMBOL     = auto()   # fat_fs_info, .L4 …
    HI         = auto()   # %hi(symbol)
    LO         = auto()   # %lo(symbol)
    LO_MEM     = auto()   # %lo(symbol)(reg)  — before fix-up
    RESOLVED_MEM = auto() # symbol(reg)       — after fix-up


# ---------------------------------------------------------------------------
# Operand  (typed, so every transformation knows what it's touching)
# ---------------------------------------------------------------------------

@dataclass
class Operand:
    kind:  OperandKind
    raw:   str                       # original text, always preserved

    # populated depending on kind
    reg:    Optional[str] = None     # REGISTER, MEMORY, LO_MEM, RESOLVED_MEM
    imm:    Optional[int] = None     # IMMEDIATE, MEMORY offset
    symbol: Optional[str] = None     # SYMBOL, HI, LO, LO_MEM, RESOLVED_MEM

    def __str__(self):
        """Re-serialise to assembly text."""
        if self.kind == OperandKind.REGISTER:
            return self.reg
        if self.kind == OperandKind.IMMEDIATE:
            return str(self.imm) if self.imm is not None else self.raw
        if self.kind == OperandKind.MEMORY:
            offset = self.imm if self.imm is not None else 0
            return f"{offset}[{self.reg}]"
        if self.kind == OperandKind.SYMBOL:
            return self.symbol
        if self.kind == OperandKind.HI:
            return f"{self.symbol}"
        if self.kind == OperandKind.LO:
            return f"{self.symbol}"
        if self.kind == OperandKind.LO_MEM:
            return f"[{self.reg}]"
        if self.kind == OperandKind.RESOLVED_MEM:
            return f"{self.symbol}[{self.reg}]"
        return self.raw


def parse_operand(text: str) -> Operand:
    """Turn a single operand string into a typed Operand."""
    t = text.strip()

    # %hi(symbol)
    m = re.fullmatch(r'%hi\(([^)]+)\)', t)
    if m:
        return Operand(OperandKind.HI, t, symbol=m.group(1))

    # %lo(symbol)(reg)
    m = re.fullmatch(r'%lo\(([^)]+)\)\((\w+)\)', t)
    if m:
        return Operand(OperandKind.LO_MEM, t, symbol=m.group(1), reg=m.group(2))

    # %lo(symbol)
    m = re.fullmatch(r'%lo\(([^)]+)\)', t)
    if m:
        return Operand(OperandKind.LO, t, symbol=m.group(1))

    # offset(reg)  — decimal or hex, optionally signed
    m = re.fullmatch(r'(-?(?:0x[0-9a-fA-F]+|\d+))\((\w+)\)', t)
    if m:
        raw_imm = m.group(1)
        imm = int(raw_imm, 16) if raw_imm.startswith('0x') else int(raw_imm)
        return Operand(OperandKind.MEMORY, t, reg=m.group(2), imm=imm)

    # immediate  (decimal or hex, optionally signed, no letters after)
    m = re.fullmatch(r'-?(?:0x[0-9a-fA-F]+|\d+)', t)
    if m:
        imm = int(t, 16) if '0x' in t else int(t)
        return Operand(OperandKind.IMMEDIATE, t, imm=imm)

    # register  (standard RISC-V ABI names)
    if re.fullmatch(r'zero|ra|sp|gp|tp|fp|[sgtf]p|'
                    r'a\d|s\d{1,2}|t\d|x\d{1,2}', t):
        return Operand(OperandKind.REGISTER, t, reg=t)

    # fallback: treat as a symbolic reference (label, global name …)
    return Operand(OperandKind.SYMBOL, t, symbol=t)
