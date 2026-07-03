#ifndef CPP_ENGINE_MOVE_ORDERING_H
#define CPP_ENGINE_MOVE_ORDERING_H

#include"rengine/board.h"
#include"rengine/move.h"
#include"rengine/move_list.h"

namespace rengine {
    using MoveScore = int;

    constexpr MoveScore QUIET_MOVE_SCORE = 0;

    MoveScore piece_order_value(PieceType piece_type);
    MoveScore move_order_score(const Board& board, Move move, Move preferred_move = 0);
    Move select_best_move(const Board& board, MoveList& moves, int start_index, Move preferred_move = 0);
    bool is_noisy_move(Move move);
}

#endif //CPP_ENGINE_MOVE_ORDERING_H
