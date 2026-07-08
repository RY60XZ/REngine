#include "rengine/board.h"
#include "rengine/types.h"
#include "cassert"
namespace rengine{
    void clear(Board& board) {
        for (auto& color_pieces : board.pieces) {
            for (auto& pieces : color_pieces) {
                pieces = 0;
            }
        }
        board.occupied[WHITE] = 0;
        board.occupied[BLACK] = 0;
        board.all = 0;
        for (auto& square : board.squares) {
            square = NO_PIECE;
        }

        board.side_to_move = WHITE;
        board.castling_rights = 0;
        board.en_passant_square = INVALID_SQUARE;
        board.zobrist_key = 0;
        board.half_move_clock = 0;
        board.full_move_number = 1;
        board.position_history_size = 0;
    }
}
