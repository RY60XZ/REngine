#include "rengine/eval.h"
#include "rengine/fen.h"
#include "rengine/make_move.h"
#include "rengine/nnue.h"
#include "rengine/square.h"
#include <cassert>
#include <string>

namespace rengine {
    Board board_from(const std::string& fen) {
        Board board;
        set_from_fen(board, fen);
        assert(board.nnue_initialized);
        assert(!board.nnue_dirty);
        assert(nnue_accumulator_matches_recompute(board));
        return board;
    }

    void assert_incremental_move(const std::string& fen, Move move) {
        Board board = board_from(fen);
        NnueAccumulator original_accumulator = board.nnue_accumulator;
        Undo undo{};

        make_move(board, move, undo);
        assert(board.nnue_initialized);
        assert(!board.nnue_dirty);
        assert(nnue_accumulator_matches_recompute(board));
        (void)evaluate(board);
        assert(nnue_accumulator_matches_recompute(board));

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
        assert(board.nnue_initialized);
        assert(!board.nnue_dirty);
        assert(board.nnue_accumulator == original_accumulator);
        assert(nnue_accumulator_matches_recompute(board));
    }

    void test_incremental_piece_moves() {
        assert_incremental_move(
            "4k3/8/8/8/8/8/8/1N2K3 w - - 3 7",
            encode_move(parse_square("b1"), parse_square("c3"), QUIET));

        assert_incremental_move(
            "4k3/8/8/8/8/3n4/4P3/4K3 w - - 5 12",
            encode_move(parse_square("e2"), parse_square("d3"), CAPTURE));

        assert_incremental_move(
            "4k3/8/8/8/8/8/4P3/4K3 w - - 5 12",
            encode_move(parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH));

        assert_incremental_move(
            "4k3/8/8/3pP3/8/8/8/4K3 w - d6 5 12",
            encode_move(parse_square("e5"), parse_square("d6"), EN_PASSANT));
    }

    void test_incremental_castling_and_rights() {
        assert_incremental_move(
            "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 3 7",
            encode_move(parse_square("e1"), parse_square("g1"), CASTLING));

        assert_incremental_move(
            "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 3 7",
            encode_move(parse_square("a1"), parse_square("a2"), QUIET));

        assert_incremental_move(
            "r3k2r/8/8/8/8/8/1b6/R3K2R b KQkq - 3 7",
            encode_move(parse_square("b2"), parse_square("a1"), CAPTURE));
    }

    void test_incremental_promotion() {
        assert_incremental_move(
            "4k3/P7/8/8/8/8/8/4K3 w - - 5 12",
            encode_move(parse_square("a7"), parse_square("a8"), PROMOTION, QUEEN));

        assert_incremental_move(
            "1n2k3/P7/8/8/8/8/8/4K3 w - - 5 12",
            encode_move(parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, QUEEN));
    }

    void test_null_move_restores_accumulator() {
        std::string fen = "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2";
        Board board = board_from(fen);
        NnueAccumulator original_accumulator = board.nnue_accumulator;
        Undo undo{};

        make_null_move(board, undo);
        assert(board.nnue_accumulator == original_accumulator);
        assert(nnue_accumulator_matches_recompute(board));

        unmake_null_move(board, undo);
        assert(board_to_fen(board) == fen);
        assert(board.nnue_accumulator == original_accumulator);
        assert(nnue_accumulator_matches_recompute(board));
    }
}

int main() {
    assert(rengine::load_default_nnue());
    assert(rengine::nnue_is_loaded());

    rengine::test_incremental_piece_moves();
    rengine::test_incremental_castling_and_rights();
    rengine::test_incremental_promotion();
    rengine::test_null_move_restores_accumulator();
}
