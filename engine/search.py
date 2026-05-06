import chess
from . import evaluation

def find_best_move(board: chess.Board, depth: int) -> chess.Move | None:
    if depth == 0 or board.is_game_over():
        return None

    maxplayer = True if board.turn == chess.WHITE else False
    if maxplayer:
        best_score = float("-inf")
    else:
        best_score = float("inf")
    best_move = None

    for move in board.legal_moves:
        board.push(move)
        score = alphabeta(board, depth - 1, float("-inf"), float("inf"))
        board.pop()
        if maxplayer:
            if score > best_score:
                best_score = score
                best_move = move
        else:
            if score < best_score:
                best_score = score
                best_move = move

    return best_move

def alphabeta(board: chess.Board, depth: int, alpha, beta) -> int:
    if depth == 0 or board.is_game_over():
        return evaluation.evaluate_board(board)
    maxPlayer = True if board.turn == chess.WHITE else False

    if maxPlayer: #White
        best_score = float("-inf")
        for move in board.legal_moves:
            board.push(move)
            score = alphabeta(board, depth-1, alpha, beta)
            board.pop()
            if score > best_score:
                best_score = score
            if score > alpha:
                alpha = score
            if alpha >= beta:
                break

        return best_score

    else: #Black
        best_score = float("inf")
        for move in board.legal_moves:
            board.push(move)
            score = alphabeta(board, depth - 1, alpha, beta)
            board.pop()
            if score < best_score:
                best_score = score
            if score < beta:
                beta = score
            if alpha >= beta:
                break

        return best_score