import random
import chess
from . import search

class BaseEngine:
    def __init__(self, depth: int):
        self.depth = depth
    def make_move(self, board: chess.Board) -> chess.Move | None:
        raise NotImplementedError

class RandomEngine(BaseEngine):
    def make_move(self, board: chess.Board) -> chess.Move | None:
        if board.is_game_over():
            return None
        legal_moves = list(board.legal_moves)
        return random.choice(legal_moves)

class ClassicalEngine(BaseEngine):
    def make_move(self, board: chess.Board) -> chess.Move | None:
        return search.find_best_move(board, depth=self.depth)
