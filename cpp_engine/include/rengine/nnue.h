#ifndef CPP_ENGINE_NNUE_H
#define CPP_ENGINE_NNUE_H

#include "rengine/board.h"
#include "rengine/move.h"
#include "rengine/search_types.h"
#include <string>

namespace rengine {
    bool load_nnue(const std::string& path);
    bool load_default_nnue();
    bool nnue_is_loaded();

    void refresh_nnue(Board& board);
    void update_nnue_after_move(Board& board, Move move, Piece moved_piece,
                                Piece captured_piece, unsigned old_castling_rights);
    void update_nnue_after_unmove(Board& board, Move move, Piece moved_piece,
                                  Piece captured_piece, unsigned post_castling_rights,
                                  bool previous_dirty, bool previous_initialized);

    Score evaluate_nnue(const Board& board);
    bool nnue_accumulator_matches_recompute(const Board& board, int epsilon = 0);
}

#endif //CPP_ENGINE_NNUE_H
