import chess

PIECE_VALUES = {
    chess.PAWN: 100,
    chess.KNIGHT: 320,
    chess.BISHOP: 330,
    chess.ROOK: 500,
    chess.QUEEN: 900,
    chess.KING: 0,
}

class Evaluator:
    def __init__(self):
        pass

    def evaluate_board(self, board: chess.Board):
        raise NotImplementedError()

class MaterialEvaluator(Evaluator):
    def evaluate_board(self, board: chess.Board) -> int:
        # eval is always from White's perspective
        if board.is_game_over():
            winner = board.outcome().winner
            if winner == chess.WHITE:
                return 999999 - board.halfmove_clock * 100
            elif winner == chess.BLACK:
                return -999999 + board.halfmove_clock * 100
            return 0

        return self.evaluate_material(board)

    def evaluate_material(self, board: chess.Board) -> int:
        score = 0
        for piece_type, value in PIECE_VALUES.items():
            white_count = board.pieces_mask(piece_type, chess.WHITE).bit_count()
            black_count = board.pieces_mask(piece_type, chess.BLACK).bit_count()
            score += value * (white_count - black_count)
        return score

_default_evaluator = MaterialEvaluator()

def evaluate_board(board: chess.Board) -> int:
    return _default_evaluator.evaluate_board(board)
