import chess
from . import evaluation

def find_best_move(board: chess.Board, depth: int) -> chess.Move | None:
    if depth == 0 or board.is_game_over():
        return None
    legal_moves = list(board.legal_moves)
    best_score = float("-inf")
    best_move = None
    for move in legal_moves:
        board.push(move)
        score = - negamax(board, depth - 1)
        if score > best_score:
            best_score = score
            best_move = move
        board.pop()
    return best_move

def negamax(board: chess.Board, depth: int) -> int:
    #the function returns the side-to-move eval
    if depth == 0 or board.is_game_over():
        eval = evaluation.evaluate_board(board)
        return eval if board.turn == chess.WHITE else -eval
    legal_moves = list(board.legal_moves)
    best_score = float("-inf")
    for move in legal_moves:
        board.push(move)
        score = - negamax(board, depth - 1)
        best_score = max(best_score, score)
        board.pop()
    return best_score
