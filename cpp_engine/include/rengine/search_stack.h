#ifndef CPP_ENGINE_SEARCH_STACK_H
#define CPP_ENGINE_SEARCH_STACK_H

#include"rengine/move.h"
#include<array>
#include<cstdint>

namespace rengine {
    constexpr int MAX_PLY = 512;
    constexpr int KILLER_MOVE_SLOTS = 2;
    constexpr int BOARD_SQUARES = 64;
    constexpr int COLOR_COUNT = 2;

    struct SearchStack {
        std::array<std::array<Move, KILLER_MOVE_SLOTS>, MAX_PLY> killer_moves{};
        std::array<std::array<std::array<std::int32_t, BOARD_SQUARES>, BOARD_SQUARES>, COLOR_COUNT> history{};
    };
}

#endif //CPP_ENGINE_SEARCH_STACK_H
