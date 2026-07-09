#ifndef CPP_ENGINE_MAKE_MOVE_H
#define CPP_ENGINE_MAKE_MOVE_H
#include"rengine/board.h"
#include"rengine/move.h"
#include"rengine/types.h"
namespace rengine {
    struct Undo {
        Move move;
        Piece captured_piece;
        unsigned castling_rights;
        Square en_passant_square;
        ZobristKey zobrist_key;
        int half_move_clock;
        int full_move_number;
        int position_history_size;
        bool nnue_dirty;
        bool nnue_initialized;
    };

    void make_move(Board& board, Move move, Undo& undo);
    void unmake_move(Board& board, const Undo& undo);
    void make_null_move(Board& board, Undo& undo);
    void unmake_null_move(Board& board, const Undo& undo);
    [[nodiscard]] inline unsigned update_castling_rights(Board& board, Square from, Square to) {
        unsigned original_castling_rights = board.castling_rights;
        switch (from) {
            case 0: //a1
                board.castling_rights &= ~WHITE_QUEENSIDE_CASTLING;
                break;
            case 4: //e1
                board.castling_rights &= ~(WHITE_QUEENSIDE_CASTLING | WHITE_KINGSIDE_CASTLING);
                break;
            case 7: //h1
                board.castling_rights &= ~WHITE_KINGSIDE_CASTLING;
                break;
            case 56: //a8
                board.castling_rights &= ~BLACK_QUEENSIDE_CASTLING;
                break;
            case 60: //e8
                board.castling_rights &= ~(BLACK_KINGSIDE_CASTLING | BLACK_QUEENSIDE_CASTLING);
                break;
            case 63: //h8
                board.castling_rights &= ~BLACK_KINGSIDE_CASTLING;
                break;
            default:
                break;
        }
        switch (to) {
            case 0: //a1
                board.castling_rights &= ~WHITE_QUEENSIDE_CASTLING;
                break;
            case 7: //h1
                board.castling_rights &= ~WHITE_KINGSIDE_CASTLING;
                break;
            case 56: //a8
                board.castling_rights &= ~BLACK_QUEENSIDE_CASTLING;
                break;
            case 63: //h8
                board.castling_rights &= ~BLACK_KINGSIDE_CASTLING;
                break;
            default:
                break;
        }
        return original_castling_rights;
    }
}
#endif //CPP_ENGINE_MAKE_MOVE_H
