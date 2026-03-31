from dataclasses import dataclass, field
from enum import Enum, auto


class DirectiveKind(Enum):
    ALIGN   = auto()
    ZERO    = auto()
    COMM    = auto()    # .comm name, size, align
    LOCAL   = auto()    # .local name  (marks symbol as non-exported)
    GLOBL   = auto()
    SIZE    = auto()
    TYPE    = auto()
    SECTION = auto()
    TEXT    = auto()
    BSS     = auto()
    DATA    = auto()
    IDENT   = auto()
    FILE    = auto()
    OPTION  = auto()
    ATTRIBUTE = auto()
    SET = auto()
    UNKNOWN = auto()



# Directives we simply drop — they carry no semantic weight for our assembler
METADATA_DIRECTIVES = {
    DirectiveKind.FILE, DirectiveKind.OPTION, DirectiveKind.ATTRIBUTE,
    DirectiveKind.TYPE, DirectiveKind.SIZE,   DirectiveKind.IDENT,
    DirectiveKind.SECTION, DirectiveKind.TEXT, DirectiveKind.BSS,
    DirectiveKind.DATA,    DirectiveKind.GLOBL, DirectiveKind.SET,
}


@dataclass
class Directive:
    kind:    DirectiveKind
    args:    list[str] = field(default_factory=list)
    raw:     str = ""
    line_no: int = 0

    def __str__(self):
        if self.kind == DirectiveKind.ALIGN:
            return f"ALIGN {2**int(self.args[0])}"
        if self.args:
            return f"    {self.kind.name.lower()} " + ", ".join(self.args)
        return f"    {self.kind.name.lower()}"


