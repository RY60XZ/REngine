import unittest
import chess
from engine import *
from engine.evaluation import evaluate_board

def main():
    board = chess.Board()
    assert(evaluate_board(board) == 0)
    board.push_san("e4")
    assert(evaluate_board(board) == 0)
    board.push_san("e5")
    assert(evaluate_board(board) == 0)
    board.push_san("Bc4")
    assert(evaluate_board(board) == 0)
    board.push_san("Bc5")
    assert(evaluate_board(board) == 0)
    board.push_san("Qh5")
    assert(evaluate_board(board) == 0)
    board.push_san("Nf6")
    assert(evaluate_board(board) == 0)
    board.push_san("Qf7")
    assert(evaluate_board(board) == 999999)
    board = chess.Board()
    board.push_san("f3")
    board.push_san("e5")
    board.push_san("g4")
    board.push_san("Qh4")
    assert(evaluate_board(board) == -999999)
    board = chess.Board("rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2")
    assert(evaluate_board(board) == 100)
    board.push_san("Qd5")
    assert(evaluate_board(board) == 0)

if __name__ == '__main__':
    main()
