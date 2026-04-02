import os
import re
import sys
import logging
from dataclasses import dataclass, field
from typing import Optional, Union
from enum import Enum, auto
from operand import *
from instruction import *
from label import *
from directive import *
from data_definition import *
from function import Function
from pprint import pprint

logging.basicConfig(level=logging.INFO, format='[%(levelname)s] %(message)s')

DEBUG = True 

if DEBUG:
    debug_dir = "debug"
    os.makedirs(debug_dir, exist_ok=True)

_DIRECTIVE_MAP = {
    ".align": DirectiveKind.ALIGN,
    ".zero":  DirectiveKind.ZERO,
    ".comm":  DirectiveKind.COMM,
    ".local": DirectiveKind.LOCAL,
    ".globl": DirectiveKind.GLOBL,
    ".size":  DirectiveKind.SIZE,
    ".type":  DirectiveKind.TYPE,
    ".section": DirectiveKind.SECTION,
    ".text":  DirectiveKind.TEXT,
    ".bss":   DirectiveKind.BSS,
    ".data":  DirectiveKind.DATA,
    ".ident": DirectiveKind.IDENT,
    ".file":  DirectiveKind.FILE,
    ".option": DirectiveKind.OPTION,
    ".attribute": DirectiveKind.ATTRIBUTE,
    ".set": DirectiveKind.SET,
    ".string": DirectiveKind.STRING,
}


LANCHOR_MAP = {}

class Section(Enum):
    TEXT = auto()
    BSS  = auto()
    DATA = auto()
    RODATA = auto()
    UNKNOWN = auto()

@dataclass
class Module:
    source_file: str = ""
    functions:   list[Function]        = field(default_factory=list)
    data_defs:   list[DataDefinition]  = field(default_factory=list)
    global_syms: set[str]              = field(default_factory=set)
    str_table:   dict[str, str]        = field(default_factory=dict)

    def get_function(self, name: str) -> Optional[Function]:
        return next((f for f in self.functions if f.name == name), None)

    def summary(self):
        print(f"Module: {self.source_file}")
        print(f"  {len(self.functions)} function(s): "
              + ", ".join(f.name for f in self.functions))
        print(f"  {len(self.data_defs)} data def(s): "
              + ", ".join(d.name for d in self.data_defs))
        print(f"  exported symbols: {self.global_syms}")

# Raw token  (internal to the parser only)

@dataclass
class RawLine:
    line_no: int
    raw:     str
    label:   Optional[str]      = None
    op:      Optional[str]      = None
    args:    list[str]          = field(default_factory=list)
    comment: Optional[str]      = None  


def tokenise(raw: str, line_no: int) -> RawLine:
    raw  = raw.replace("\t", " ")
    text = raw.strip()
    rl = RawLine(line_no=line_no, raw=raw)
    
    if not text:
        return rl

    if text.startswith('#'):
        rl.comment = text[1:]
        return rl


    if text.endswith(':'):
        rl.label = text[:-1]
        return rl

    if "#" in text:
        rl.comment = text[text.index("#")+1:]
        text = text[:text.index("#")]

    parts = re.split(r'\s+', text, maxsplit=1)
    rl.op = parts[0]
    if len(parts) > 1:
        rl.args = [a.strip() for a in parts[1].split(',')]
    return rl


# Parser

def parse(source: str, filename: str = "") -> Module:
    module = Module(source_file=filename)

    raw_lines = [tokenise(l, i + 1) for i, l in enumerate(source.splitlines())]

    current_section  = Section.UNKNOWN
    current_function: Optional[Function] = None
    pending_label:    Optional[str]      = None   # label seen before next instruction

    i = 0
    while i < len(raw_lines):
        rl = raw_lines[i]

        if not rl.label and not rl.op:
            i += 1
            continue

        if rl.label and not rl.op:
            name      = rl.label
            is_local  = name.startswith('.L')

            if current_function and (is_local and current_section == Section.TEXT):
                lbl = Label(name=name, is_local=is_local, line_no=rl.line_no)
                current_function.body.append(lbl)
            else:
                # It's a data-section label — hold it for the .zero/.comm
                # that follows, or open a new function.
                pending_label = name

            i += 1
            continue

        if rl.op and rl.op.startswith('.'):
            kind = _DIRECTIVE_MAP.get(rl.op, DirectiveKind.UNKNOWN)

            # section switches
            if kind == DirectiveKind.TEXT:
                current_section = Section.TEXT
                i += 1; continue
            if kind in (DirectiveKind.BSS, DirectiveKind.DATA):
                current_section = Section.BSS if kind == DirectiveKind.BSS else Section.DATA
                i += 1; continue
            
            if kind == DirectiveKind.SET:
                # calculate LANCHOR 
                i += 1; continue

            if kind == DirectiveKind.SECTION:
                if ".text" in rl.args[0]:
                    current_section = Section.TEXT
                    i += 1; continue

                elif ".rodata" in rl.args[0]:
                    current_section = Section.RODATA
                    i += 1; continue

            # track exported names
            if kind == DirectiveKind.GLOBL:
                sym = rl.args[0] if rl.args else ""
                module.global_syms.add(sym)
                i += 1; continue

            # .comm name, size, align  →  DataDefinition
            if kind == DirectiveKind.COMM:
                name, size_s, align_s = rl.args[0], rl.args[1], rl.args[2]
                dd = DataDefinition(
                    name      = name,
                    size      = int(size_s),
                    alignment = int(align_s),
                    exported  = name in module.global_syms,
                    line_no   = rl.line_no,
                )
                module.data_defs.append(dd)
                pending_label = None
                i += 1; continue

            # .zero size  (must follow a data-section label)
            if kind == DirectiveKind.ZERO and pending_label:
                dd = DataDefinition(
                    name      = pending_label,
                    size      = int(rl.args[0]),
                    alignment = 4,
                    exported  = pending_label in module.global_syms,
                    line_no   = rl.line_no,
                )
                module.data_defs.append(dd)
                pending_label = None
                i += 1; continue

            if kind == DirectiveKind.STRING:
                string = rl.args[0].replace("\"", "")
                remapped_label = string+"_"+"str_"+str(len(module.str_table))
                dd = DataDefinition(
                    name      = remapped_label,
                    size      = 0,
                    alignment = 4,
                    string    = string,
                    exported  = pending_label in module.global_syms,
                    line_no   = rl.line_no,
                )
                module.str_table[pending_label] = remapped_label
                module.data_defs.append(dd)
                pending_label = None
                i += 1; continue

            # .align inside a function body  →  keep as Directive node
            if kind == DirectiveKind.ALIGN and current_function:
                d = Directive(kind=kind, args=rl.args, raw=rl.raw, line_no=rl.line_no)
                current_function.body.append(d)
                i += 1; continue

            # .local  →  mark symbol as non-exported (just record it)
            if kind == DirectiveKind.LOCAL:
                i += 1; continue

            # everything else in METADATA_DIRECTIVES → drop silently
            if kind in METADATA_DIRECTIVES:
                i += 1; continue

            # unknown directive inside a function → keep for safety
            if current_function:
                d = Directive(kind=DirectiveKind.UNKNOWN, args=rl.args,
                              raw=rl.raw, line_no=rl.line_no)
                current_function.body.append(d)
            i += 1
            continue

        if rl.op:
            # A pending_label that reaches an instruction means this is a
            # function entry point.
            if pending_label and current_section == Section.TEXT:
                fn = Function(
                    name     = pending_label,
                    exported = pending_label in module.global_syms,
                    line_no  = rl.line_no,
                )
                module.functions.append(fn)
                current_function = fn
                pending_label    = None
                logging.info(f"Opened function: {fn.name}")

            instr = Instruction(
                op       = rl.op,
                operands = [parse_operand(a) for a in rl.args],
                raw      = rl.raw,
                line_no  = rl.line_no,
            )

            if current_function:
                current_function.body.append(instr)
            else:
                logging.warning(f"Line {rl.line_no}: instruction outside any function: {rl.raw.strip()}")

        i += 1

    # Mark exported functions
    for fn in module.functions:
        fn.exported = fn.name in module.global_syms
    
    return module

def pass_fill_LANCHOR_MAP(module: Module):
    global LANCHOR_MAP
    LANCHOR_MAP = {}

    logging.info("Building .LANCHOR map...")

    # read the original source lines
    with open(module.source_file) as f:
        lines = f.readlines()

    current_var = None
    for i, line in enumerate(lines):
        line_s = line.strip()
        # Track current variable (globl or pending label)
        m_glob = re.match(r"\.globl\s+(\w+)", line_s)
        if m_glob:
            current_var = m_glob.group(1)
            continue

        # Track pending label (for .zero/.comm)
        m_label = re.match(r"(\w+):", line_s)
        if m_label:
            current_var = m_label.group(1)
            continue

        # Parse .set directives
        m_set = re.match(r"\.set\s+(\.LANCHOR\d+),\s*\. \+ (\d+)", line_s)
        if m_set and current_var:
            anchor, offset = m_set.groups()
            LANCHOR_MAP[anchor] = (current_var, int(offset))
            logging.info(f"  LANCHOR {anchor} -> {current_var} + {offset}")

    logging.info(f"Finished building .LANCHOR map: {LANCHOR_MAP}")

# Pass: resolve local labels per function
def pass_resolve_local_labels(module: Module):
    for fn in module.functions:
        logging.info(f"Resolving local labels in {fn.name}")
        fn.resolve_local_labels()

# Pass: collapse lui + addi(%hi/%lo pair) → la
def pass_collapse_la(module: Module):
    for fn in module.functions:
        body = fn.body
        out  = []
        i    = 0
        LA_MAPPED = {}
        while i < len(body):
            node = body[i]
            if (isinstance(node, Instruction) and node.op == "lui" and i + 1 < len(body)):
                # print("LA PASS")
                # pprint(node)
                # pprint(body[i+1])
                # print("\n\n\n")
                d1 = node.dest()
                hi = node.src(1)
                LANCHOR_ENTRY = None
                if hi and str(hi).startswith(".LANCHOR"):
                    LANCHOR_ENTRY =  LANCHOR_MAP[hi.symbol]
                    if LANCHOR_ENTRY:
                        LA_MAPPED[hi.symbol] = d1
                        hi.symbol = LANCHOR_ENTRY[0]
                    else:
                        hi.symbol = "UNKNOWN"

                # if not a LANCHOR the maybe a str
                elif hi and str(hi).startswith("."):
                    str_remapped = module.str_table.get(hi.symbol)
                    if not str_remapped:
                        hi.symbol = "UNKNOWN"
                    else:
                        LA_MAPPED[hi.symbol] = d1
                        hi.symbol = str_remapped

                la = Instruction(
                    op       = "la",
                    operands = [
                        Operand(OperandKind.REGISTER, d1.reg, reg=d1.reg),
                        Operand(OperandKind.SYMBOL,   hi.symbol, symbol=hi.symbol),
                    ],
                )

                logging.info(f"  {fn.name}: lui+addi → la {d1.reg}, {hi.symbol}")
                out.append(la)

                if LANCHOR_ENTRY and int(LANCHOR_ENTRY[1]) != 0:
                    addi_offset = Instruction(
                            op="addi",
                            operands=[
                                    Operand(OperandKind.REGISTER, "", reg=d1.reg),
                                    Operand(OperandKind.REGISTER, "", reg=d1.reg),
                                    Operand(OperandKind.IMMEDIATE, "", imm=int(LANCHOR_ENTRY[1])),
                                ],
                            )
                    out.append(addi_offset)
                    pass

                next_node = body[i+1]
                # print(f"isinstance(next_node, Instruction): {isinstance(next_node, Instruction)}")
                # print(f'next_node.op == \'addi\': {next_node.op == "addi"}')
                # print(f'next_node.dest() == d1: {next_node.dest() == d1}')
                # print(f'next_node.src(1) == d1: {next_node.src(1) == d1}')
                # print(f'next_node.src(2) == hi: {next_node.src(2) == hi}')
                # print(f'SRC2={repr(next_node.src(2))}, HI={repr(hi)}')
                if (isinstance(next_node, Instruction) and next_node.op == "addi" and 
                    next_node.dest() == d1 and next_node.src(1) == d1 and str(next_node.src(2)) == str(hi)):
                    # print("PASS CONDITION SKIPPING ADDI INST")
                    i += 2
                else:
                    i += 1
                continue
            if node and isinstance(node, Instruction) and node.op == "addi":
                if node.src(2).kind != OperandKind.REGISTER and node.src(2).kind != OperandKind.IMMEDIATE:
                    try:
                        dest_reg = LA_MAPPED.get(node.src(2).symbol)
                        if dest_reg.reg == node.dest().reg and dest_reg.reg == node.src(1).reg:
                            i+=1
                            continue
                    except:
                        pass
            
            out.append(node)
            i += 1

        fn.body = out

# Emit
def emit(module: Module) -> str:
    out = []

    for fn in module.functions:
        out.append(str(fn))
        out.append("")

    for dd in module.data_defs:
        out.append(str(dd))
        out.append("")

    return "\n".join(out)


# Pipeline
def transform(source: str, filename: str = "") -> str:
    module = parse(source, filename)
    module.summary()

    if DEBUG:
        with open(os.path.join(debug_dir, "0 - PARSE.s"), "w") as f:
            f.write(emit(module))

    pass_resolve_local_labels(module)

    if DEBUG:
        with open(os.path.join(debug_dir, "1 - RESOLVE LABEL.s"), "w") as f:
            f.write(emit(module))

    pass_fill_LANCHOR_MAP(module)

    pass_collapse_la(module)

    if DEBUG:
        with open(os.path.join(debug_dir, "2 - COLLAPSE.s"), "w") as f:
            f.write(emit(module))

    return emit(module)


# CLI
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("python convert.py <file>")
        sys.exit(1)

    path = sys.argv[1]
    with open(path) as f:
        source = f.read()

    result = transform(source, path)

    out_path = path.replace(".s", ".out.s")
    with open(out_path, "w") as f:
        f.write(result)

    logging.info(f"Written to {out_path}")
