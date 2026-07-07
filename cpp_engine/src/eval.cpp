#include"rengine/eval.h"
#include"rengine/square.h"
#include<bit>

namespace rengine {
    namespace {
        constexpr Score PIECE_VALUES[6] = {
            100,
            320,
            330,
            500,
            900,
            0,
        };

        constexpr Score PIECE_SQUARE_VALUES[6][64] = {
            {
                 0,   0,   0,   0,   0,   0,   0,   0,
                 5,  10,  10,  -5,  -5,  10,  10,   5,
                 5,  -5, -10,   0,   0, -10,  -5,   5,
                 0,   0,   0,  20,  20,   0,   0,   0,
                 5,   5,  10,  25,  25,  10,   5,   5,
                10,  10,  20,  30,  30,  20,  10,  10,
                50,  50,  50,  50,  50,  50,  50,  50,
                 0,   0,   0,   0,   0,   0,   0,   0,
            },
            {
               -50, -35, -30, -25, -25, -30, -35, -50,
               -35, -20,   0,   5,   5,   0, -20, -35,
               -30,   5,  10,  15,  15,  10,   5, -30,
               -25,   5,  15,  20,  20,  15,   5, -25,
               -25,   5,  15,  20,  20,  15,   5, -25,
               -30,   5,  10,  15,  15,  10,   5, -30,
               -35, -20,   0,   5,   5,   0, -20, -35,
               -50, -35, -30, -25, -25, -30, -35, -50,
            },
            {
               -20, -10, -10, -10, -10, -10, -10, -20,
               -10,   5,   0,   0,   0,   0,   5, -10,
               -10,  10,  10,  10,  10,  10,  10, -10,
               -10,   0,  10,  15,  15,  10,   0, -10,
               -10,   5,  10,  15,  15,  10,   5, -10,
               -10,   0,  10,  10,  10,  10,   0, -10,
               -10,   0,   0,   0,   0,   0,   0, -10,
               -20, -10, -10, -10, -10, -10, -10, -20,
            },
            {
                 0,   0,   5,  10,  10,   5,   0,   0,
                -5,   0,   0,   0,   0,   0,   0,  -5,
                -5,   0,   0,   0,   0,   0,   0,  -5,
                -5,   0,   0,   0,   0,   0,   0,  -5,
                -5,   0,   0,   0,   0,   0,   0,  -5,
                -5,   0,   0,   0,   0,   0,   0,  -5,
                 5,  10,  10,  10,  10,  10,  10,   5,
                 0,   0,   5,  10,  10,   5,   0,   0,
            },
            {
               -20, -10, -10,  -5,  -5, -10, -10, -20,
               -10,   0,   0,   0,   0,   0,   0, -10,
               -10,   0,   5,   5,   5,   5,   0, -10,
                -5,   0,   5,  10,  10,   5,   0,  -5,
                 0,   0,   5,  10,  10,   5,   0,  -5,
               -10,   5,   5,   5,   5,   5,   0, -10,
               -10,   0,   5,   0,   0,   0,   0, -10,
               -20, -10, -10,  -5,  -5, -10, -10, -20,
            },
            {
                20,  30,  10,   0,   0,  10,  30,  20,
                20,  20,   0,   0,   0,   0,  20,  20,
               -10, -20, -20, -20, -20, -20, -20, -10,
               -20, -30, -30, -40, -40, -30, -30, -20,
               -30, -40, -40, -50, -50, -40, -40, -30,
               -30, -40, -40, -50, -50, -40, -40, -30,
               -30, -40, -40, -50, -50, -40, -40, -30,
               -30, -40, -40, -50, -50, -40, -40, -30,
            },
        };

        constexpr Square mirror_square(Square square) {
            return square ^ 56;
        }

        Score piece_square_value(PieceType piece_type, Color color, Square square) {
            if (color == BLACK) {
                square = mirror_square(square);
            }
            return PIECE_SQUARE_VALUES[piece_type][square];
        }

        Score evaluate_for(const Board& board, Color color) {
            Score score = 0;
            for (int piece_type = PAWN; piece_type <= KING; ++piece_type) {
                Bitboard pieces = board.pieces[color][piece_type];
                while (pieces != 0) {
                    Square square = static_cast<Square>(std::countr_zero(pieces));
                    score += PIECE_VALUES[piece_type];
                    score += piece_square_value(static_cast<PieceType>(piece_type), color, square);
                    pieces &= pieces - 1;
                }
            }
            return score;
        }
    }

    Score evaluate(const Board& board) {
        Score white_minus_black = evaluate_for(board, WHITE) - evaluate_for(board, BLACK);
        return board.side_to_move == WHITE ? white_minus_black : -white_minus_black;
    }
}
