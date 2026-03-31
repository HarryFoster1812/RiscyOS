from dataclasses import dataclass, field
from typing import Optional

from operand import Operand


@dataclass
class Instruction:
    op:       str
    operands: list[Operand] = field(default_factory=list)
    raw:      str = ""
    line_no:  int = 0

    def __str__(self):
        if self.operands:
            return f"    {self.op} " + ", ".join(str(o) for o in self.operands)
        return f"    {self.op}"

    # Convenience accessors (keeps transformation code readable)
    def dest(self) -> Optional[Operand]:
        return self.operands[0] if self.operands else None

    def src(self, n: int = 1) -> Optional[Operand]:
        return self.operands[n] if n < len(self.operands) else None
