#ifndef CPP_ENGINE_SEARCH_H
#define CPP_ENGINE_SEARCH_H

#include"rengine/board.h"
#include"rengine/search_types.h"

namespace rengine {
    SearchResult search_position(Board& board, const SearchLimits& limits);
    void clear_search_tables();
    Score negamax(Board& board, int depth, Score alpha, Score beta, int ply, SearchContext& ctx,
                  MoveList& pv, bool allow_null_move = true);
    SearchResult search_root(Board& board, int depth, SearchContext& ctx, Move previous_best);
    Score qsearch(Board& board, Score alpha, Score beta, int ply, SearchContext& ctx);
    inline SearchResult search(Board& board, const SearchLimits& limits) {
        return search_position(board, limits);
    }
}

#endif //CPP_ENGINE_SEARCH_H
