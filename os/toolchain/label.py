from dataclasses import dataclass


@dataclass
class Label:
    name:           str
    is_local:       bool        # True when original name starts with .L
    resolved_name:  str = ""    # filled in during the local-label resolution pass
    line_no:        int = 0

    def __post_init__(self):
        if not self.resolved_name:
            self.resolved_name = self.name

    def __str__(self):
        return f"{self.resolved_name}:"
