#include"rengine/time.h"
#include"rengine/search_stack.h"
#include<algorithm>

namespace rengine {
    namespace {
        bool has_clock_time(const TimeSettings& settings, Color side_to_move) {
            const auto remaining = side_to_move == WHITE ? settings.white_time : settings.black_time;
            return remaining.count() > 0;
        }
    }

    std::chrono::milliseconds allocate_time(const TimeSettings& settings, Color side_to_move) {
        const auto remaining = side_to_move == WHITE ? settings.white_time : settings.black_time;
        const auto increment = side_to_move == WHITE ? settings.white_increment : settings.black_increment;

        if (remaining.count() <= 0) {
            return std::chrono::milliseconds{0};
        }

        const long long clock_ms = remaining.count();
        const long long reserve_ms = std::min(50LL, std::max(1LL, clock_ms / 20));
        const long long available_ms = std::max(1LL, clock_ms - reserve_ms);
        const int divisor = settings.moves_to_go > 0 ? settings.moves_to_go : 30;

        long long budget_ms = available_ms / divisor + (increment.count() * 3) / 4;
        const long long cap_ms = std::max(1LL, available_ms / 2);
        budget_ms = std::clamp(budget_ms, 1LL, cap_ms);

        return std::chrono::milliseconds{budget_ms};
    }

    SearchLimits make_search_limits(const TimeSettings& settings, Color side_to_move) {
        SearchLimits limits;
        const bool fixed_depth = settings.depth > 0;
        const bool fixed_nodes = settings.nodes != NO_NODE_LIMIT;

        limits.depth = fixed_depth ? settings.depth : MAX_PLY;
        limits.node_limit = settings.nodes;
        limits.movetime = settings.movetime;

        if (limits.movetime.count() == 0 && has_clock_time(settings, side_to_move)) {
            limits.movetime = allocate_time(settings, side_to_move);
        }

        const bool has_time_limit = limits.movetime.count() > 0;
        limits.infinite = settings.infinite || (!fixed_depth && !fixed_nodes && !has_time_limit);

        return limits;
    }
}
