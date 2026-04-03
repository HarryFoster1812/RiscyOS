from dataclasses import dataclass
from typing import Optional
from enum import Enum, auto

class DataDefinitionType(Enum):
    STRING   = auto()
    WORD     = auto()
    HALFWORD     = auto()
    DOUBLEWORD   = auto()
    SPACE     = auto()


@dataclass
class DataDefinition:
    name:      str
    size:      int
    alignment: int  = 4
    init_val:  int  = 0       # 0 = zero-initialised (BSS)
    exported:  bool = False   # True if .globl was seen for this symbol
    line_no:   int  = 0
    string:    Optional[str] = None

    def __str__(self):
        lines = []
        lines.append(f"{self.name}:")

        if self.string:
            lines.append(f"    defb \"{self.string}\\0\"")
            lines.append(f"ALIGN 4")
        else:
            lines.append(f"    defs {self.size}, {self.init_val}")
        return "\n".join(lines)
