#include"rengine/movegen.h"
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

    bool has_move(const MoveList& move_list, Square from, Square to,
                  MoveFlag flag, PieceType promotion = PAWN) {
        for (Move move : move_list) {
            if (move_from(move) == from &&
                move_to(move) == to &&
                move_flag(move) == flag &&
                move_promotion_type(move) == promotion) {
                return true;
            }
        }
        return false;
    }

    int count_move(const MoveList& move_list, Square from, Square to,
                   MoveFlag flag, PieceType promotion = PAWN) {
        int count = 0;
        for (Move move : move_list) {
            if (move_from(move) == from &&
                move_to(move) == to &&
                move_flag(move) == flag &&
                move_promotion_type(move) == promotion) {
                ++count;
            }
        }
        return count;
    }

    int count_flag(const MoveList& move_list, MoveFlag flag) {
        int count = 0;
        for (Move move : move_list) {
            if (move_flag(move) == flag) {
                ++count;
            }
        }
        return count;
    }

    void test_start_position() {
        Board board = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        MoveList move_list;

        generate_pseudo_legal_moves(board, move_list);
        assert(move_list.size() == 20);
        assert(has_move(move_list, parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH));
        assert(has_move(move_list, parse_square("g1"), parse_square("f3"), QUIET));

        board = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
        move_list.clear();
        generate_pseudo_legal_moves(board, move_list);
        assert(move_list.size() == 20);
        assert(has_move(move_list, parse_square("e7"), parse_square("e5"), DOUBLE_PAWN_PUSH));
        assert(has_move(move_list, parse_square("g8"), parse_square("f6"), QUIET));
    }

    void test_pawn_pushes() {
        Board board = board_from("8/8/8/8/8/8/4P3/8 w - - 0 1");
        MoveList move_list;

        generate_pawn_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("e2"), parse_square("e3"), QUIET));
        assert(has_move(move_list, parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH));

        board = board_from("8/8/8/8/8/4n3/4P3/8 w - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, WHITE);
        assert(!has_move(move_list, parse_square("e2"), parse_square("e3"), QUIET));
        assert(!has_move(move_list, parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH));

        board = board_from("8/4p3/4N3/8/8/8/8/8 b - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, BLACK);
        assert(!has_move(move_list, parse_square("e7"), parse_square("e6"), QUIET));
        assert(!has_move(move_list, parse_square("e7"), parse_square("e5"), DOUBLE_PAWN_PUSH));
    }

    void test_pawn_captures() {
        Board board = board_from("8/8/8/8/8/1n6/P1P5/8 w - - 0 1");
        MoveList move_list;

        generate_pawn_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("a2"), parse_square("b3"), CAPTURE));
        assert(has_move(move_list, parse_square("c2"), parse_square("b3"), CAPTURE));
        assert(count_move(move_list, parse_square("a2"), parse_square("b3"), CAPTURE) == 1);
        assert(count_move(move_list, parse_square("c2"), parse_square("b3"), CAPTURE) == 1);

        board = board_from("8/8/p1p5/1N6/8/8/8/8 b - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, BLACK);
        assert(has_move(move_list, parse_square("a6"), parse_square("b5"), CAPTURE));
        assert(has_move(move_list, parse_square("c6"), parse_square("b5"), CAPTURE));
    }

    void test_pawn_promotions() {
        Board board = board_from("8/P7/8/8/8/8/8/8 w - - 0 1");
        MoveList move_list;

        generate_pawn_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("a7"), parse_square("a8"), PROMOTION, KNIGHT));
        assert(has_move(move_list, parse_square("a7"), parse_square("a8"), PROMOTION, BISHOP));
        assert(has_move(move_list, parse_square("a7"), parse_square("a8"), PROMOTION, ROOK));
        assert(has_move(move_list, parse_square("a7"), parse_square("a8"), PROMOTION, QUEEN));
        assert(!has_move(move_list, parse_square("a7"), parse_square("a8"), QUIET));

        board = board_from("1n6/P7/8/8/8/8/8/8 w - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, KNIGHT));
        assert(has_move(move_list, parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, BISHOP));
        assert(has_move(move_list, parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, ROOK));
        assert(has_move(move_list, parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, QUEEN));

        board = board_from("8/P7/1n6/8/8/8/8/8 w - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, WHITE);
        assert(count_flag(move_list, PROMOTION_CAPTURE) == 0);

        board = board_from("8/8/8/8/8/8/p7/8 b - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, BLACK);
        assert(has_move(move_list, parse_square("a2"), parse_square("a1"), PROMOTION, KNIGHT));
        assert(has_move(move_list, parse_square("a2"), parse_square("a1"), PROMOTION, BISHOP));
        assert(has_move(move_list, parse_square("a2"), parse_square("a1"), PROMOTION, ROOK));
        assert(has_move(move_list, parse_square("a2"), parse_square("a1"), PROMOTION, QUEEN));

        board = board_from("8/8/8/8/8/8/p7/1N6 b - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, BLACK);
        assert(has_move(move_list, parse_square("a2"), parse_square("b1"), PROMOTION_CAPTURE, KNIGHT));
        assert(has_move(move_list, parse_square("a2"), parse_square("b1"), PROMOTION_CAPTURE, BISHOP));
        assert(has_move(move_list, parse_square("a2"), parse_square("b1"), PROMOTION_CAPTURE, ROOK));
        assert(has_move(move_list, parse_square("a2"), parse_square("b1"), PROMOTION_CAPTURE, QUEEN));
    }

    void test_qsearch_promotions() {
        Board board = board_from("1n5k/P7/8/8/8/8/8/4K3 w - - 0 1");
        MoveList move_list;

        generate_qsearch_pseudo_legal_moves(board, move_list);

        assert(count_flag(move_list, PROMOTION) == 4);
        assert(count_flag(move_list, PROMOTION_CAPTURE) == 4);
        for (int pt = KNIGHT; pt <= QUEEN; ++pt) {
            PieceType promotion = static_cast<PieceType>(pt);
            assert(count_move(move_list, parse_square("a7"), parse_square("a8"), PROMOTION, promotion) == 1);
            assert(count_move(move_list, parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, promotion) == 1);
        }
    }

    void test_qsearch_legal_move_probe() {
        Board board = board_from("4k3/8/8/8/8/8/8/4KQ2 w - - 0 1");
        MoveList move_list;

        generate_qsearch_pseudo_legal_moves(board, move_list);

        assert(has_any_legal_move(board));
        assert(move_list.empty());

        board = board_from("7k/p4K2/6Q1/8/8/8/8/8 b - - 0 1");
        move_list.clear();

        generate_qsearch_pseudo_legal_moves(board, move_list);

        assert(has_any_legal_move(board));
        assert(move_list.empty());

        board = board_from("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");
        move_list.clear();

        generate_qsearch_pseudo_legal_moves(board, move_list);

        assert(!has_any_legal_move(board));
        assert(move_list.empty());
    }

    void test_pawn_en_passant() {
        Board board = board_from("8/8/8/3pP3/8/8/8/8 w - d6 0 1");
        MoveList move_list;

        generate_pawn_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("e5"), parse_square("d6"), EN_PASSANT));

        board = board_from("8/8/8/3pP3/8/8/8/8 w - - 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, WHITE);
        assert(!has_move(move_list, parse_square("e5"), parse_square("d6"), EN_PASSANT));

        board = board_from("8/8/8/8/3Pp3/8/8/8 b - d3 0 1");
        move_list.clear();
        generate_pawn_moves(board, move_list, BLACK);
        assert(has_move(move_list, parse_square("e4"), parse_square("d3"), EN_PASSANT));
    }

    void test_knight_moves() {
        Board board = board_from("8/8/8/5P2/3N4/5p2/8/8 w - - 0 1");
        MoveList move_list;

        generate_knight_moves(board, move_list, WHITE);
        assert(move_list.size() == 7);
        assert(has_move(move_list, parse_square("d4"), parse_square("f3"), CAPTURE));
        assert(has_move(move_list, parse_square("d4"), parse_square("b3"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("f5"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("f5"), CAPTURE));
    }

    void test_king_moves() {
        Board board = board_from("8/8/8/2p1P3/3K4/8/8/8 w - - 0 1");
        MoveList move_list;

        generate_king_moves(board, move_list, WHITE);
        assert(move_list.size() == 7);
        assert(has_move(move_list, parse_square("d4"), parse_square("c5"), CAPTURE));
        assert(has_move(move_list, parse_square("d4"), parse_square("c3"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("e5"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("e5"), CAPTURE));
    }

    void test_rook_blockers() {
        Board board = board_from("8/3p4/8/8/P2R2p1/8/3P4/8 w - - 0 1");
        MoveList move_list;

        generate_rook_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("d4"), parse_square("d7"), CAPTURE));
        assert(has_move(move_list, parse_square("d4"), parse_square("g4"), CAPTURE));
        assert(has_move(move_list, parse_square("d4"), parse_square("d3"), QUIET));
        assert(has_move(move_list, parse_square("d4"), parse_square("b4"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("d8"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("d2"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("a4"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("h4"), QUIET));
    }

    void test_bishop_blockers() {
        Board board = board_from("8/6p1/1P6/8/3B4/8/5P2/p7 w - - 0 1");
        MoveList move_list;

        generate_bishop_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("d4"), parse_square("g7"), CAPTURE));
        assert(has_move(move_list, parse_square("d4"), parse_square("a1"), CAPTURE));
        assert(has_move(move_list, parse_square("d4"), parse_square("e5"), QUIET));
        assert(has_move(move_list, parse_square("d4"), parse_square("c5"), QUIET));
        assert(has_move(move_list, parse_square("d4"), parse_square("e3"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("h8"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("b6"), QUIET));
        assert(!has_move(move_list, parse_square("d4"), parse_square("f2"), QUIET));
    }

    void test_queen_moves() {
        Board board = board_from("8/8/8/8/3Q4/8/8/8 w - - 0 1");
        MoveList move_list;

        generate_queen_moves(board, move_list, WHITE);
        assert(move_list.size() == 27);
        assert(has_move(move_list, parse_square("d4"), parse_square("d8"), QUIET));
        assert(has_move(move_list, parse_square("d4"), parse_square("h4"), QUIET));
        assert(has_move(move_list, parse_square("d4"), parse_square("a1"), QUIET));
        assert(has_move(move_list, parse_square("d4"), parse_square("h8"), QUIET));
    }

    void test_castling_moves() {
        Board board = board_from("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        MoveList move_list;

        generate_castling_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("e1"), parse_square("g1"), CASTLING));
        assert(has_move(move_list, parse_square("e1"), parse_square("c1"), CASTLING));

        move_list.clear();
        generate_castling_moves(board, move_list, BLACK);
        assert(has_move(move_list, parse_square("e8"), parse_square("g8"), CASTLING));
        assert(has_move(move_list, parse_square("e8"), parse_square("c8"), CASTLING));

        board = board_from("5r2/8/8/8/8/8/8/R3K2R w KQ - 0 1");
        move_list.clear();
        generate_castling_moves(board, move_list, WHITE);
        assert(!has_move(move_list, parse_square("e1"), parse_square("g1"), CASTLING));
        assert(has_move(move_list, parse_square("e1"), parse_square("c1"), CASTLING));

        board = board_from("8/8/8/8/8/8/8/R2NK2R w KQ - 0 1");
        move_list.clear();
        generate_castling_moves(board, move_list, WHITE);
        assert(has_move(move_list, parse_square("e1"), parse_square("g1"), CASTLING));
        assert(!has_move(move_list, parse_square("e1"), parse_square("c1"), CASTLING));
    }
}

int main() {
    rengine::test_start_position();
    rengine::test_pawn_pushes();
    rengine::test_pawn_captures();
    rengine::test_pawn_promotions();
    rengine::test_qsearch_promotions();
    rengine::test_qsearch_legal_move_probe();
    rengine::test_pawn_en_passant();
    rengine::test_knight_moves();
    rengine::test_king_moves();
    rengine::test_rook_blockers();
    rengine::test_bishop_blockers();
    rengine::test_queen_moves();
    rengine::test_castling_moves();
}
