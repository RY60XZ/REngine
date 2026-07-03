#ifndef CPP_ENGINE_SEARCH_TYPES_H
#define CPP_ENGINE_SEARCH_TYPES_H
#include"rengine/move.h"
#include"rengine/move_list.h"
#include<chrono>
#include<cstdint>
#include<limits>

namespace rengine {
    using Score = int;

    constexpr Score VALUE_DRAW = 0;
    constexpr Score VALUE_MATE = 32000;
    constexpr Score VALUE_INF = 32767;
    constexpr int MAX_PLY = 256;
    constexpr Score VALUE_MATE_IN_MAX_PLY = VALUE_MATE - MAX_PLY;
    constexpr Score VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + MAX_PLY;
    constexpr std::uint64_t NO_NODE_LIMIT = std::numeric_limits<std::uint64_t>::max();

    enum class StopReason {
        DepthLimit,
        NodeLimit,
        TimeLimit,
        Infinite,
        NoLegalMoves,
    };

    constexpr Score mate_in(int ply) {
        return VALUE_MATE - ply;
    }

    constexpr Score mated_in(int ply) {
        return -VALUE_MATE + ply;
    }

    constexpr bool is_mate_score(Score score) {
        return (score >= VALUE_MATE_IN_MAX_PLY && score < VALUE_INF) ||
               (score <= VALUE_MATED_IN_MAX_PLY && score > -VALUE_INF);
    }

    constexpr Score score_to_tt(Score score, int ply) {
        if (score >= VALUE_MATE_IN_MAX_PLY && score < VALUE_INF) {
            return score + ply;
        }
        if (score <= VALUE_MATED_IN_MAX_PLY && score > -VALUE_INF) {
            return score - ply;
        }
        return score;
    }

    constexpr Score score_from_tt(Score score, int ply) {
        if (score >= VALUE_MATE_IN_MAX_PLY && score < VALUE_INF) {
            return score - ply;
        }
        if (score <= VALUE_MATED_IN_MAX_PLY && score > -VALUE_INF) {
            return score + ply;
        }
        return score;
    }

    struct SearchLimits {
        int depth = 1;
        std::uint64_t node_limit = NO_NODE_LIMIT;
        std::chrono::milliseconds movetime{0};
        bool infinite = false;
    };

    struct SearchStats {
        std::uint64_t nodes = 0;
        std::uint64_t qnodes = 0;
        std::uint64_t cutoffs = 0;
        std::uint64_t first_move_cutoffs = 0;
        std::chrono::nanoseconds elapsed_time{0};
        int max_ply = 0;
        StopReason stop_reason = StopReason::DepthLimit;
    };

    struct SearchResult {
        Move best_move = 0;
        bool has_best_move = false;
        Score score = VALUE_DRAW;
        int completed_depth = 0;
        SearchStats stats;
        MoveList principal_variation;
    };

    struct SearchContext {
        const SearchLimits& limits;
        SearchStats stats;
        std::chrono::steady_clock::time_point deadline;
        bool stopped = false;
    };

}

#endif //CPP_ENGINE_SEARCH_TYPES_H
