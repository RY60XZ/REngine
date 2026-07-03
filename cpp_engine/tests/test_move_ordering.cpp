#include"rengine/fen.h"
#include"rengine/move_ordering.h"
#include"rengine/search.h"
#include"rengine/square.h"
#include<cassert>
#include<string>

namespace rengine {
    Board board_from(const std::string& fen) {
        Board board;
        set_from_fen(board, fen);
        return board;
    }

    void test_mvv_lva_prefers_valuable_victim_low_attacker() {
        Board board = board_from("4k3/p7/5q2/8/4N3/8/8/Q3K3 w - - 0 1");
        Move queen_takes_pawn = encode_move(parse_square("a1"), parse_square("a7"), CAPTURE);
        Move knight_takes_queen = encode_move(parse_square("e4"), parse_square("f6"), CAPTURE);

        assert(move_order_score(board, knight_takes_queen) > move_order_score(board, queen_takes_pawn));

        MoveList moves;
        moves.push_back(queen_takes_pawn);
        moves.push_back(knight_takes_queen);

        Move selected = select_best_move(board, moves, 0);
        assert(selected == knight_takes_queen);
        assert(moves[0] == knight_takes_queen);
        assert(moves[1] == queen_takes_pawn);
    }

    void test_promotion_scores_above_quiet_move() {
        Board board = board_from("4k3/P7/8/8/8/8/8/4K1N1 w - - 0 1");
        Move quiet = encode_move(parse_square("g1"), parse_square("f3"), QUIET);
        Move queen_promotion = encode_move(parse_square("a7"), parse_square("a8"), PROMOTION, QUEEN);
        Move knight_promotion = encode_move(parse_square("a7"), parse_square("a8"), PROMOTION, KNIGHT);

        assert(move_order_score(board, queen_promotion) > move_order_score(board, quiet));
        assert(move_order_score(board, queen_promotion) > move_order_score(board, knight_promotion));
        assert(is_noisy_move(queen_promotion));
        assert(!is_noisy_move(quiet));
    }

    void test_preferred_move_wins_ties_and_scores() {
        Board board = board_from("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
        Move left_rook = encode_move(parse_square("a1"), parse_square("a2"), QUIET);
        Move right_rook = encode_move(parse_square("h1"), parse_square("h2"), QUIET);

        MoveList moves;
        moves.push_back(left_rook);
        moves.push_back(right_rook);

        assert(move_order_score(board, right_rook, right_rook) > move_order_score(board, left_rook, right_rook));
        Move selected = select_best_move(board, moves, 0, right_rook);
        assert(selected == right_rook);
        assert(moves[0] == right_rook);
    }

    void test_select_best_preserves_prior_moves() {
        Board board = board_from("4k3/p7/5q2/8/4N3/8/8/Q3K3 w - - 0 1");
        Move quiet = encode_move(parse_square("e1"), parse_square("d1"), QUIET);
        Move queen_takes_pawn = encode_move(parse_square("a1"), parse_square("a7"), CAPTURE);
        Move knight_takes_queen = encode_move(parse_square("e4"), parse_square("f6"), CAPTURE);

        MoveList moves;
        moves.push_back(quiet);
        moves.push_back(queen_takes_pawn);
        moves.push_back(knight_takes_queen);

        Move selected = select_best_move(board, moves, 1);
        assert(selected == knight_takes_queen);
        assert(moves[0] == quiet);
        assert(moves[1] == knight_takes_queen);
        assert(moves[2] == queen_takes_pawn);
    }

    void test_ordered_search_restores_board() {
        Board board = board_from("q6k/8/8/8/8/8/8/R3K3 w - - 0 1");
        std::string before = board_to_fen(board);

        SearchLimits limits;
        limits.depth = 2;
        SearchResult result = search_position(board, limits);

        assert(board_to_fen(board) == before);
        assert(result.has_best_move);
        assert(result.stats.first_move_cutoffs <= result.stats.cutoffs);
    }
}

int main() {
    rengine::test_mvv_lva_prefers_valuable_victim_low_attacker();
    rengine::test_promotion_scores_above_quiet_move();
    rengine::test_preferred_move_wins_ties_and_scores();
    rengine::test_select_best_preserves_prior_moves();
    rengine::test_ordered_search_restores_board();
}
