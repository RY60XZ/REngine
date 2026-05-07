import chess
from engine.engine import RandomEngine, ClassicalEngine

board = chess.Board()
engine = ClassicalEngine(6)

while not board.is_game_over():
    print(board.unicode())
    legal_move_flag = False
    while not legal_move_flag:
        player_move = str(input("Enter your move: "))
        try:
            move = board.parse_san(player_move)
            board.push(move)
            legal_move_flag = True
        except chess.InvalidMoveError:
            print("Invalid move. Try again.")
            continue
        except chess.IllegalMoveError:
            print("Illegal move. Try again.")
            continue
        except chess.AmbiguousMoveError:
            print("Ambiguous move. Try again.")
            continue
    if board.is_game_over():
        print("Game over.")
        break
    engine_move = engine.make_move(board)
    if engine_move is None:
        print("The engine can make no move. Game over.")
        break
    board.push(engine_move)
    print(f"engine_move: {engine_move}")



