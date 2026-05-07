import chess
from . import evaluation, transposition

PIECE_VALUES = {
    chess.PAWN: 100,
    chess.KNIGHT: 320,
    chess.BISHOP: 330,
    chess.ROOK: 500,
    chess.QUEEN: 900,
    chess.KING: 1000,
}

tt = transposition.TranspositionTable()
evaluator = evaluation.MaterialEvaluator()
QUIESCENCE_DEPTH = 6

def find_best_move_iterative(board: chess.Board, depth: int) -> chess.Move | None:
    best_move = None
    for i in range(1, depth+1):
        move = find_best_move(board, i)
        if move is not None:
            best_move = move
    return best_move

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

    key = board._transposition_key()
    entry = tt.get(key)
    if entry is not None and entry.depth >= depth:
        if entry.flag == transposition.TTFlag.EXACT:
            return entry.best_move
        if entry.flag == transposition.TTFlag.LOWER:
            alpha = max(alpha, entry.eval)
        elif entry.flag == transposition.TTFlag.UPPER:
            beta = min(beta, entry.eval)
        if alpha >= beta:
            return entry.best_move

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

    if best_move is not None:
        tt.insert(key, transposition.Entry(depth, best_score, best_move, transposition.TTFlag.EXACT))
    return best_move

def alphabeta(board: chess.Board, depth: int, alpha, beta) -> int:
    if depth == 0:
        return quiescence(board, alpha, beta, QUIESCENCE_DEPTH)
    maxPlayer = True if board.turn == chess.WHITE else False

    alpha_original = alpha
    beta_original = beta
    key = board._transposition_key()
    entry = tt.get(key)

    if entry is not None and entry.depth >= depth:
        if entry.flag == transposition.TTFlag.EXACT:
            return entry.eval
        if entry.flag == transposition.TTFlag.LOWER:
            alpha = max(alpha, entry.eval)
        elif entry.flag == transposition.TTFlag.UPPER:
            beta = min(beta, entry.eval)
        if alpha >= beta:
            return entry.eval

    if maxPlayer: #White
        best_score = float("-inf")
        best_move = None
        has_move = False
        for move in order_moves(board):
            has_move = True
            board.push(move)
            score = alphabeta(board, depth-1, alpha, beta)
            board.pop()
            if score > best_score:
                best_score = score
                best_move = move
            if score > alpha:
                alpha = score
            if alpha >= beta:
                break

        if not has_move:
            best_score = evaluator.evaluate_board(board)
            tt.insert(key, transposition.Entry(depth, best_score, best_move, transposition.TTFlag.EXACT))
            return best_score

        if best_score <= alpha_original:
            flag = transposition.TTFlag.UPPER
        elif best_score >= beta_original:
            flag = transposition.TTFlag.LOWER
        else:
            flag = transposition.TTFlag.EXACT
        tt.insert(key, transposition.Entry(depth, best_score, best_move, flag))
        return best_score

    else: #Black
        best_score = float("inf")
        best_move = None
        has_move = False
        for move in order_moves(board):
            has_move = True
            board.push(move)
            score = alphabeta(board, depth - 1, alpha, beta)
            board.pop()
            if score < best_score:
                best_score = score
                best_move = move
            if score < beta:
                beta = score
            if alpha >= beta:
                break

        if not has_move:
            best_score = evaluator.evaluate_board(board)
            tt.insert(key, transposition.Entry(depth, best_score, best_move, transposition.TTFlag.EXACT))
            return best_score

        if best_score <= alpha_original:
            flag = transposition.TTFlag.UPPER
        elif best_score >= beta_original:
            flag = transposition.TTFlag.LOWER
        else:
            flag = transposition.TTFlag.EXACT
        tt.insert(key, transposition.Entry(depth, best_score, best_move, flag))
        return best_score

def quiescence(board: chess.Board, alpha, beta, depth: int) -> int:
    if board.is_game_over():
        return evaluator.evaluate_board(board)
    if depth == 0:
        return evaluator.evaluate_material(board)

    maxPlayer = True if board.turn == chess.WHITE else False
    in_check = board.is_check()
    moves = select_chaotic_moves(board, in_check)

    if maxPlayer:
        stand_pat = float("-inf") if in_check else evaluator.evaluate_material(board)
        if stand_pat >= beta:
            return beta
        if stand_pat > alpha:
            alpha = stand_pat

        has_move = False
        for move in moves:
            has_move = True
            board.push(move)
            score = quiescence(board, alpha, beta, depth - 1)
            board.pop()
            if score >= beta:
                return beta
            if score > alpha:
                alpha = score

        if not has_move and in_check:
            return evaluator.evaluate_board(board)
        return alpha

    else:
        stand_pat = float("inf") if in_check else evaluator.evaluate_material(board)
        if stand_pat <= alpha:
            return alpha
        if stand_pat < beta:
            beta = stand_pat

        has_move = False
        for move in moves:
            has_move = True
            board.push(move)
            score = quiescence(board, alpha, beta, depth - 1)
            board.pop()
            if score <= alpha:
                return alpha
            if score < beta:
                beta = score

        if not has_move and in_check:
            return evaluator.evaluate_board(board)
        return beta

def order_moves(board: chess.Board):
    entry = tt.get(board._transposition_key())
    tt_move = entry.best_move if entry is not None else None

    tt_moves = []
    tactical_moves = []
    quiet_moves = []

    for move in board.legal_moves:
        if move == tt_move:
            tt_moves.append(move)
        elif move.promotion is not None or board.is_capture(move):
            tactical_moves.append((move, score_move(board, move)))
        else:
            quiet_moves.append(move)

    tactical_moves.sort(key=lambda item: item[1], reverse=True)
    return tt_moves + [move for move, score in tactical_moves] + quiet_moves

def select_chaotic_moves(board: chess.Board, in_check: bool | None = None):
    if in_check is None:
        in_check = board.is_check()

    if in_check:
        scored_moves = [(move, score_move(board, move)) for move in board.legal_moves]
        scored_moves.sort(key=lambda item: item[1], reverse=True)
        return [move for move, score in scored_moves]

    scored_moves = [(move, score_move(board, move)) for move in board.generate_legal_captures()]

    promotion_rank = chess.BB_RANK_7 if board.turn == chess.WHITE else chess.BB_RANK_2
    if board.pawns & board.occupied_co[board.turn] & promotion_rank:
        for move in board.legal_moves:
            if move.promotion is not None and not board.is_capture(move):
                scored_moves.append((move, score_move(board, move)))

    scored_moves.sort(key=lambda item: item[1], reverse=True)
    return [move for move, score in scored_moves]

def score_move(board: chess.Board, move: chess.Move, tt_move: chess.Move | None = None) -> int:
    if tt_move == move:
        return 10_000_000

    is_promotion = False
    promotion_piece = move.promotion
    if promotion_piece is not None:
        is_promotion = True
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
    if is_capture:
        score += PIECE_VALUES[victim.piece_type] * 10 + (PIECE_VALUES[chess.KING]-PIECE_VALUES[aggressor.piece_type])
    return score
