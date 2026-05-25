#include"rengine/make_move.h"
#include"rengine/fen.h"
#include"rengine/square.h"
#include<cassert>
#include<string>

namespace rengine {
    Board board_from(const std::string& fen) {
        Board board;
        set_from_fen(board, fen);
        return board;
    }

    void assert_unmake_restores(const std::string& fen, Move move) {
        Board board = board_from(fen);
        Undo undo;

        make_move(board, move, undo);
        unmake_move(board, undo);

        assert(board_to_fen(board) == fen);
    }

    void test_quiet_move() {
        std::string fen = "8/8/8/8/8/8/8/1N6 w - - 3 7";
        Board board = board_from(fen);
        Move move = encode_move(parse_square("b1"), parse_square("c3"), QUIET);
        Undo undo;

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/8/8/8/2N5/8/8 b - - 4 7");
        assert(undo.captured_piece == NO_PIECE);

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);

        fen = "8/8/8/8/8/8/8/1n6 b - - 3 7";
        board = board_from(fen);
        move = encode_move(parse_square("b1"), parse_square("c3"), QUIET);

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/8/8/8/2n5/8/8 w - - 4 8");

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
    }

    void test_capture_move() {
        std::string fen = "8/8/8/8/8/3n4/4P3/8 w KQkq - 5 12";
        Board board = board_from(fen);
        Move move = encode_move(parse_square("e2"), parse_square("d3"), CAPTURE);
        Undo undo;

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/8/8/8/3P4/8/8 b KQkq - 0 12");
        assert(undo.captured_piece == BLACK_KNIGHT);

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
    }

    void test_double_pawn_push() {
        std::string fen = "8/8/8/8/8/8/4P3/8 w - - 5 12";
        Board board = board_from(fen);
        Move move = encode_move(parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH);
        Undo undo;

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/8/8/4P3/8/8/8 b - e3 0 12");

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);

        fen = "8/4p3/8/8/8/8/8/8 b - - 5 12";
        board = board_from(fen);
        move = encode_move(parse_square("e7"), parse_square("e5"), DOUBLE_PAWN_PUSH);

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/8/4p3/8/8/8/8 w - e6 0 13");

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
    }

    void test_en_passant_move() {
        std::string fen = "8/8/8/3pP3/8/8/8/8 w - d6 5 12";
        Board board = board_from(fen);
        Move move = encode_move(parse_square("e5"), parse_square("d6"), EN_PASSANT);
        Undo undo;

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/3P4/8/8/8/8/8 b - - 0 12");
        assert(undo.captured_piece == BLACK_PAWN);

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);

        fen = "8/8/8/8/3Pp3/8/8/8 b - d3 5 12";
        board = board_from(fen);
        move = encode_move(parse_square("e4"), parse_square("d3"), EN_PASSANT);

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/8/8/8/3p4/8/8 w - - 0 13");
        assert(undo.captured_piece == WHITE_PAWN);

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
    }

    void test_castling_move() {
        std::string fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 3 7";
        Board board = board_from(fen);
        Move move = encode_move(parse_square("e1"), parse_square("g1"), CASTLING);
        Undo undo;

        make_move(board, move, undo);
        assert(board_to_fen(board) == "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 4 7");

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);

        fen = "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 3 7";
        board = board_from(fen);
        move = encode_move(parse_square("e8"), parse_square("c8"), CASTLING);

        make_move(board, move, undo);
        assert(board_to_fen(board) == "2kr3r/8/8/8/8/8/8/R3K2R w KQ - 4 8");

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
    }

    void test_promotion_move() {
        std::string fen = "8/P7/8/8/8/8/8/8 w - - 5 12";
        Board board = board_from(fen);
        Move move = encode_move(parse_square("a7"), parse_square("a8"), PROMOTION, QUEEN);
        Undo undo;

        make_move(board, move, undo);
        assert(board_to_fen(board) == "Q7/8/8/8/8/8/8/8 b - - 0 12");

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);

        fen = "1n6/P7/8/8/8/8/8/8 w - - 5 12";
        board = board_from(fen);
        move = encode_move(parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, QUEEN);

        make_move(board, move, undo);
        assert(board_to_fen(board) == "1Q6/8/8/8/8/8/8/8 b - - 0 12");
        assert(undo.captured_piece == BLACK_KNIGHT);

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);

        fen = "8/8/8/8/8/8/p7/8 b - - 5 12";
        board = board_from(fen);
        move = encode_move(parse_square("a2"), parse_square("a1"), PROMOTION, QUEEN);

        make_move(board, move, undo);
        assert(board_to_fen(board) == "8/8/8/8/8/8/8/q7 w - - 0 13");

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
    }

    void test_castling_rights() {
        std::string fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 3 7";
        Board board = board_from(fen);
        Move move = encode_move(parse_square("a1"), parse_square("a2"), QUIET);
        Undo undo;

        make_move(board, move, undo);
        assert((board.castling_rights & WHITE_QUEENSIDE_CASTLING) == 0);
        assert((board.castling_rights & WHITE_KINGSIDE_CASTLING) != 0);

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);

        fen = "r3k2r/8/8/8/8/8/1b6/R3K2R b KQkq - 3 7";
        board = board_from(fen);
        move = encode_move(parse_square("b2"), parse_square("a1"), CAPTURE);

        make_move(board, move, undo);
        assert((board.castling_rights & WHITE_QUEENSIDE_CASTLING) == 0);
        assert((board.castling_rights & WHITE_KINGSIDE_CASTLING) != 0);
        assert(undo.captured_piece == WHITE_ROOK);

        unmake_move(board, undo);
        assert(board_to_fen(board) == fen);
    }

    void test_round_trips() {
        assert_unmake_restores("8/8/8/8/8/8/4P3/8 w - - 5 12",
                               encode_move(parse_square("e2"), parse_square("e3"), QUIET));
        assert_unmake_restores("8/8/8/8/8/8/4P3/8 w - - 5 12",
                               encode_move(parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH));
        assert_unmake_restores("8/8/8/3pP3/8/8/8/8 w - d6 5 12",
                               encode_move(parse_square("e5"), parse_square("d6"), EN_PASSANT));
        assert_unmake_restores("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 3 7",
                               encode_move(parse_square("e1"), parse_square("c1"), CASTLING));
        assert_unmake_restores("8/P7/8/8/8/8/8/8 w - - 5 12",
                               encode_move(parse_square("a7"), parse_square("a8"), PROMOTION, QUEEN));
    }
}

int main() {
    rengine::test_quiet_move();
    rengine::test_capture_move();
    rengine::test_double_pawn_push();
    rengine::test_en_passant_move();
    rengine::test_castling_move();
    rengine::test_promotion_move();
    rengine::test_castling_rights();
    rengine::test_round_trips();
}
