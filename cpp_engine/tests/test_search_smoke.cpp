#include"rengine/fen.h"
#include"rengine/move_format.h"
#include"rengine/search.h"
#include<cassert>
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
        assert(result.stats.qnodes == 0);
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
        assert(result.score == 500);
    }

    void test_static_depth_zero_search() {
        Board board = board_from("4k3/8/8/8/8/8/8/4KQ2 w - - 0 1");
        SearchLimits limits;
        limits.depth = 0;

        SearchResult result = search_position(board, limits);

        assert(!result.has_best_move);
        assert(result.completed_depth == 0);
        assert(result.stats.nodes == 0);
        assert(result.score == 900);
    }

    void test_node_limit_is_deterministic() {
        Board board = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        SearchLimits limits;
        limits.depth = 1;
        limits.node_limit = 1;

        SearchResult result = search_position(board, limits);

        assert(result.has_best_move);
        assert(result.completed_depth == 0);
        assert(result.stats.nodes == 1);
        assert(result.stats.stop_reason == StopReason::NodeLimit);
    }
}

int main() {
    rengine::test_startpos_depth_one_search();
    rengine::test_material_capture_is_selected();
    rengine::test_static_depth_zero_search();
    rengine::test_node_limit_is_deterministic();
}
