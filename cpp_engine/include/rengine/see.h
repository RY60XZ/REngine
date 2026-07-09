#ifndef CPP_ENGINE_SEE_H
#define CPP_ENGINE_SEE_H

#include "rengine/board.h"
#include "rengine/move.h"
#include "rengine/search_types.h"

namespace rengine {
    Score see_score(const Board& board, Move move);
    bool see_ge(const Board& board, Move move, Score threshold);
}

#endif //CPP_ENGINE_SEE_H
