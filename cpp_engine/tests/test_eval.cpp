#include"rengine/eval.h"
#include"rengine/fen.h"
#include<cassert>
#include<string>

namespace rengine {
    Board board_from(const std::string& fen) {
        Board board;
        set_from_fen(board, fen);
        return board;
    }

    void test_zero_material() {
        Board empty = board_from("8/8/8/8/8/8/8/8 w - - 0 1");
        assert(evaluate(empty) == VALUE_DRAW);

        Board kings_only_white = board_from("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
        assert(evaluate(kings_only_white) == VALUE_DRAW);

        Board kings_only_black = board_from("4k3/8/8/8/8/8/8/4K3 b - - 0 1");
        assert(evaluate(kings_only_black) == VALUE_DRAW);
    }

    void test_side_to_move_perspective() {
        Board white_to_move = board_from("4k3/8/8/8/8/8/8/4KQ2 w - - 0 1");
        assert(evaluate(white_to_move) == 900);

        Board black_to_move = board_from("4k3/8/8/8/8/8/8/4KQ2 b - - 0 1");
        assert(evaluate(black_to_move) == -900);
    }

    void test_material_values() {
        Board board = board_from("2rbk3/8/8/8/8/8/8/4KQN1 w - - 0 1");
        assert(evaluate(board) == 390);
    }

    void test_fen_round_trip_preserves_eval() {
        Board board = board_from("r3k2r/ppp2ppp/2n5/3qp3/8/2N2Q2/PPP2PPP/R3K2R w KQkq - 3 12");
        Score before = evaluate(board);

        Board round_trip = board_from(board_to_fen(board));
        assert(evaluate(round_trip) == before);
    }

    void test_evaluate_does_not_mutate_board() {
        Board board = board_from("r3k2r/ppp2ppp/2n5/3qp3/8/2N2Q2/PPP2PPP/R3K2R b KQkq - 3 12");
        std::string before = board_to_fen(board);

        (void)evaluate(board);

        assert(board_to_fen(board) == before);
    }

    void test_mate_score_helpers() {
        assert(mate_in(1) > mate_in(2));
        assert(mated_in(1) < mated_in(2));
        assert(is_mate_score(mate_in(MAX_PLY)));
        assert(is_mate_score(mated_in(MAX_PLY)));
        assert(!is_mate_score(VALUE_INF));
        assert(!is_mate_score(900));

        Score score = mate_in(5);
        assert(score_from_tt(score_to_tt(score, 3), 3) == score);
        assert(score_from_tt(score_to_tt(mate_in(1), 10), 10) == mate_in(1));
        assert(score_from_tt(score_to_tt(mated_in(1), 10), 10) == mated_in(1));
        assert(score_to_tt(VALUE_INF, 10) == VALUE_INF);
    }
}

int main() {
    rengine::test_zero_material();
    rengine::test_side_to_move_perspective();
    rengine::test_material_values();
    rengine::test_fen_round_trip_preserves_eval();
    rengine::test_evaluate_does_not_mutate_board();
    rengine::test_mate_score_helpers();
}
