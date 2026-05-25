#ifndef CPP_ENGINE_PERFT_H
#define CPP_ENGINE_PERFT_H
#include"rengine/board.h"
#include"types.h"
#include<cstdint>

namespace rengine {
    uint64_t perft(Board& board, int depth);
}

#endif //CPP_ENGINE_PERFT_H