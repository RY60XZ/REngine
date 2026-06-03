#include"rengine/eval.h"
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

        Score material_for(const Board& board, Color color) {
            Score material = 0;
            for (int piece_type = PAWN; piece_type <= KING; ++piece_type) {
                material += PIECE_VALUES[piece_type] *
                    std::popcount(board.pieces[color][piece_type]);
            }
            return material;
        }
    }

    Score evaluate(const Board& board) {
        Score white_minus_black = material_for(board, WHITE) - material_for(board, BLACK);
        return board.side_to_move == WHITE ? white_minus_black : -white_minus_black;
    }
}
