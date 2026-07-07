#ifndef CPP_ENGINE_TIME_H
#define CPP_ENGINE_TIME_H

#include"rengine/search_types.h"
#include"rengine/types.h"
#include<chrono>
#include<cstdint>

namespace rengine {
    struct TimeSettings {
        int depth = 0;
        std::uint64_t nodes = NO_NODE_LIMIT;
        std::chrono::milliseconds movetime{0};
        bool infinite = false;

        std::chrono::milliseconds white_time{-1};
        std::chrono::milliseconds black_time{-1};
        std::chrono::milliseconds white_increment{0};
        std::chrono::milliseconds black_increment{0};
        int moves_to_go = 0;
    };

    std::chrono::milliseconds allocate_time(const TimeSettings& settings, Color side_to_move);
    SearchLimits make_search_limits(const TimeSettings& settings, Color side_to_move);
}

#endif //CPP_ENGINE_TIME_H
