#include"rengine/search.h"
#include"rengine/attack.h"
#include"rengine/draw.h"
#include"rengine/eval.h"
#include"rengine/make_move.h"
#include"rengine/movegen.h"
#include<algorithm>
#include<chrono>

namespace rengine {
    namespace {
        using Clock = std::chrono::steady_clock;
        constexpr std::uint64_t TIME_CHECK_INTERVAL = 1024;

        void finish_timing(SearchResult& result, Clock::time_point start) {
            result.stats.elapsed_time = Clock::now() - start;
        }

        bool should_stop(SearchContext& ctx) {
            if (ctx.stats.nodes>=ctx.limits.node_limit) {
                ctx.stopped = true;
                ctx.stats.stop_reason = StopReason::NodeLimit;
                return true;
            }

            if ((ctx.stats.nodes % TIME_CHECK_INTERVAL)==0 && Clock::now()>=ctx.deadline) {
                ctx.stopped = true;
                ctx.stats.stop_reason = StopReason::TimeLimit;
                return true;
            }

            return false;
        }
    }

    Score negamax(Board& board, int depth, Score alpha, Score beta, int ply, SearchContext& ctx) {
        if (should_stop(ctx)) {
            return alpha;
        }

        ++ctx.stats.nodes;
        ctx.stats.max_ply = std::max(ctx.stats.max_ply, ply);

        MoveList legal_moves;
        generate_legal_moves(board, legal_moves);

        if (legal_moves.empty()) {
            if (in_check(board, board.side_to_move)) {
                return mated_in(ply);
            }
            else return VALUE_DRAW;
        }

        if (is_fifty_move_rule_draw(board)) {
            return VALUE_DRAW;
        }
        if (is_insufficient_material_draw(board)) {
            return VALUE_DRAW;
        }
        if (is_threefold_draw(board)) {
            return VALUE_DRAW;
        }

        if (depth==0) {
            return evaluate(board);
        }

        Score best_score = -VALUE_INF;
        for (auto move : legal_moves) {
            Undo undo{};
            make_move(board, move, undo);
            Score current_score = -negamax(board, depth-1, -beta, -alpha, ply+1, ctx);
            unmake_move(board, undo);

            if (ctx.stopped) {
                return alpha; //placeholder value since this value should never be used
            }

            best_score = std::max(best_score, current_score);
            if (best_score > alpha) {
                alpha = best_score;
            }
            if (alpha>=beta) {
                ++ctx.stats.cutoffs;
                return alpha;
            }
        }

        return best_score;
    }

    SearchResult search_position(Board& board, const SearchLimits& limits) {
        const auto start = Clock::now();
        const auto deadline = limits.movetime.count() > 0
            ? start + limits.movetime
            : Clock::time_point::max();
        const int requested_depth = std::max(0, limits.depth);

        SearchContext ctx{limits, {}, deadline, false};
        SearchResult result;
        ctx.stats.stop_reason = limits.infinite ? StopReason::Infinite : StopReason::DepthLimit;

        if (requested_depth == 0) {
            result.score = evaluate(board);
            finish_timing(result, start);
            return result;
        }

        MoveList legal_moves;
        generate_legal_moves(board, legal_moves);

        if (legal_moves.empty()) {
            result.score = in_check(board, board.side_to_move) ? mated_in(0) : VALUE_DRAW;
            ctx.stats.stop_reason = StopReason::NoLegalMoves;
            result.stats = ctx.stats;
            finish_timing(result, start);
            return result;
        }

        if (is_fifty_move_rule_draw(board) ||
            is_insufficient_material_draw(board) ||
            is_threefold_draw(board)) {
            result.score = VALUE_DRAW;
            result.stats = ctx.stats;
            finish_timing(result, start);
            return result;
        }

        Score best_score = -VALUE_INF;
        Score alpha = -VALUE_INF;
        bool completed_root = true;

        for (Move move : legal_moves) {
            if (should_stop(ctx)) {
                completed_root = false;
                break;
            }

            Undo undo{};
            make_move(board, move, undo);
            Score score = -negamax(board, requested_depth-1, -VALUE_INF, -alpha, 1, ctx);
            unmake_move(board, undo);

            if (ctx.stopped) {
                completed_root = false;
                break;
            }

            if (!result.has_best_move || score > best_score) {
                best_score = score;
                result.best_move = move;
                result.has_best_move = true;
                result.score = score;
                result.principal_variation = {move};
            }
            if (score>alpha) {
                alpha = score;
            }
        }

        if (completed_root) {
            result.completed_depth = requested_depth;
            ctx.stats.stop_reason = StopReason::DepthLimit;
        }

        result.stats = ctx.stats;
        finish_timing(result, start);
        return result;
    }
}
