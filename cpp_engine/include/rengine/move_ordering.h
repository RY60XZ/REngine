#ifndef CPP_ENGINE_MOVE_ORDERING_H
#define CPP_ENGINE_MOVE_ORDERING_H

#include"rengine/board.h"
#include"rengine/move.h"
#include"rengine/move_list.h"
#include"rengine/search_types.h"

namespace rengine {
    using MoveScore = int;

    constexpr MoveScore QUIET_MOVE_SCORE = 0;

    MoveScore piece_order_value(PieceType piece_type);
    MoveScore move_order_score(const Board& board, Move move, Move preferred_move = 0);
    MoveScore move_order_score(const Board& board, Move move, const SearchStack& stack,
                               int ply, Move preferred_move = 0, SearchStats* stats = nullptr);
    Move select_best_move(const Board& board, MoveList& moves, int start_index, Move preferred_move = 0);
    Move select_best_move(const Board& board, MoveList& moves, int start_index,
                          const SearchStack& stack, int ply, Move preferred_move = 0,
                          SearchStats* stats = nullptr);
    bool is_noisy_move(Move move);
    bool is_quiet_move(Move move);
    int killer_slot_for_move(const SearchStack& stack, int ply, Move move);
    bool store_killer_move(SearchStack& stack, int ply, Move move);
    MoveScore history_score(const SearchStack& stack, Color color, Move move);
    void update_history(SearchStack& stack, Color color, Move move, int depth);
}

#endif //CPP_ENGINE_MOVE_ORDERING_H
