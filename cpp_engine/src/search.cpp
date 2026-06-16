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
            const std::uint64_t searched_nodes = ctx.stats.nodes + ctx.stats.qnodes;
            if (searched_nodes >= ctx.limits.node_limit) {
                ctx.stopped = true;
                ctx.stats.stop_reason = StopReason::NodeLimit;
                return true;
            }

            if ((searched_nodes % TIME_CHECK_INTERVAL)==0 && Clock::now()>=ctx.deadline) {
                ctx.stopped = true;
                ctx.stats.stop_reason = StopReason::TimeLimit;
                return true;
            }

            return false;
        }
    }

    Score negamax(Board& board, int depth, Score alpha, Score beta, int ply, SearchContext& ctx, MoveList& pv) {
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
            return qsearch(board, alpha, beta, ply, ctx);
        }

        MoveList child_pv;
        Score best_score = -VALUE_INF;
        for (auto move : legal_moves) {
            child_pv.clear();
            Undo undo{};
            make_move(board, move, undo);
            Score current_score = -negamax(board, depth-1, -beta, -alpha, ply+1, ctx, child_pv);
            unmake_move(board, undo);

            if (ctx.stopped) {
                return alpha; //placeholder value since this value should never be used
            }

            best_score = std::max(best_score, current_score);
            if (best_score > alpha) {
                alpha = best_score;
                pv.clear();
                pv.push_back(move);
                pv.append(child_pv.begin(), child_pv.end());
            }
            if (alpha>=beta) {
                ++ctx.stats.cutoffs;
                return alpha;
            }
        }

        return best_score;
    }

    SearchResult search_root(Board& board, int depth, SearchContext& ctx, Move previous_best) {
        SearchResult result;

        MoveList legal_moves;
        generate_legal_moves(board, legal_moves);

        if (legal_moves.empty()) {
            result.score = in_check(board, board.side_to_move) ? mated_in(0) : VALUE_DRAW;
            ctx.stats.stop_reason = StopReason::NoLegalMoves;
            result.stats = ctx.stats;
            return result;
        }

        if (is_fifty_move_rule_draw(board) ||
            is_insufficient_material_draw(board) ||
            is_threefold_draw(board)) {
            result.score = VALUE_DRAW;
            result.stats = ctx.stats;
            return result;
        }

        Score best_score = -VALUE_INF;
        Score alpha = -VALUE_INF;

        auto search_move = [&](Move move) {
            if (should_stop(ctx)) {
                return false;
            }

            MoveList child_pv;
            Undo undo{};
            make_move(board, move, undo);
            Score score = -negamax(board, depth-1, -VALUE_INF, -alpha, 1, ctx, child_pv);
            unmake_move(board, undo);

            if (ctx.stopped) {
                return false;
            }

            if (!result.has_best_move || score > best_score) {
                best_score = score;
                result.best_move = move;
                result.has_best_move = true;
                result.score = score;
                result.principal_variation.clear();
                result.principal_variation.push_back(move);
                result.principal_variation.append(child_pv.begin(), child_pv.end());
            }
            if (score>alpha) {
                alpha = score;
            }
            return true;
        };

        if (previous_best != 0) {
            for (Move move : legal_moves) {
                if (move == previous_best && !search_move(move)) {
                    result.stats = ctx.stats;
                    return result;
                }
            }
        }

        for (Move move : legal_moves) {
            if (move == previous_best) {
                continue;
            }
            if (!search_move(move)) {
                result.stats = ctx.stats;
                return result;
            }
        }

        result.completed_depth = depth;
        result.stats = ctx.stats;
        return result;
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
            result.score = qsearch(board, -VALUE_INF, VALUE_INF, 0, ctx);
            result.stats = ctx.stats;
            finish_timing(result, start);
            return result;
        }

        Move previous_best = 0;

        for (int depth = 1; depth <= requested_depth; ++depth) {
            SearchResult depth_result = search_root(board, depth, ctx, previous_best);
            if (ctx.stopped) {
                if (!result.has_best_move && depth_result.has_best_move) {
                    result = depth_result;
                    result.completed_depth = 0;
                }
                break;
            }

            result = depth_result;
            previous_best = result.has_best_move ? result.best_move : 0;
            ctx.stats.stop_reason = StopReason::DepthLimit;
        }

        result.stats = ctx.stats;
        finish_timing(result, start);
        return result;
    }
    Score qsearch(Board& board, Score alpha, Score beta, int ply, SearchContext& ctx) {
        if (should_stop(ctx)) {
            return alpha;
        }
        ++ctx.stats.qnodes;
        ctx.stats.max_ply = std::max(ctx.stats.max_ply, ply);

        MoveList moves_to_search, legal_moves;
        generate_legal_moves(board, legal_moves);

        const bool king_in_check = in_check(board, board.side_to_move);
        if (legal_moves.empty()) {
            return king_in_check ? mated_in(ply) : VALUE_DRAW;
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

        Score best_score = -VALUE_INF;
        if (king_in_check) {
            moves_to_search = legal_moves;
        }
        else {
            best_score = evaluate(board);
            if (best_score >= beta) {
                return best_score;
            }
            if (best_score > alpha) {
                alpha = best_score;
            }

            for (auto move : legal_moves) {
                if (is_promotion(move) || is_capture(move)) {
                    moves_to_search.push_back(move);
                }
            }
        }

        for (auto move : moves_to_search) {
            Undo undo{};
            make_move(board, move, undo);
            auto sc = -qsearch(board, -beta, -alpha, ply+1, ctx);
            unmake_move(board, undo);

            if (ctx.stopped) {
                return alpha;
            }

            if (sc >= beta) {
                return sc;
            }
            if (sc > alpha) {
                alpha = sc;
            }
            if (sc > best_score) {
                best_score = sc;
            }
        }
        return best_score;
    }
}
