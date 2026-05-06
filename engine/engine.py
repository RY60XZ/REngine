import random

import chess

class BaseEngine:
    def __init__(self, depth: int):
        self.depth = depth
    def make_move(self, board: chess.Board):
        pass

class RandomEngine(BaseEngine):
    def make_move(self, board: chess.Board):
        legal_moves = list(board.legal_moves)
        if not legal_moves:
            return None
        return random.choice(legal_moves)

class ClassicalEngine(BaseEngine):
    def make_move(self, board: chess.Board):
        pass