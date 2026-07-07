#include"rengine/search.h"
#include"rengine/attack.h"
#include"rengine/draw.h"
#include"rengine/eval.h"
#include"rengine/make_move.h"
#include"rengine/move_ordering.h"
#include"rengine/movegen.h"
#include"rengine/tt.h"
#include<algorithm>
#include<atomic>
#include<chrono>

namespace rengine {
    namespace {
        using Clock = std::chrono::steady_clock;
        constexpr std::uint64_t TIME_CHECK_INTERVAL = 1024;

        void finish_timing(SearchResult& result, Clock::time_point start) {
            result.stats.elapsed_time = Clock::now() - start;
        }

        bool should_stop(SearchContext& ctx) {
            if (ctx.limits.stop != nullptr && ctx.limits.stop->load(std::memory_order_relaxed)) {
                ctx.stopped = true;
                ctx.stats.stop_reason = StopReason::Stopped;
                return true;
            }

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

        TranspositionTable& shared_tt() {
            static TranspositionTable tt(1 << 21);
            return tt;
        }

        void record_beta_cutoff(SearchContext& ctx, const Board& board, Move move,
                                int ply, int depth, int move_index) {
            ++ctx.stats.cutoffs;
            if (move_index == 0) {
                ++ctx.stats.first_move_cutoffs;
            }

            if (!is_quiet_move(move)) {
                return;
            }

            if (killer_slot_for_move(ctx.stack, ply, move) >= 0) {
                ++ctx.stats.killer_cutoffs;
            }
            else {
                ++ctx.stats.history_quiet_cutoffs;
            }

            store_killer_move(ctx.stack, ply, move);
            update_history(ctx.stack, board.side_to_move, move, depth);
        }
    }

    void clear_search_tables() {
        shared_tt().clear();
    }

    Score negamax(Board& board, int depth, Score alpha, Score beta, int ply, SearchContext& ctx, MoveList& pv) {
        if (should_stop(ctx)) {
            return alpha;
        }

        ++ctx.stats.nodes;
        ctx.stats.max_ply = std::max(ctx.stats.max_ply, ply);

        if (depth==0) {
            return qsearch(board, alpha, beta, ply, ctx);
        }

        const bool is_draw = is_fifty_move_rule_draw(board) ||
                             is_insufficient_material_draw(board) ||
                             is_threefold_draw(board);

        Color us = board.side_to_move;
        bool king_in_check = in_check(board, us);

        bool found_legal_moves = false;

        Score alpha_original = alpha;
        Move hash_move = 0;
        TTEntry* tt_entry = is_draw ? nullptr : ctx.tt->probe(board.zobrist_key);
        if (tt_entry != nullptr) {
            hash_move = tt_entry->best_move;
            if (tt_entry->depth >= depth) {
                Score tt_score = score_from_tt(tt_entry->score, ply);
                if (tt_entry->flag == TT_EXACT) {
                    return tt_score;
                }
                if (tt_entry->flag == TT_LOWER && tt_score >= beta) {
                    return tt_score;
                }
                if (tt_entry->flag == TT_UPPER && tt_score <= alpha) {
                    return tt_score;
                }
            }
        }

        MoveList pseudo_legal_moves;
        generate_pseudo_legal_moves(board, pseudo_legal_moves);
        MoveScoreList move_scores;
        score_moves(board, pseudo_legal_moves, move_scores, ctx.stack, ply, hash_move, &ctx.stats);

        MoveList child_pv;
        Score best_score = -VALUE_INF;
        Move best_move = 0;
        for (int move_index = 0; move_index < pseudo_legal_moves.size(); ++move_index) {
            Move move = select_best_move(pseudo_legal_moves, move_scores, move_index);
            Undo undo{};
            make_move(board, move, undo);
            if (in_check(board, us)) {
                unmake_move(board, undo);
                continue;
            }
            found_legal_moves = true;
            if (is_draw) {
                unmake_move(board, undo);
                return VALUE_DRAW;
            }
            child_pv.clear();
            Score current_score = -negamax(board, depth-1, -beta, -alpha, ply+1, ctx, child_pv);
            unmake_move(board, undo);

            if (ctx.stopped) {
                return alpha; //placeholder value since this value should never be used
            }

            if (current_score > best_score) {
                best_score = current_score;
                best_move = move;
            }
            if (current_score > alpha) {
                alpha = current_score;
                pv.clear();
                pv.push_back(move);
                pv.append(child_pv.begin(), child_pv.end());
            }
            if (alpha>=beta) {
                record_beta_cutoff(ctx, board, move, ply, depth, move_index);
                ctx.tt->store(board.zobrist_key, depth, score_to_tt(alpha, ply), TT_LOWER, move);
                return alpha;
            }
        }
        if (!found_legal_moves) {
            return king_in_check ? mated_in(ply) : VALUE_DRAW;
        }
        TTFlag flag = TT_EXACT;
        if (best_score <= alpha_original) {
            flag = TT_UPPER;
        }
        ctx.tt->store(board.zobrist_key, depth, score_to_tt(best_score, ply), flag, best_move);
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
        Move preferred_move = previous_best;
        TTEntry* tt_entry = ctx.tt->probe(board.zobrist_key);
        if (tt_entry != nullptr && tt_entry->best_move != 0) {
            preferred_move = tt_entry->best_move;
        }
        MoveScoreList move_scores;
        score_moves(board, legal_moves, move_scores, ctx.stack, 0, preferred_move, &ctx.stats);

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

        for (int move_index = 0; move_index < legal_moves.size(); ++move_index) {
            Move move = select_best_move(legal_moves, move_scores, move_index);
            if (!search_move(move)) {
                result.stats = ctx.stats;
                return result;
            }
        }

        if (result.has_best_move) {
            ctx.tt->store(board.zobrist_key, depth, score_to_tt(result.score, 0), TT_EXACT, result.best_move);
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
        TranspositionTable& tt = shared_tt();
        tt.advance_age();
        SearchContext ctx{limits, {}, deadline, false, {}, &tt};
        SearchResult result;
        ctx.stats.stop_reason = limits.infinite ? StopReason::Infinite : StopReason::DepthLimit;

        if (requested_depth == 0) {
            result.score = qsearch(board, -VALUE_INF, VALUE_INF, 0, ctx);
            result.stats = ctx.stats;
            finish_timing(result, start);
            if (limits.info_callback) {
                limits.info_callback(result);
            }
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
            result.stats = ctx.stats;
            finish_timing(result, start);
            if (limits.info_callback) {
                limits.info_callback(result);
            }
            if (ctx.stats.stop_reason == StopReason::NoLegalMoves) {
                break;
            }
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

        Color us = board.side_to_move;
        const bool king_in_check = in_check(board, us);
        const bool is_draw = is_fifty_move_rule_draw(board) ||
                             is_insufficient_material_draw(board) ||
                             is_threefold_draw(board);

        MoveList qsearch_pseudo_legal_moves;
        Score best_score = -VALUE_INF;
        if (king_in_check) {
            generate_qsearch_pseudo_legal_moves(board, qsearch_pseudo_legal_moves);
            if (qsearch_pseudo_legal_moves.empty()) {
                return mated_in(ply);
            }
            if (is_draw) {
                return VALUE_DRAW;
            }
        }
        else {
            if (is_draw) return VALUE_DRAW;

            best_score = evaluate(board);
            if (best_score >= beta) {
                return has_any_legal_move(board) ? best_score : VALUE_DRAW;
            }
            if (best_score > alpha) alpha = best_score;

            generate_qsearch_pseudo_legal_moves(board, qsearch_pseudo_legal_moves);
        }
        MoveScoreList move_scores;
        score_moves(board, qsearch_pseudo_legal_moves, move_scores, ctx.stack, ply, 0, &ctx.stats);

        bool found_legal_moves = king_in_check;
        for (int move_index = 0; move_index < qsearch_pseudo_legal_moves.size(); ++move_index) {
            Move move = select_best_move(qsearch_pseudo_legal_moves, move_scores, move_index);
            Undo undo{};
            make_move(board, move, undo);
            if (!king_in_check && in_check(board, us)) {
                unmake_move(board, undo);
                continue;
            }

            found_legal_moves = true;
            auto sc = -qsearch(board, -beta, -alpha, ply+1, ctx);
            unmake_move(board, undo);

            if (ctx.stopped) {
                return alpha;
            }

            if (sc >= beta) {
                record_beta_cutoff(ctx, board, move, ply, 1, move_index);
                return sc;
            }
            if (sc > alpha) {
                alpha = sc;
            }
            if (sc > best_score) {
                best_score = sc;
            }
        }
        if (!found_legal_moves) {
            if (!has_any_legal_move(board)) return VALUE_DRAW;
        }
        return best_score;
    }
}
