#ifndef CPP_ENGINE_MOVEGEN_H
#define CPP_ENGINE_MOVEGEN_H
#include "rengine/board.h"
#include "rengine/types.h"
#include "rengine/move_list.h"
#include <bit>

namespace rengine {
    void generate_pseudo_legal_moves(const Board& board, MoveList& move_list, Color by);

    void generate_knight_moves(const Board& board, MoveList& move_list, Color by);
    void generate_king_moves(const Board& board, MoveList& move_list, Color by);
    void generate_bishop_moves(const Board& board, MoveList& move_list, Color by);
    void generate_rook_moves(const Board& board, MoveList& move_list, Color by);
    void generate_queen_moves(const Board& board, MoveList& move_list, Color by);
    void generate_pawn_moves(const Board& board, MoveList& move_list, Color by);
    void generate_pawn_single_moves(const Board& board, MoveList& move_list, Color by);
    void generate_pawn_double_moves(const Board& board, MoveList& move_list, Color by);
    void generate_pawn_capture_moves(const Board& board, MoveList& move_list, Color by);
    void generate_pawn_promotion_moves(const Board& board, MoveList& move_list, Color by);
    void generate_pawn_capture_promotion_moves(const Board& board, MoveList& move_list, Color by);
    void generate_pawn_en_passant_moves(const Board& board, MoveList& move_list, Color by);
    void generate_castling_moves(const Board& board, MoveList& move_list, Color by);

    inline Square pop_lsb(Bitboard& bb) {
        assert(bb!=0);
        auto square = static_cast<Square>(std::countr_zero(bb));
        bb = bb & (bb-1);
        return square;
    }
}
#endif //CPP_ENGINE_MOVEGEN_H