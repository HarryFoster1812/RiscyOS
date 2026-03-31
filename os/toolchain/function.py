
from dataclasses import dataclass, field
import logging

from label import Label

from instruction import Instruction

from operand import OperandKind


@dataclass
class Function:
    name:     str
    exported: bool = False
    body:     list = field(default_factory=list)   # List[Instruction | Label]
    line_no:  int  = 0

    # ---- helpers -----------------------------------------------------------

    def local_labels(self) -> list[Label]:
        return [n for n in self.body if isinstance(n, Label) and n.is_local]

    def instructions(self) -> list[Instruction]:
        return [n for n in self.body if isinstance(n, Instruction)]

    def resolve_local_labels(self):
        """
        Rename every .Lx label (and every reference to it) to
        <function_name>__<original> so the outer assembler sees only
        globally unique, flat names.

        Example:  .L4  inside  fs_init  →  fs_init__L4
        """
        mapping: dict[str, str] = {}

        for node in self.body:
            if isinstance(node, Label) and node.is_local:
                new_name = f"{self.name}__{node.name.lstrip('.')}"
                mapping[node.name] = new_name
                node.resolved_name = new_name
                logging.info(f"  label  {node.name} → {new_name}")

        # Patch every SYMBOL operand that refers to a local label
        for instr in self.instructions():
            for op in instr.operands:
                if op.kind == OperandKind.SYMBOL and op.symbol in mapping:
                    op.symbol = mapping[op.symbol]

    def __str__(self):
        lines = []
        lines.append(f"{self.name}:")
        for node in self.body:
            lines.append(str(node))
        return "\n".join(lines)

