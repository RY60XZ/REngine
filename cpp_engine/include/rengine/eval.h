#ifndef CPP_ENGINE_EVAL_H
#define CPP_ENGINE_EVAL_H

#include"rengine/board.h"
#include"rengine/search_types.h"

namespace rengine {
    Score evaluate_pst(const Board& board);
    Score evaluate(const Board& board);
}

#endif //CPP_ENGINE_EVAL_H
