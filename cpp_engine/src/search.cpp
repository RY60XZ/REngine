#include"rengine/search.h"
#include"rengine/eval.h"
#include"rengine/make_move.h"
#include"rengine/movegen.h"
#include<algorithm>
#include<chrono>

namespace rengine {
    namespace {
        using Clock = std::chrono::steady_clock;

        void finish_timing(SearchResult& result, Clock::time_point start) {
            result.stats.elapsed_time = Clock::now() - start;
        }
    }

    SearchResult search_position(Board& board, const SearchLimits& limits) {
        const auto start = Clock::now();
        const bool has_node_limit = limits.node_limit != NO_NODE_LIMIT;
        const bool has_time_limit = limits.movetime.count() > 0;
        const auto deadline = has_time_limit ? start + limits.movetime : start;
        const int requested_depth = std::max(0, limits.depth);

        SearchResult result;
        result.stats.stop_reason = limits.infinite ? StopReason::Infinite : StopReason::DepthLimit;

        if (requested_depth == 0) {
            result.score = evaluate(board);
            finish_timing(result, start);
            return result;
        }

        MoveList legal_moves;
        generate_legal_moves(board, legal_moves);

        if (legal_moves.empty()) {
            result.score = evaluate(board);
            result.stats.stop_reason = StopReason::NoLegalMoves;
            finish_timing(result, start);
            return result;
        }

        Score best_score = -VALUE_INF;
        bool completed_root = true;

        for (Move move : legal_moves) {
            if (has_node_limit && result.stats.nodes >= limits.node_limit) {
                result.stats.stop_reason = StopReason::NodeLimit;
                completed_root = false;
                break;
            }

            if (has_time_limit && Clock::now() >= deadline) {
                result.stats.stop_reason = StopReason::TimeLimit;
                completed_root = false;
                break;
            }

            Undo undo{};
            make_move(board, move, undo);
            ++result.stats.nodes;
            result.stats.max_ply = std::max(result.stats.max_ply, 1);
            Score score = -evaluate(board);
            unmake_move(board, undo);

            if (!result.has_best_move || score > best_score) {
                best_score = score;
                result.best_move = move;
                result.has_best_move = true;
                result.score = score;
                result.principal_variation = {move};
            }
        }

        if (completed_root) {
            result.completed_depth = 1;
            result.stats.stop_reason = StopReason::DepthLimit;
        }

        finish_timing(result, start);
        return result;
    }
}
