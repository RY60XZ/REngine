from dataclasses import dataclass
from enum import Enum
import chess

class TTFlag(Enum):
    EXACT = "EXACT"
    LOWER = "LOWER"
    UPPER = "UPPER"

@dataclass
class Entry:
    depth: int
    eval: int | float
    best_move: chess.Move | None
    flag: TTFlag

class TranspositionTable:
    MAX_SIZE = 1_000_000
    def __init__(self):
        self.table = {}

    def insert(self, key, entry: Entry) -> None:
        if key not in self.table:
            self.table[key] = entry
        else:
            if entry.depth >= self.table[key].depth:
                self.table[key] = entry
        if len(self.table) >= TranspositionTable.MAX_SIZE:
            self.table.clear()

    def get(self, key) -> Entry | None:
        return self.table.get(key)
