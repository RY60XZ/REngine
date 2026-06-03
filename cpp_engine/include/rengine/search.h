#ifndef CPP_ENGINE_SEARCH_H
#define CPP_ENGINE_SEARCH_H

#include"rengine/board.h"
#include"rengine/search_types.h"

namespace rengine {
    SearchResult search_position(Board& board, const SearchLimits& limits);

    inline SearchResult search(Board& board, const SearchLimits& limits) {
        return search_position(board, limits);
    }
}

#endif //CPP_ENGINE_SEARCH_H
