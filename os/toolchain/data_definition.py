from dataclasses import dataclass

@dataclass
class DataDefinition:
    name:      str
    size:      int
    alignment: int  = 4
    init_val:  int  = 0       # 0 = zero-initialised (BSS)
    exported:  bool = False   # True if .globl was seen for this symbol
    line_no:   int  = 0

    def __str__(self):
        lines = []
        lines.append(f"{self.name}:")
        lines.append(f"    defs {self.size}, {self.init_val}")
        return "\n".join(lines)
