#include"rengine/fen.h"
#include"rengine/move_format.h"
#include"rengine/search.h"
#include"rengine/tt.h"
#include<cassert>
#include<chrono>
#include<string>

namespace rengine {
    Board board_from(const std::string& fen) {
        Board board;
        set_from_fen(board, fen);
        return board;
    }

    void test_startpos_depth_one_search() {
        Board board = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::string before = board_to_fen(board);

        SearchLimits limits;
        limits.depth = 1;
        SearchResult result = search_position(board, limits);

        assert(board_to_fen(board) == before);
        assert(result.has_best_move);
        assert(result.completed_depth == 1);
        assert(result.stats.nodes == 20);
        assert(result.stats.qnodes == 20);
        assert(result.stats.cutoffs == 0);
        assert(result.stats.max_ply == 1);
        assert(result.principal_variation.size() == 1);
        assert(result.principal_variation[0] == result.best_move);
    }

    void test_material_capture_is_selected() {
        Board board = board_from("q6k/8/8/8/8/8/8/R3K3 w - - 0 1");
        SearchLimits limits;
        limits.depth = 1;

        SearchResult result = search_position(board, limits);

        assert(result.has_best_move);
        assert(move_to_uci(result.best_move) == "a1a8");
        assert(result.score == 480);
    }

    void test_static_depth_zero_search() {
        Board board = board_from("4k3/8/8/8/8/8/8/4KQ2 w - - 0 1");
        SearchLimits limits;
        limits.depth = 0;

        SearchResult result = search_position(board, limits);

        assert(!result.has_best_move);
        assert(result.completed_depth == 0);
        assert(result.stats.nodes == 0);
        assert(result.stats.qnodes == 1);
        assert(result.score == 890);
    }

    void test_qsearch_stand_pat_without_noisy_moves() {
        Board board = board_from("4k3/8/8/8/8/8/8/4KQ2 w - - 0 1");
        SearchLimits limits;
        SearchContext ctx{limits, {}, std::chrono::steady_clock::time_point::max(), false, {}};

        Score score = qsearch(board, -VALUE_INF, VALUE_INF, 0, ctx);

        assert(score == 890);
        assert(ctx.stats.qnodes == 1);
    }

    void test_depth_two_pv() {
        Board board = board_from("q6k/8/8/8/8/8/8/R3K3 w - - 0 1");
        SearchLimits limits;
        limits.depth = 2;

        SearchResult result = search_position(board, limits);

        assert(result.has_best_move);
        assert(result.completed_depth == 2);
        assert(result.principal_variation.size() > 1);
        assert(result.principal_variation.size() <= limits.depth);
        assert(result.principal_variation[0] == result.best_move);
    }

    void test_node_limit_is_deterministic() {
        Board board = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        SearchLimits limits;
        limits.depth = 1;
        limits.node_limit = 2;

        SearchResult result = search_position(board, limits);

        assert(result.has_best_move);
        assert(result.completed_depth == 0);
        assert(result.stats.nodes == 1);
        assert(result.stats.qnodes == 1);
        assert(result.stats.stop_reason == StopReason::NodeLimit);
    }

    void test_qsearch_terminal_checkmate() {
        Board board = board_from("7k/5K2/7Q/8/8/8/8/8 b - - 0 1");
        SearchLimits limits;
        SearchContext ctx{limits, {}, std::chrono::steady_clock::time_point::max(), false, {}};

        Score score = qsearch(board, -VALUE_INF, VALUE_INF, 0, ctx);

        assert(score == mated_in(0));
        assert(ctx.stats.qnodes == 1);
    }

    void test_qsearch_mate_takes_precedence_over_fifty_move_draw() {
        Board board = board_from("7k/5K2/7Q/8/8/8/8/8 b - - 100 1");
        SearchLimits limits;
        SearchContext ctx{limits, {}, std::chrono::steady_clock::time_point::max(), false, {}};

        Score score = qsearch(board, -VALUE_INF, VALUE_INF, 0, ctx);

        assert(score == mated_in(0));
    }

    void test_qsearch_terminal_stalemate() {
        Board board = board_from("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");
        SearchLimits limits;
        SearchContext ctx{limits, {}, std::chrono::steady_clock::time_point::max(), false, {}};

        Score score = qsearch(board, -VALUE_INF, VALUE_INF, 0, ctx);

        assert(score == VALUE_DRAW);
    }

    void test_terminal_checkmate() {
        Board board = board_from("7k/5K2/7Q/8/8/8/8/8 b - - 0 1");
        SearchLimits limits;
        limits.depth = 1;

        SearchResult result = search_position(board, limits);

        assert(!result.has_best_move);
        assert(result.score == mated_in(0));
        assert(result.stats.stop_reason == StopReason::NoLegalMoves);
    }

    void test_terminal_stalemate() {
        Board board = board_from("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");
        SearchLimits limits;
        limits.depth = 1;

        SearchResult result = search_position(board, limits);

        assert(!result.has_best_move);
        assert(result.score == VALUE_DRAW);
        assert(result.stats.stop_reason == StopReason::NoLegalMoves);
    }

    void test_fifty_move_draw() {
        Board board = board_from("4k3/8/8/8/8/8/8/4KQ2 w - - 100 1");
        SearchLimits limits;
        limits.depth = 1;

        SearchResult result = search_position(board, limits);

        assert(!result.has_best_move);
        assert(result.score == VALUE_DRAW);
    }

    void test_quiet_mate_takes_precedence_over_fifty_move_draw() {
        Board board = board_from("7k/8/5KQ1/8/8/8/8/8 w - - 99 1");
        SearchLimits limits;
        limits.depth = 2;

        SearchResult result = search_position(board, limits);

        assert(result.has_best_move);
        assert(move_to_uci(result.best_move) == "g6g7");
        assert(result.score == mate_in(1));
    }

    void test_null_move_pruning_cutoff() {
        Board board = board_from("4k3/8/8/8/8/8/8/4KQ2 w - - 0 1");
        const std::string before = board_to_fen(board);
        SearchLimits limits;
        TranspositionTable tt(1024);
        SearchContext ctx{limits, {}, std::chrono::steady_clock::time_point::max(), false, {}, &tt};
        MoveList pv;

        Score score = negamax(board, 3, 500, 600, 0, ctx, pv);

        assert(board_to_fen(board) == before);
        assert(score == 600);
        assert(ctx.stats.null_move_searches == 1);
        assert(ctx.stats.null_move_cutoffs == 1);
    }

    void test_null_move_pruning_skips_stalemate() {
        Board board = board_from("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");
        SearchLimits limits;
        TranspositionTable tt(1024);
        SearchContext ctx{limits, {}, std::chrono::steady_clock::time_point::max(), false, {}, &tt};
        MoveList pv;

        Score score = negamax(board, 3, -1100, -1000, 0, ctx, pv);

        assert(score == VALUE_DRAW);
        assert(ctx.stats.null_move_searches == 0);
        assert(ctx.stats.null_move_cutoffs == 0);
    }

    void test_late_move_reduction_searches_late_quiet_moves_at_reduced_depth() {
        Board board = board_from("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
        const std::string before = board_to_fen(board);
        SearchLimits limits;
        TranspositionTable tt(1024);
        SearchContext ctx{limits, {}, std::chrono::steady_clock::time_point::max(), false, {}, &tt};
        MoveList pv;

        Score score = negamax(board, 4, -VALUE_INF, VALUE_INF, 0, ctx, pv);

        assert(board_to_fen(board) == before);
        assert(score > -VALUE_INF);
        assert(ctx.stats.lmr_reductions > 0);
    }
}

int main() {
    rengine::test_startpos_depth_one_search();
    rengine::test_material_capture_is_selected();
    rengine::test_static_depth_zero_search();
    rengine::test_qsearch_stand_pat_without_noisy_moves();
    rengine::test_depth_two_pv();
    rengine::test_node_limit_is_deterministic();
    rengine::test_qsearch_terminal_checkmate();
    rengine::test_qsearch_mate_takes_precedence_over_fifty_move_draw();
    rengine::test_qsearch_terminal_stalemate();
    rengine::test_terminal_checkmate();
    rengine::test_terminal_stalemate();
    rengine::test_fifty_move_draw();
    rengine::test_quiet_mate_takes_precedence_over_fifty_move_draw();
    rengine::test_null_move_pruning_cutoff();
    rengine::test_null_move_pruning_skips_stalemate();
    rengine::test_late_move_reduction_searches_late_quiet_moves_at_reduced_depth();
}
