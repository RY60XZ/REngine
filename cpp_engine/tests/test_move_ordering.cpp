#include"rengine/fen.h"
#include"rengine/move_ordering.h"
#include"rengine/see.h"
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

    void test_see_scores_capture_sequences() {
        Board winning = board_from("4k3/8/5q2/8/4N3/8/8/4K3 w - - 0 1");
        Move knight_takes_queen = encode_move(parse_square("e4"), parse_square("f6"), CAPTURE);
        assert(see_score(winning, knight_takes_queen) == 900);
        assert(see_ge(winning, knight_takes_queen, 0));

        Board losing = board_from("r3k3/p7/8/8/8/8/8/Q3K3 w - - 0 1");
        Move queen_takes_pawn = encode_move(parse_square("a1"), parse_square("a7"), CAPTURE);
        assert(see_score(losing, queen_takes_pawn) == -800);
        assert(!see_ge(losing, queen_takes_pawn, 0));

        Board en_passant = board_from("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
        Move ep = encode_move(parse_square("e5"), parse_square("d6"), EN_PASSANT);
        assert(see_score(en_passant, ep) == 100);
        assert(see_ge(en_passant, ep, 0));
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

    void test_scored_selection_counts_killer_stats_once() {
        Board board = board_from("4k3/p7/5q2/8/4N3/8/8/R3K2R w KQ - 0 1");
        SearchStack stack;
        SearchStats stats;
        Move quiet_one = encode_move(parse_square("a1"), parse_square("a2"), QUIET);
        Move quiet_two = encode_move(parse_square("h1"), parse_square("h2"), QUIET);
        Move knight_takes_queen = encode_move(parse_square("e4"), parse_square("f6"), CAPTURE);

        assert(store_killer_move(stack, 3, quiet_two));

        MoveList moves;
        moves.push_back(quiet_one);
        moves.push_back(quiet_two);
        moves.push_back(knight_takes_queen);

        MoveScoreList scores{};
        score_moves(board, moves, scores, stack, 3, 0, &stats);
        assert(stats.killer_probes == 2);
        assert(stats.killer_hits == 1);

        Move selected = select_best_move(moves, scores, 0);
        assert(selected == knight_takes_queen);
        assert(stats.killer_probes == 2);
        assert(stats.killer_hits == 1);

        selected = select_best_move(moves, scores, 1);
        assert(selected == quiet_two);
        assert(stats.killer_probes == 2);
        assert(stats.killer_hits == 1);
    }

    void test_killer_moves_ignore_noisy_moves_and_avoid_duplicates() {
        SearchStack stack;
        Move quiet_one = encode_move(parse_square("a1"), parse_square("a2"), QUIET);
        Move quiet_two = encode_move(parse_square("h1"), parse_square("h2"), QUIET);
        Move capture = encode_move(parse_square("a1"), parse_square("a7"), CAPTURE);

        assert(!store_killer_move(stack, 4, capture));
        assert(killer_slot_for_move(stack, 4, capture) == -1);
        assert(stack.killer_moves[4][0] == 0);
        assert(stack.killer_moves[4][1] == 0);

        assert(store_killer_move(stack, 4, quiet_one));
        assert(stack.killer_moves[4][0] == quiet_one);
        assert(stack.killer_moves[4][1] == 0);

        assert(!store_killer_move(stack, 4, quiet_one));
        assert(stack.killer_moves[4][0] == quiet_one);
        assert(stack.killer_moves[4][1] == 0);

        assert(store_killer_move(stack, 4, quiet_two));
        assert(stack.killer_moves[4][0] == quiet_two);
        assert(stack.killer_moves[4][1] == quiet_one);

        assert(!store_killer_move(stack, 4, quiet_one));
        assert(stack.killer_moves[4][0] == quiet_one);
        assert(stack.killer_moves[4][1] == quiet_two);
        assert(stack.killer_moves[4][0] != stack.killer_moves[4][1]);
    }

    void test_history_scores_only_quiet_moves() {
        SearchStack stack;
        Move quiet = encode_move(parse_square("a1"), parse_square("a2"), QUIET);
        Move capture = encode_move(parse_square("a1"), parse_square("a7"), CAPTURE);

        update_history(stack, WHITE, quiet, 4);
        update_history(stack, WHITE, capture, 10);

        assert(history_score(stack, WHITE, quiet) == 16);
        assert(history_score(stack, BLACK, quiet) == 0);
        assert(history_score(stack, WHITE, capture) == 0);
    }

    void test_killer_orders_above_history_but_below_capture() {
        Board board = board_from("4k3/p7/5q2/8/4N3/8/8/R3K2R w KQ - 0 1");
        SearchStack stack;
        Move history_quiet = encode_move(parse_square("a1"), parse_square("a2"), QUIET);
        Move killer_quiet = encode_move(parse_square("h1"), parse_square("h2"), QUIET);
        Move knight_takes_queen = encode_move(parse_square("e4"), parse_square("f6"), CAPTURE);

        update_history(stack, WHITE, history_quiet, MAX_PLY);
        assert(store_killer_move(stack, 3, killer_quiet));

        assert(move_order_score(board, killer_quiet, stack, 3) >
               move_order_score(board, history_quiet, stack, 3));
        assert(move_order_score(board, knight_takes_queen, stack, 3) >
               move_order_score(board, killer_quiet, stack, 3));

        MoveList moves;
        moves.push_back(history_quiet);
        moves.push_back(killer_quiet);

        Move selected = select_best_move(board, moves, 0, stack, 3);
        assert(selected == killer_quiet);
    }

    void test_ordered_search_restores_board() {
        Board board = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::string before = board_to_fen(board);

        SearchLimits limits;
        limits.depth = 3;
        SearchResult result = search_position(board, limits);

        assert(board_to_fen(board) == before);
        assert(result.has_best_move);
        assert(result.stats.first_move_cutoffs <= result.stats.cutoffs);
        assert(result.stats.killer_hits <= result.stats.killer_probes);
        assert(result.stats.killer_cutoffs + result.stats.history_quiet_cutoffs <= result.stats.cutoffs);
        assert(result.stats.killer_probes > 0);
        assert(result.stats.killer_cutoffs + result.stats.history_quiet_cutoffs > 0);
    }
}

int main() {
    rengine::test_mvv_lva_prefers_valuable_victim_low_attacker();
    rengine::test_promotion_scores_above_quiet_move();
    rengine::test_see_scores_capture_sequences();
    rengine::test_preferred_move_wins_ties_and_scores();
    rengine::test_select_best_preserves_prior_moves();
    rengine::test_scored_selection_counts_killer_stats_once();
    rengine::test_killer_moves_ignore_noisy_moves_and_avoid_duplicates();
    rengine::test_history_scores_only_quiet_moves();
    rengine::test_killer_orders_above_history_but_below_capture();
    rengine::test_ordered_search_restores_board();
}
