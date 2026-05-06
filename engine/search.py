import chess
from . import evaluation

PIECE_VALUES = {
    chess.PAWN: 100,
    chess.KNIGHT: 320,
    chess.BISHOP: 330,
    chess.ROOK: 500,
    chess.QUEEN: 900,
    chess.KING: 1000,
}

def find_best_move(board: chess.Board, depth: int) -> chess.Move | None:
    if depth == 0 or board.is_game_over():
        return None

    maxplayer = True if board.turn == chess.WHITE else False
    alpha = float("-inf")
    beta = float("inf")
    if maxplayer:
        best_score = float("-inf")
    else:
        best_score = float("inf")
    best_move = None

    for move in order_moves(board):
        board.push(move)
        score = alphabeta(board, depth - 1, alpha, beta)
        board.pop()
        if maxplayer:
            if score > best_score:
                best_score = score
                best_move = move
            alpha = max(alpha, best_score)
        else:
            if score < best_score:
                best_score = score
                best_move = move
            beta = min(beta, best_score)

    return best_move

def alphabeta(board: chess.Board, depth: int, alpha, beta) -> int:
    if depth == 0:
        return evaluation.evaluate_board(board)
    maxPlayer = True if board.turn == chess.WHITE else False

    if maxPlayer: #White
        best_score = float("-inf")
        has_move = False
        for move in order_moves(board):
            has_move = True
            board.push(move)
            score = alphabeta(board, depth-1, alpha, beta)
            board.pop()
            if score > best_score:
                best_score = score
            if score > alpha:
                alpha = score
            if alpha >= beta:
                break

        if not has_move:
            return -999999 if board.is_check() else 0
        return best_score

    else: #Black
        best_score = float("inf")
        has_move = False
        for move in order_moves(board):
            has_move = True
            board.push(move)
            score = alphabeta(board, depth - 1, alpha, beta)
            board.pop()
            if score < best_score:
                best_score = score
            if score < beta:
                beta = score
            if alpha >= beta:
                break

        if not has_move:
            return 999999 if board.is_check() else 0
        return best_score

def order_moves(board: chess.Board):
    return sorted(board.legal_moves, key=lambda move: score_move(board, move), reverse=True)

def score_move(board: chess.Board, move: chess.Move) -> int:
    is_promotion = False
    promotion_piece = move.promotion
    if promotion_piece is not None:
        is_promotion = True
    is_check = board.gives_check(move)
    is_capture = board.is_capture(move)
    aggressor = None
    victim = None
    victim_square = None
    if is_capture:
        aggressor = board.piece_at(move.from_square)
        if board.is_en_passant(move):
            # En passant victim is not on move.to_square.
            victim_square = chess.square(
                chess.square_file(move.to_square),
                chess.square_rank(move.from_square),
            )
            victim = board.piece_at(victim_square)
        else:
            victim_square = move.to_square
            victim = board.piece_at(move.to_square)

    score = 0
    if is_promotion:
        score += PIECE_VALUES[promotion_piece] * 10
    if is_check:
        score += 1000
    if is_capture:
        score += PIECE_VALUES[victim.piece_type] * 10 + (PIECE_VALUES[chess.KING]-PIECE_VALUES[aggressor.piece_type])
    return score
